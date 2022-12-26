// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct bufBucket {
  struct spinlock lock;
  struct buf head;
};

struct bufBucket buckets[13];

static struct buf* pool_pop() {
  struct buf *b;
  acquire(&bcache.lock);
  if (bcache.head.next == &bcache.head) {
    panic("no buffers");
  }
  b = bcache.head.next;
  b->next->prev = &bcache.head;
  bcache.head.next = b->next;
  b->next = 0;
  b->prev = 0;
  release(&bcache.lock);
  acquiresleep(&b->lock);
  return b;
}

static void pool_push(struct buf* b) {
    acquire(&bcache.lock);
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
    release(&bcache.lock);
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for (int i = 0; i < 13; i++) {
    initlock(&buckets[i].lock, "bcache");
    buckets[i].head.next = 0;
  }
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % 13;
  acquire(&buckets[id].lock);
  for (b = buckets[id].head.next; b; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&buckets[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  b = pool_pop();
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->next = 0;
  if (buckets[id].head.next) {
    b->next = buckets[id].head.next;
    buckets[id].head.next->prev = b;
  }
  b->prev = &buckets[id].head;
  buckets[id].head.next = b;
  release(&buckets[id].lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  releasesleep(&b->lock);

  int id = b->blockno % 13;

  acquire(&buckets[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->prev->next = b->next;
    if (b->next) {
      b->next->prev = b->prev;
    }
    pool_push(b);
  }
  release(&buckets[id].lock);
}

void
bpin(struct buf *b) {
  int id = b->blockno % 13;
  acquire(&buckets[id].lock);
  b->refcnt++;
  release(&buckets[id].lock);
}

void
bunpin(struct buf *b) {
  int id = b->blockno % 13;
  acquire(&buckets[id].lock);
  b->refcnt--;
  release(&buckets[id].lock);
}


