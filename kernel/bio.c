// Scalable buffer cache using hash buckets and per-bucket LRU.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct bucket buckets[NBUCKET];
  struct spinlock evict_lock;
  struct buf buf[NBUF];
} bcache;

static uint
hash(uint dev, uint blockno)
{
  return (dev * 31 + blockno) % NBUCKET;
}

static void
remove_buf(struct buf *b)
{
  b->prev->next = b->next;
  b->next->prev = b->prev;
}

static void
insert_head(struct buf *head, struct buf *b)
{
  b->next = head->next;
  b->prev = head;
  head->next->prev = b;
  head->next = b;
}

void
binit(void)
{
  struct buf *b;
  int i;

  for (i = 0; i < NBUCKET; i++) {
    initlock(&bcache.buckets[i].lock, "bcache");
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
  }

  initlock(&bcache.evict_lock, "bcache_evict");

  for (i = 0, b = bcache.buf; i < NBUF; i++, b++) {
    initsleeplock(&b->lock, "buffer");
    b->valid = 0;
    b->disk = 0;
    b->refcnt = 0;

    // Give unused buffers distinct placeholder identities.
    b->dev = 0;
    b->blockno = i + 1;

    insert_head(&bcache.buckets[hash(b->dev, b->blockno)].head, b);
  }
}

// Return a locked buffer for (dev, blockno).
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *candidate;
  uint target;
  int old;
  int i;

  target = hash(dev, blockno);

  // Fast path: look in the target bucket.
  acquire(&bcache.buckets[target].lock);

  for (b = bcache.buckets[target].head.next;
       b != &bcache.buckets[target].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.buckets[target].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.buckets[target].lock);

  // Serialize cache misses and buffer reassignment.
  acquire(&bcache.evict_lock);

  // Recheck after acquiring eviction lock.
  acquire(&bcache.buckets[target].lock);

  for (b = bcache.buckets[target].head.next;
       b != &bcache.buckets[target].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.buckets[target].lock);
      release(&bcache.evict_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.buckets[target].lock);

  // Find an unused buffer, starting with the LRU end of each bucket.
  for (;;) {
    candidate = 0;
    old = -1;

    for (i = 0; i < NBUCKET; i++) {
      acquire(&bcache.buckets[i].lock);

      for (b = bcache.buckets[i].head.prev;
           b != &bcache.buckets[i].head; b = b->prev) {
        if (b->refcnt == 0) {
          candidate = b;
          old = i;
          break;
        }
      }

      release(&bcache.buckets[i].lock);

      if (candidate != 0)
        break;
    }

    if (candidate == 0) {
      release(&bcache.evict_lock);
      panic("bget: no buffers");
    }

    // Lock both buckets in a fixed order to avoid deadlock.
    if (old < (int)target) {
      acquire(&bcache.buckets[old].lock);
      acquire(&bcache.buckets[target].lock);
    } else if (old > (int)target) {
      acquire(&bcache.buckets[target].lock);
      acquire(&bcache.buckets[old].lock);
    } else {
      acquire(&bcache.buckets[target].lock);
    }

    // A concurrent cache hit may have claimed this buffer.
    if (candidate->refcnt != 0) {
      if (old < (int)target) {
        release(&bcache.buckets[target].lock);
        release(&bcache.buckets[old].lock);
      } else if (old > (int)target) {
        release(&bcache.buckets[old].lock);
        release(&bcache.buckets[target].lock);
      } else {
        release(&bcache.buckets[target].lock);
      }
      continue;
    }

    // Move the buffer to the target bucket.
    remove_buf(candidate);
    candidate->dev = dev;
    candidate->blockno = blockno;
    candidate->valid = 0;
    candidate->refcnt = 1;
    insert_head(&bcache.buckets[target].head, candidate);

    if (old < (int)target) {
      release(&bcache.buckets[target].lock);
      release(&bcache.buckets[old].lock);
    } else if (old > (int)target) {
      release(&bcache.buckets[old].lock);
      release(&bcache.buckets[target].lock);
    } else {
      release(&bcache.buckets[target].lock);
    }

    release(&bcache.evict_lock);

    acquiresleep(&candidate->lock);
    return candidate;
  }
}

struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
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
  uint idx;

  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  idx = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[idx].lock);

  b->refcnt--;

  if (b->refcnt == 0) {
    remove_buf(b);
    insert_head(&bcache.buckets[idx].head, b);
  }

  release(&bcache.buckets[idx].lock);
}

void
bpin(struct buf *b)
{
  uint idx = hash(b->dev, b->blockno);

  acquire(&bcache.buckets[idx].lock);
  b->refcnt++;
  release(&bcache.buckets[idx].lock);
}

void
bunpin(struct buf *b)
{
  uint idx = hash(b->dev, b->blockno);

  acquire(&bcache.buckets[idx].lock);
  b->refcnt--;
  release(&bcache.buckets[idx].lock);
}
