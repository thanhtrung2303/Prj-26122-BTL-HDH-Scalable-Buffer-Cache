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

// Each entry is protected by the corresponding bucket lock.
static struct cache_stats bucket_stats[NB_BUCKET];

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

// out points to kernel memory. Copy to user space only after this returns.
// Counters describe recorded events; a lookup may still be waiting for I/O.
void
bcache_stats_snapshot(struct cache_stats *out)
{
  struct cache_stats total = {0};

  lock_all_buckets();
  for (int i = 0; i < NB_BUCKET; i++) {
    const struct cache_stats *s = &bucket_stats[i];

    total.demand_lookups += s->demand_lookups;
    total.demand_lookup_hits += s->demand_lookup_hits;
    total.demand_lookup_misses += s->demand_lookup_misses;
    total.demand_disk_reads += s->demand_disk_reads;
    total.read_ahead_lookups += s->read_ahead_lookups;
    total.read_ahead_lookup_hits += s->read_ahead_lookup_hits;
    total.read_ahead_lookup_misses += s->read_ahead_lookup_misses;
    total.read_ahead_skips += s->read_ahead_skips;
    total.read_ahead_disk_reads += s->read_ahead_disk_reads;
  }
  unlock_all_buckets();

  *out = total;
}

// Reset counters only, preserving cached data and replacement state.
// An in-flight request may record its lookup before reset and its I/O after.
void
bcache_stats_reset(void)
{
  lock_all_buckets();
  memset(bucket_stats, 0, sizeof(bucket_stats));
  unlock_all_buckets();
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

    // Send error if lru_clock reaches max value.
    if (lru_clock == ~(uint64)0) {
      panic("bcache: lru clock overflow");
    }

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
  memset(bucket_stats, 0, sizeof(bucket_stats));

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

#if BCACHE_K01 && !BCACHE_SELFTEST
#error "BCACHE_K01 requires BCACHE_SELFTEST"
#endif

#if BCACHE_K01
#define K01_BLOCK (FSSIZE + 300)
static int k01_arrivals, k01_allocations, k01_rechecks;
static struct buf *k01_child_buf;

// Both harts must miss before either can enter the eviction path.
// Stay on this hart until its peer arrives. No cache lock is held.
static void
k01_after_miss(void)
{
  push_off();
  uint64 start = r_time();
  __atomic_add_fetch(&k01_arrivals, 1, __ATOMIC_ACQ_REL);
  while (__atomic_load_n(&k01_arrivals, __ATOMIC_ACQUIRE) != 2) {
    if (r_time() - start > 50000000)
      panic("K01: second hart did not reach miss");
  }
  pop_off();
}
#endif

// Caller holds the target bucket lock.
static void
record_lookup_hit(struct cache_stats *stats, enum cache_source source)
{
  if (source == DEMAND) {
    stats->demand_lookups++;
    stats->demand_lookup_hits++;
  } else {
    stats->read_ahead_lookups++;
    stats->read_ahead_lookup_hits++;
  }
}

// Success: returns a referenced, sleep-locked buffer.
// READ_AHEAD may return 0 when no buffer is available.
static struct buf *
bget_source(uint dev, uint blockno, enum cache_source source)
{
  uint index = cache_hash(dev, blockno);
  struct cache_stats *stats = &bucket_stats[index];
  struct buf *b;

  // Fast path: lock only the target bucket.
  acquire(&bcache.bucket[index].lock);
  b = lookup(index, dev, blockno);

  if (b) {
    b->refcnt++;
    record_lookup_hit(stats, source);
    release(&bcache.bucket[index].lock);
    acquiresleep(&b->lock);
    return b;
  }

  release(&bcache.bucket[index].lock);

#if BCACHE_K01
  if (dev == ROOTDEV && blockno == K01_BLOCK)
    k01_after_miss();
#endif

  // Slow path: serialize allocation and stabilize all mappings.
  acquire(&bcache.eviction_lock);
  lock_all_buckets();

  // Another CPU may have inserted this block in the meantime.
  b = lookup(index, dev, blockno);

  if (b) {
#if BCACHE_K01
    if (dev == ROOTDEV && blockno == K01_BLOCK)
      k01_rechecks++;
#endif
    b->refcnt++;
    record_lookup_hit(stats, source);
  } else {
    b = choose_victim();

    if (b == 0) {
      if (source == READ_AHEAD) {
        stats->read_ahead_lookups++;
        stats->read_ahead_skips++;
      }
      unlock_all_buckets();
      release(&bcache.eviction_lock);

      if (source == READ_AHEAD)
        return 0;

      panic("bget: no buffers");
    }

    if (b->mapped)
      unlink_buffer(b);

#if BCACHE_K01
    if (dev == ROOTDEV && blockno == K01_BLOCK)
      k01_allocations++;
#endif
    b->dev = dev;
    b->blockno = blockno;
    b->mapped = 1;
    b->valid = 0;
    b->refcnt = 1;
    b->lastuse = 0;

    b->hnext = bcache.bucket[index].head;
    bcache.bucket[index].head = b;
    if (source == DEMAND) {
      stats->demand_lookups++;
      stats->demand_lookup_misses++;
    } else {
      stats->read_ahead_lookups++;
      stats->read_ahead_lookup_misses++;
    }
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
    // The held buffer reference keeps the mapping stable.
    uint index = cache_hash(dev, blockno);
    acquire(&bcache.bucket[index].lock);
    if (source == DEMAND)
      bucket_stats[index].demand_disk_reads++;
    else
      bucket_stats[index].read_ahead_disk_reads++;
    release(&bcache.bucket[index].lock);

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
// Called before fsinit, with every buffer pinned and no concurrent cache user.
static void
check_stats(void)
{
  struct cache_stats snapshot, again;
  const struct cache_stats zero = {0};
  uint64 clock_before = lru_clock;

  // Distinct fields in every bucket catch omitted fields/buckets and narrowing.
  lock_all_buckets();
  for (int i = 0; i < NB_BUCKET; i++) {
    uint64 n = i + 1;
    bucket_stats[i] = (struct cache_stats){
      .demand_lookups = 3 * n,
      .demand_lookup_hits = n,
      .demand_lookup_misses = 2 * n,
      .demand_disk_reads = (1ULL << 32) + 4 * n,
      .read_ahead_lookups = 18 * n,
      .read_ahead_lookup_hits = 5 * n,
      .read_ahead_lookup_misses = 6 * n,
      .read_ahead_skips = 7 * n,
      .read_ahead_disk_reads = 8 * n,
    };
  }
  unlock_all_buckets();

  uint64 sum = NB_BUCKET * (NB_BUCKET + 1) / 2;
  const struct cache_stats expected = {
    3 * sum, sum, 2 * sum, ((uint64)NB_BUCKET << 32) + 4 * sum,
    18 * sum, 5 * sum, 6 * sum, 7 * sum, 8 * sum,
  };
  bcache_stats_snapshot(&snapshot);
  bcache_stats_snapshot(&again);
  if (memcmp(&snapshot, &expected, sizeof(snapshot)) != 0 ||
      memcmp(&snapshot, &again, sizeof(snapshot)) != 0)
    panic("stats: snapshot aggregation");

  bcache_stats_reset();
  bcache_stats_snapshot(&snapshot);
  if (memcmp(&snapshot, &zero, sizeof(snapshot)) != 0)
    panic("stats: reset counters");
  bcache_stats_reset();
  bcache_stats_snapshot(&again);
  if (memcmp(&again, &zero, sizeof(again)) != 0)
    panic("stats: repeated reset");

  if (lru_clock != clock_before)
    panic("stats: reset clock");
  for (int i = 0; i < NBUF; i++) {
    struct buf *b = &bcache.buf[i];
    // Fixture references went 1 -> 2 -> 1, so lastuse must still be zero.
    if (!b->mapped || b->valid || b->refcnt != 1 || b->lastuse != 0)
      panic("stats: reset cache state");
  }
  printk("PASS: stats snapshot aggregation and counter-only reset\n");
}

// Check both representations while mappings are stable. A repeated pointer
// also catches a cycle before following the same link a second time.
static void
check_mappings(void)
{
  int seen[NBUF] = {0};

  acquire(&bcache.eviction_lock);
  lock_all_buckets();
  for (uint i = 0; i < NB_BUCKET; i++) {
    for (struct buf *b = bcache.bucket[i].head; b; b = b->hnext) {
      int j = 0;
      while (j < NBUF && b != &bcache.buf[j])
        j++;
      if (j == NBUF || seen[j] || !b->mapped ||
          cache_hash(b->dev, b->blockno) != i)
        panic("bcache: invalid bucket chain");
      seen[j] = 1;
    }
  }
  for (int j = 0; j < NBUF; j++) {
    struct buf *b = &bcache.buf[j];
    if (seen[j] != b->mapped)
      panic("bcache: lost mapping");
    for (int k = 0; k < j && b->mapped; k++) {
      struct buf *other = &bcache.buf[k];
      if (other->mapped && other->dev == b->dev && other->blockno == b->blockno)
        panic("bcache: duplicate key");
    }
  }
  unlock_all_buckets();
  release(&bcache.eviction_lock);
}

#if BCACHE_K01
void
bcache_k01_child(void)
{
  k01_child_buf = bget_source(ROOTDEV, K01_BLOCK, DEMAND);
  brelse(k01_child_buf);
  kexit(0);
}

void
bcache_k01(void)
{
  int pid = kfork();
  if (pid < 0)
    panic("K01: fork");
  struct buf *b = bget_source(ROOTDEV, K01_BLOCK, DEMAND);
  brelse(b);
  if (kwait(0) != pid || k01_arrivals != 2 || k01_allocations != 1 ||
      k01_rechecks != 1 || b != k01_child_buf || b->refcnt != 0)
    panic("K01: miss/recheck mismatch");
  check_mappings();
  printk("PASS K01: two hart misses, one allocation, one recheck hit\n");
}
#endif

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
  check_stats();
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
  check_mappings();

  // Recycle the sole idle buffer again, this time within its current bucket.
  bunpin(held[0]);
  b = bget_source(ROOTDEV + 1, base + NB_BUCKET, DEMAND);
  if (b != held[0])
    panic("K03: wrong same-bucket victim");
  bpin(b);
  brelse(b);
  if (bget_source(ROOTDEV + 1, base, READ_AHEAD) != 0)
    panic("K03: stale same-bucket key");
  check_mappings();
  printk("PASS K03: same/cross-bucket reuse and mapping invariants\n");

  for (int i = 0; i < NBUF; i++)
    bunpin(held[i]);

  // Remove synthetic mappings before normal filesystem startup.

  // K05: Hold all buffers, then release two from different buckets.
  for (int i = 0; i < NBUF; i++)
    held[i] = bget_source(ROOTDEV, base + i, DEMAND);

  // Make the buffer in the higher-numbered bucket the older one.
  int older = 0;
  int newer = 1;

  if (cache_hash(ROOTDEV, base) < cache_hash(ROOTDEV, base + 1)) {
    older = 1;
    newer = 0;
  }

  brelse(held[older]);
  brelse(held[newer]);

  // A cache miss should reuse the buffer that was released first.
  b = bget_source(ROOTDEV, base + NBUF, DEMAND);

  if (b != held[older])
    panic("selftest: global LRU");

  held[older] = b;
  printk("PASS K05: oldest idle buffer across buckets\n");

  // K06: Reacquire the remaining idle buffer and test pin/unpin.
  b = bget_source(ROOTDEV, base + newer, DEMAND);

  uint64 previous = b->lastuse;
  uint64 clock_before = lru_clock;

  bpin(b);
  brelse(b);

  // refcnt from 2 to 1: timestamp remain unchanged.
  if (b->refcnt != 1 || b->lastuse != previous || lru_clock != clock_before)
    panic("selftest: timestamp while pinned");

  // All buffers are now either held or pinned.
  if (bget_source(ROOTDEV, base + NBUF + 1, READ_AHEAD) != 0)
    panic("selftest: reused pinned buffer");

  bunpin(b);

  // refcnt from 1 to 0: assign new timestamp.
  if (b->refcnt != 0 || b->lastuse != clock_before + 1 ||
      lru_clock != clock_before + 1)
    panic("selftest: last unpin timestamp");

  printk("PASS K06: timestamp only on final release\n");

  // Release the remaining held buffers; the newer buffer is now idle.
  for (int i = 0; i < NBUF; i++) {
    if (i != newer)
      brelse(held[i]);
  }
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

#define K07_BLOCK (FSSIZE + 200)

static int k07_can_release;

void
bcache_k07_child(void)
{
  struct buf *b = bget_source(ROOTDEV, K07_BLOCK, DEMAND);
  uint index = cache_hash(ROOTDEV, K07_BLOCK);

  // Hold the final reference until the parent checks the first release.
  while (!__atomic_load_n(&k07_can_release, __ATOMIC_ACQUIRE))
    yield();

  acquire(&bcache.bucket[index].lock);
  uint64 previous = b->lastuse;
  release(&bcache.bucket[index].lock);

  brelse(b);

  acquire(&bcache.bucket[index].lock);
  if (b->refcnt != 0 || b->lastuse <= previous)
    panic("K07: final release did not update timestamp");
  release(&bcache.bucket[index].lock);

  kexit(0);
}

void
bcache_k07(void)
{
  struct buf *held[NBUF];
  uint index = cache_hash(ROOTDEV, K07_BLOCK);

  // Fill the cache with synthetic buffers without reading from disk.
  for (int i = 0; i < NBUF; i++)
    held[i] = bget_source(ROOTDEV, K07_BLOCK + i, DEMAND);

  struct buf *b = held[0];
  uint64 previous = b->lastuse;
  int pid = kfork();
  if (pid < 0)
    panic("K07: fork failed");

  acquire(&tickslock);
  uint start = ticks;
  release(&tickslock);

  // Wait until the child sleeps on this exact buffer lock.
  while (!bcache_k07_sleeping(pid, &b->lock)) {
    acquire(&tickslock);
    uint elapsed = ticks - start;
    release(&tickslock);

    if (elapsed >= 100)
      panic("K07: child did not sleep on buffer");
    yield();
  }

  acquire(&bcache.bucket[index].lock);
  if (b->refcnt != 2)
    panic("K07: waiter reference missing");
  release(&bcache.bucket[index].lock);

  brelse(b);

  acquire(&bcache.bucket[index].lock);
  if (b->refcnt != 1 || b->lastuse != previous)
    panic("K07: timestamp changed before final release");
  release(&bcache.bucket[index].lock);

  // All buffers still have references, so a cache miss must return zero.
  if (bget_source(ROOTDEV, K07_BLOCK + NBUF, READ_AHEAD) != 0)
    panic("K07: reused referenced buffer");

  for (int i = 1; i < NBUF; i++)
    brelse(held[i]);

  __atomic_store_n(&k07_can_release, 1, __ATOMIC_RELEASE);
  if (kwait(0) != pid)
    panic("K07: wait failed");

  printk("PASS K07: waiter protects buffer until final release\n");
}
#endif
