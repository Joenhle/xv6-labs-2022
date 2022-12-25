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
  int count;
};

struct kmem kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    initlock(&kmems[i].lock, "kmem");
    kmems[i].count = 0;
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

// Free the page of physical memory pointed at by pa,
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
  int id = cpuid();
  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  kmems[id].count += 1;
  release(&kmems[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  int id = cpuid();
  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if(r) {
    kmems[id].freelist = r->next;
    kmems[id].count -= 1;
  } else {
    int max = 0, max_index = -1;
    for (int i = 0; i < NCPU; i++) {
      if (id == i) {
        continue;
      }
      acquire(&kmems[i].lock);
    }
    for (int i = 0; i < NCPU; i++) {
      if (id == i) {
        continue;
      }
      if (kmems[i].count > max) {
        max = kmems[i].count;
        max_index = i;
      }
    }
    for (int i = 0; i < NCPU; i++) {
      if (i == id || i == max_index) {
        continue;
      }
      release(&kmems[i].lock);
    }
    if (max != 0) {
      struct run *temp = kmems[max_index].freelist;
      int count = 1;
      for (int i = 1; i < kmems[max_index].count / 2; i++) {
        if (temp) {
          temp = temp->next;
          count += 1;
        } else {
          break;
        }
      }
      kmems[id].freelist = kmems[max_index].freelist;
      kmems[id].count = count;
      kmems[max_index].freelist = temp->next;
      kmems[max_index].count -= count;
      temp->next = 0;
      r = kmems[id].freelist;
      kmems[id].freelist = r->next;
      kmems[id].count -= 1;
      release(&kmems[max_index].lock);
    }
  }
  release(&kmems[id].lock);
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
