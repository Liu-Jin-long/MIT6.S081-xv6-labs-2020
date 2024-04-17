void vmprint(pagetable_t pt)
{
  void _vmprint(pagetable_t pagetable, int level);
  printf("page table %p\n", pt);
  _vmprint(pt, 1);
  // printf("page table %p\n", pt);
  // char *prefix_2 = " ..";
  // char *prefix_1 = " .. ..";
  // char *prefix_0 = " .. .. ..";
  // for (int i = 0; i < 512; i++)
  // {
  //   pte_t pte_2 = pt[i];
  //   if (pte_2 & PTE_V)
  //   {
  //     printf("%s%d: pte %p pa %p\n", prefix_2, i, pte_2, PTE2PA(pte_2));
  //     pagetable_t pt_1 = (pagetable_t)PTE2PA(pte_2);
  //     if ((pte_2 & (PTE_R | PTE_W | PTE_X)) == 0)
  //     {
  //       for (int j = 0; j < 512; j++)
  //       {
  //         pte_t pte_1 = pt_1[j];
  //         if (pte_1 & PTE_V)
  //         {
  //           printf("%s%d: pte %p pa %p\n", prefix_1, j, pte_1, PTE2PA(pte_1));
  //           pagetable_t pt_0 = (pagetable_t)PTE2PA(pte_1);
  //           if ((pte_1 & (PTE_R | PTE_W | PTE_X)) == 0)
  //           {
  //             for (int k = 0; k < 512; k++)
  //             {
  //               pte_t pte_0 = pt_0[k];
  //               if (pte_0 & PTE_V)
  //               {
  //                 printf("%s%d: pte %p pa %p\n", prefix_0, k, pte_0, PTE2PA(pte_0));
  //               }
  //             }
  //           }
  //         }
  //       }
  //     }
  //   }
  // }
}
void _vmprint(pagetable_t pagetable, int level)
{
  for (int i = 0; i < 512; i++)
  {
    pte_t pte = pagetable[i];
    // PTE_V is a flag for whether the page table is valid
    if (pte & PTE_V)
    {
      for (int j = 0; j < level; j++)
      {
        if (j)
          printf(" ");
        printf("..");
      }
      uint64 child = PTE2PA(pte);
      printf("%d: pte %p pa %p\n", i, pte, child);
      if ((pte & (PTE_R | PTE_W | PTE_X)) == 0)
      {
        // this PTE points to a lower-level page table.
        _vmprint((pagetable_t)child, level + 1);
      }
    }
  }
}
void free_proc_kernel_pagetable(pagetable_t kernel_pagetable)
{
  for (int i = 0; i < 512; i++)
  {
    pte_t pte = kernel_pagetable[i];
    if (pte & PTE_V)
    {
      kernel_pagetable[i] = 0;
      if ((pte & (PTE_R | PTE_W | PTE_X)) == 0)
      {
        free_proc_kernel_pagetable((pagetable_t)PTE2PA(pte));
      }
    }
  }
  kfree(kernel_pagetable);
}
void user2kernel_vmcopy(pagetable_t pagetable, pagetable_t kernel_pagetable, uint64 oldsz, uint64 newsz)
{
  pte_t *pte_from, *pte_to;
  oldsz = PGROUNDUP(oldsz);
  for (uint64 i = oldsz; i < newsz; i += PGSIZE)
  {
    if ((pte_from = walk(pagetable, i, 0)) == 0)
    {
      panic("user2kernel_vmcopy: src pte does not exist");
    }
    if ((pte_to = walk(kernel_pagetable, i, 1)) == 0)
    {
      panic("user2kernel_vmcopy: pte walk failed");
    }
    uint64 pa = PTE2PA(*pte_from);
    uint flag = (PTE_FLAGS(*pte_from)) & (~PTE_U);
    *pte_to = PA2PTE(pa) | flag;
  }
}


pagetable_t proc_kernel_pagetable_init()
{
  pagetable_t proc_kernel_pagetable = (pagetable_t)kalloc();
  memset(proc_kernel_pagetable, 0, PGSIZE);

  // uart registers
  proc_kvmmap(proc_kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  proc_kvmmap(proc_kernel_pagetable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // CLINT
  proc_kvmmap(proc_kernel_pagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);

  // PLIC
  proc_kvmmap(proc_kernel_pagetable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  proc_kvmmap(proc_kernel_pagetable, KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  proc_kvmmap(proc_kernel_pagetable, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  proc_kvmmap(proc_kernel_pagetable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
  return proc_kernel_pagetable;
}
void proc_kvmmap(pagetable_t proc_kernel_pagetable, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if (mappages(proc_kernel_pagetable, va, sz, pa, perm) != 0)
    panic("proc_kvmmap");
}