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
struct page_ref
{
  struct spinlock lock;
  int page_count[PHYSTOP >> 12];
} page_ref_count;
struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

void kinit()
{
  initlock(&page_ref_count.lock, "page_ref_count");
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    kfree(p);
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

  acquire(&page_ref_count.lock);
  int* p_count = &page_ref_count.page_count[(uint64)pa >> 12];
  if (*p_count > 0)
  {
    (*p_count)--;
    if (*p_count != 0)
    {
      release(&page_ref_count.lock);
      return;
    }
  }
  release(&page_ref_count.lock);
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
  {
    kmem.freelist = r->next;
    acquire(&page_ref_count.lock);
    page_ref_count.page_count[(uint64)r >> 12] = 1;
    release(&page_ref_count.lock);
  }
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
  }

  return (void *)r;
}

int page_ref_increase(uint64 pa)
{
  pa = PGROUNDDOWN(pa);
  if (pa < (uint64)end || pa >= (uint64)PHYSTOP)
  {
    return -1;
  }
  acquire(&page_ref_count.lock);
  page_ref_count.page_count[pa >> 12]++;
  release(&page_ref_count.lock);
  return 0;
}
int is_cow_page(pagetable_t pagetable, uint64 va)
{
  if (va >= MAXVA)
  {
    return -1;
  }
  pte_t *pte;
  if ((pte = walk(pagetable, va, 0)) == 0)
  {
    return -1;
  }
  if (*pte & PTE_V && *pte & PTE_COW)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}
uint64 cow_alloc(pagetable_t pagetable, uint64 va)
{
  va = PGROUNDDOWN(va);
  uint64 pa;
  if ((pa = walkaddr(pagetable, va)) == 0)
  {
    return 0;
  }
  pte_t *pte = walk(pagetable, va, 0);
  if (page_ref_count.page_count[pa >> 12] == 1)
  {
    *pte = (*pte | PTE_W) & (~PTE_COW);
    return pa;
  }
  else
  {
    void *mem;
    if ((mem = kalloc()) == 0)
    {
      return 0;
    }
    memmove(mem, (void *)pa, PGSIZE);
    *pte = *pte & (~PTE_V);
    if (mappages(pagetable, va, PGSIZE, (uint64)mem, (PTE_FLAGS(*pte) | PTE_W) & (~PTE_COW)) != 0)
    {
      kfree(mem);
      *pte = *pte | PTE_V;
      return 0;
    }
    kfree((void *)pa);
    return (uint64)mem;
  }
}
