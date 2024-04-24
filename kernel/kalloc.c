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

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem, cpu_mem[NCPU];

void kinit()
{
  initlock(&kmem.lock, "kmem");
  char buf[12] = {0};
  for (int i = 0; i < NCPU; i++)
  {
    snprintf(buf, sizeof(buf), "kmem_cpu%d", i);
    initlock(&cpu_mem[i].lock, buf);
  }
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  int cpu_id;
  struct run *r;
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    cpu_id = ((uint64)p >> 12) % NCPU;
    memset(p, 1, PGSIZE);
    r = (struct run *)p;
    // acquire(&cpu_mem[cpu_id].lock);
    r->next = cpu_mem[cpu_id].freelist;
    cpu_mem[cpu_id].freelist = r;
    // release(&cpu_mem[cpu_id].lock);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{

  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;
  push_off();
  int cpu_id = cpuid();

  acquire(&cpu_mem[cpu_id].lock);
  r->next = cpu_mem[cpu_id].freelist;
  cpu_mem[cpu_id].freelist = r;
  release(&cpu_mem[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  int cpu_id = cpuid();

  struct run *r;

  acquire(&cpu_mem[cpu_id].lock);
  r = cpu_mem[cpu_id].freelist;
  if (r)
  {
    cpu_mem[cpu_id].freelist = r->next;
  }
  else
  {
    for (int i = 0; i < NCPU; i++)
    {
      if (i == cpu_id)
      {
        continue;
      }
      acquire(&cpu_mem[i].lock);
      if (cpu_mem[i].freelist)
      {
        r = cpu_mem[i].freelist;
        cpu_mem[i].freelist = r->next;
        release(&cpu_mem[i].lock);
        break;
      }
      release(&cpu_mem[i].lock);
    }
  }
  release(&cpu_mem[cpu_id].lock);
  pop_off();
  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
  }

  return (void *)r;
}
