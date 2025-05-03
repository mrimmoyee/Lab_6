#ifndef _KERN_PROC_PPROC_H_
  #define _KERN_PROC_PPROC_H_

  #ifdef _KERN_

  #include <lib/types.h>
  #include <vmm/MPTOp/export.h> // For pgd_t

  unsigned int proc_create(void *elf_addr, unsigned int quota);
  void proc_start_user(void);
  struct proc *proc_get(unsigned int pid);

  struct proc {
      uint32_t pid; // Process ID
      uint32_t brk; // Heap break
      uint32_t heap_start; // Start of heap (e.g., VM_USERLO)
      pgd_t *pgd; // Page directory
      int superpage_enabled; // 1 if super pages allowed
      int contiguous_enabled; // 1 if contiguous allocations allowed
  };

  #endif /* _KERN_ */

  #endif /* !_KERN_PROC_PPROC_H_ */