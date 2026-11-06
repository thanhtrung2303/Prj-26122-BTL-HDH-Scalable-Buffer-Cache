// Hash-bucket buffer cache.
// Lock order: eviction_lock -> buckets ascending -> lru_clock_lock.
// Release cache spinlocks before waiting for a sleeplock or disk I/O.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#include "cache_stats.h"

#define NB_BUCKET 13

struct cache_bucket {
  struct spinlock lock;
  struct buf *head;
};

static struct {
  struct spinlock eviction_lock;
  struct cache_bucket bucket[NB_BUCKET];
  struct buf buf[NBUF];
} bcache;

static struct spinlock lru_clock_lock;
static uint64 lru_clock;

static uint
cache_hash(uint dev, uint blockno)
{
  return ((uint64)blockno + (uint64)dev) % NB_BUCKET;
}

// Caller holds the corresponding bucket lock.
static struct buf *
lookup(uint index, uint dev, uint blockno)
{
  struct buf *b;

  for (b = bcache.bucket[index].head; b; b = b->hnext) {
    if (b->mapped && b->dev == dev && b->blockno == blockno)
      return b;
  }
  return 0;
}

static void
lock_all_buckets(void)
{
  for (int i = 0; i < NB_BUCKET; i++)
    acquire(&bcache.bucket[i].lock);
}

static void
unlock_all_buckets(void)
{
  for (int i = NB_BUCKET - 1; i >= 0; i--)
    release(&bcache.bucket[i].lock);
}

// Caller holds eviction_lock and all bucket locks.
// Prefer unmapped buffers, then the oldest idle buffer.
// Equal timestamps retain the lowest array index.
static struct buf *
choose_victim(void)
{
  struct buf *victim = 0;

  for (int i = 0; i < NBUF; i++) {
    struct buf *b = &bcache.buf[i];

    if (b->refcnt != 0)
      continue;

    if (!b->mapped)
      return b;

    if (victim == 0 || b->lastuse < victim->lastuse)
      victim = b;
  }
  return victim;
}

// Caller holds the old bucket lock.
static void
unlink_buffer(struct buf *b)
{
  uint index = cache_hash(b->dev, b->blockno);
  struct buf **link = &bcache.bucket[index].head;

  while (*link && *link != b)
    link = &(*link)->hnext;

  if (*link == 0)
    panic("bcache: missing buffer");

  *link = b->hnext;
  b->hnext = 0;
}

// Caller holds the buffer's bucket lock.
static void
drop_reference(struct buf *b)
{
  if (b->refcnt == 0)
    panic("bcache: refcnt underflow");

  b->refcnt--;

  if (b->refcnt == 0) {
    acquire(&lru_clock_lock);
    b->lastuse = ++lru_clock;
    release(&lru_clock_lock);
  }
}

void
binit(void)
{
  initlock(&bcache.eviction_lock, "bcache.eviction");
  initlock(&lru_clock_lock, "bcache.clock");
  lru_clock = 0;

  for (int i = 0; i < NB_BUCKET; i++) {
    initlock(&bcache.bucket[i].lock, "bcache.bucket");
    bcache.bucket[i].head = 0;
  }

  for (int i = 0; i < NBUF; i++) {
    struct buf *b = &bcache.buf[i];

    b->valid = 0;
    b->disk = 0;
    b->dev = 0;
    b->blockno = 0;
    b->refcnt = 0;
    b->mapped = 0;
    b->hnext = 0;
    b->lastuse = 0;
    initsleeplock(&b->lock, "buffer");
  }
}

// Success: returns a referenced, sleep-locked buffer.
// READ_AHEAD may return 0 when no buffer is available.
static struct buf *
bget_source(uint dev, uint blockno, enum cache_source source)
{
  uint index = cache_hash(dev, blockno);
  struct buf *b;

  // Fast path: lock only the target bucket.
  acquire(&bcache.bucket[index].lock);
  b = lookup(index, dev, blockno);

  if (b) {
    b->refcnt++;
    release(&bcache.bucket[index].lock);
    acquiresleep(&b->lock);
    return b;
  }

  release(&bcache.bucket[index].lock);

  // Slow path: serialize allocation and stabilize all mappings.
  acquire(&bcache.eviction_lock);
  lock_all_buckets();

  // Another CPU may have inserted this block in the meantime.
  b = lookup(index, dev, blockno);

  if (b) {
    b->refcnt++;
  } else {
    b = choose_victim();

    if (b == 0) {
      unlock_all_buckets();
      release(&bcache.eviction_lock);

      if (source == READ_AHEAD)
        return 0;

      panic("bget: no buffers");
    }

    if (b->mapped)
      unlink_buffer(b);

    b->dev = dev;
    b->blockno = blockno;
    b->mapped = 1;
    b->valid = 0;
    b->refcnt = 1;
    b->lastuse = 0;

    b->hnext = bcache.bucket[index].head;
    bcache.bucket[index].head = b;
  }

  unlock_all_buckets();
  release(&bcache.eviction_lock);

  acquiresleep(&b->lock);
  return b;
}

