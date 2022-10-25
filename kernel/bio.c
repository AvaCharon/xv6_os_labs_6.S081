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
  //struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
  struct buf buckets[NBUFBUCKETS];
  struct spinlock bucketslock[NBUFBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<NBUFBUCKETS;i++){
      initlock(&bcache.bucketslock[i],"bcachebucket");
      bcache.buckets[i].prev = &bcache.buckets[i];
      bcache.buckets[i].next = &bcache.buckets[i];
  }
  // Create linked list of buffers
  int hashid;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    hashid = b->blockno%NBUFBUCKETS;
    b->next = bcache.buckets[hashid].next;
    b->prev = &bcache.buckets[hashid];
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[hashid].next->prev = b;
    bcache.buckets[hashid].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int hashid = blockno%NBUFBUCKETS;

  acquire(&bcache.bucketslock[hashid]);

  // Is the block already cached?
  for(b = bcache.buckets[hashid].next; b != &bcache.buckets[hashid]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketslock[hashid]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  for(b = bcache.buckets[hashid].prev; b != &bcache.buckets[hashid]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucketslock[hashid]);
      acquiresleep(&b->lock);
      return b;
    }
}
    
	
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 从其他哈希桶里取
  for(int i=0;i<NBUFBUCKETS;i++){
      if(i!=hashid){
          acquire(&bcache.bucketslock[i]);
          for(b = bcache.buckets[i].prev;b != &bcache.buckets[i];b = b->prev){
              if(b->refcnt == 0){
                  b->dev = dev;
                  b->blockno = blockno;
                  b->valid = 0;
                  b->refcnt = 1;

                  b->next->prev = b->prev;
                  b->prev->next = b->next;

                  b->next = bcache.buckets[hashid].next;
                  b->prev = &bcache.buckets[hashid];
                  bcache.buckets[hashid].next->prev = b;
                  bcache.buckets[hashid].next = b;

                  release(&bcache.bucketslock[i]);
                  release(&bcache.bucketslock[hashid]);
                  acquiresleep(&b->lock);
                  return b;
              }
          }
          release(&bcache.bucketslock[i]);
      }
  }
  panic("bget: no buffers");
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

  int hashid = b->blockno%NBUFBUCKETS;

  acquire(&bcache.bucketslock[hashid]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;

    b->next = bcache.buckets[hashid].next;
    b->prev = &bcache.buckets[hashid];
    bcache.buckets[hashid].next->prev = b;
    bcache.buckets[hashid].next = b;
  }
  
  release(&bcache.bucketslock[hashid]);
}

void
bpin(struct buf *b) {
  int hashid = b->blockno%NBUFBUCKETS;
  acquire(&bcache.bucketslock[hashid]);
  b->refcnt++;
  release(&bcache.bucketslock[hashid]);
}

void
bunpin(struct buf *b) {
  int hashid = b->blockno%NBUFBUCKETS;
  acquire(&bcache.bucketslock[hashid]);
  b->refcnt--;
  release(&bcache.bucketslock[hashid]);
}


