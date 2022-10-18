// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
} kmem;

//每个cpu各有一个内存链表
struct kmem kmemArray[NCPU];

//初始化
void
kinit()
{
    for(int i=0;i<NCPU;i++){
        initlock(&kmemArray[i].lock,"kmem");
    }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  //关闭中断
  push_off();
  int id = cpuid();
  pop_off();
  
  //头插法
  acquire(&kmemArray[id].lock);
  r->next = kmemArray[id].freelist;
  kmemArray[id].freelist = r;
  release(&kmemArray[id].lock);
}

//偷取其他cpu的空余内存
void *
steal(int cpuid)
{
    struct run *r = kmemArray[cpuid].freelist;
    int i;
    for(i=0;i<NCPU;i++){
        if(i!=cpuid){
            acquire(&kmemArray[i].lock);       
            if(kmemArray[i].freelist){
                r = kmemArray[i].freelist;
                kmemArray[i].freelist=r->next;
                break;
            }
            else{
                release(&kmemArray[i].lock);
            }
        }
    }
    //如果成功偷取，应释放对应锁
    if(r){
        release(&kmemArray[i].lock);
    }
    return (void*)r;
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmemArray[id].lock);
  r = kmemArray[id].freelist;
  if(r)
    kmemArray[id].freelist = r->next;
  //否则说明当前cpu的内存用尽，应偷取
  else
    r = steal(id);
  release(&kmemArray[id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