static struct buf *
bread_source(uint dev, uint blockno, enum cache_source source)
{
  struct buf *b = bget_source(dev, blockno, source);

  if (b == 0)
    return 0;

  if (!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }

  return b;
}

struct buf *
bread(uint dev, uint blockno)
{
  return bread_source(dev, blockno, DEMAND);
}

struct buf *
bread_ra(uint dev, uint blockno)
{
  return bread_source(dev, blockno, READ_AHEAD);
}

void
bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");

  virtio_disk_rw(b, 1);
}

void
brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  // Our reference keeps the mapping stable until decremented.
  uint index = cache_hash(b->dev, b->blockno);

  acquire(&bcache.bucket[index].lock);
  drop_reference(b);
  release(&bcache.bucket[index].lock);
}

void
bpin(struct buf *b)
{
  // Caller already owns a valid reference.
  uint index = cache_hash(b->dev, b->blockno);

  acquire(&bcache.bucket[index].lock);

  if (b->refcnt == 0)
    panic("bpin: no reference");

  b->refcnt++;
  release(&bcache.bucket[index].lock);
}

void
bunpin(struct buf *b)
{
  // The existing pin keeps the mapping stable.
  uint index = cache_hash(b->dev, b->blockno);

  acquire(&bcache.bucket[index].lock);
  drop_reference(b);
  release(&bcache.bucket[index].lock);
}
#if BCACHE_SELFTEST
// Run once, before fsinit, while no other process uses the cache.
// Synthetic mappings are tested without issuing disk I/O.
void
bcache_selftest(void)
{
  struct buf *held[NBUF];
  struct buf *b;
  uint base = FSSIZE + 100;

  printk("BCACHE SELFTEST starting\n");

  // Fill the entire cache with pinned buffers.
  for (int i = 0; i < NBUF; i++) {
    held[i] = bget_source(ROOTDEV, base + i, DEMAND);
    bpin(held[i]);
    brelse(held[i]);
  }

  // A hit must reuse the existing mapping, even with no idle buffer.
  for (int i = 0; i < NBUF; i++) {
    b = bget_source(ROOTDEV, base + i, DEMAND);

    if (b != held[i])
      panic("selftest: duplicate mapping");

    brelse(b);
  }
  printk("PASS: pinned mappings and repeated lookup\n");

  // This block is absent. All buffers are pinned.
  // bread_ra must return before any disk I/O can occur.
  if (bread_ra(ROOTDEV, base + NBUF) != 0)
    panic("selftest: full cache");

  printk("PASS: read-ahead returns zero when full\n");

  // Free exactly one buffer. The next miss must reuse that buffer.
  // Changing dev by one also changes its hash bucket.
  bunpin(held[0]);

  b = bget_source(ROOTDEV + 1, base, DEMAND);
  if (b != held[0])
    panic("selftest: wrong victim");

  if (b->dev != ROOTDEV + 1 || b->blockno != base)
    panic("selftest: wrong mapping");

  // Pin the new mapping, so the entire cache is pinned again.
  bpin(b);
  brelse(b);

  // The old mapping must no longer exist.
  if (bget_source(ROOTDEV, base, READ_AHEAD) != 0)
    panic("selftest: stale mapping");

  // The new mapping must be found at its new bucket.
  b = bget_source(ROOTDEV + 1, base, DEMAND);
  if (b != held[0])
    panic("selftest: moved lookup");
  brelse(b);

  // Every other pinned mapping must remain intact.
  for (int i = 1; i < NBUF; i++) {
    b = bget_source(ROOTDEV, base + i, DEMAND);
    if (b != held[i])
      panic("selftest: pinned victim");
    brelse(b);
  }

  printk("PASS: unpin, reuse and bucket migration\n");

  for (int i = 0; i < NBUF; i++)
    bunpin(held[i]);

  // Remove synthetic mappings before normal filesystem startup.
  acquire(&bcache.eviction_lock);
  lock_all_buckets();

  for (int i = 0; i < NBUF; i++) {
    b = &bcache.buf[i];

    if (b->refcnt != 0)
      panic("selftest: leaked reference");

    b->mapped = 0;
    b->valid = 0;
    b->dev = 0;
    b->blockno = 0;
    b->lastuse = 0;
    b->hnext = 0;
  }

  for (int i = 0; i < NB_BUCKET; i++)
    bcache.bucket[i].head = 0;

  acquire(&lru_clock_lock);
  lru_clock = 0;
  release(&lru_clock_lock);

  unlock_all_buckets();
  release(&bcache.eviction_lock);

  printk("BCACHE SELFTEST PASSED\n");
}
#endif
