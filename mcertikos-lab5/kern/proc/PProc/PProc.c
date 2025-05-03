#include "lib/elf.h"
#include <stddef.h> // For NULL definition

// Define FL_IF if not already defined
#ifndef FL_IF
#define FL_IF 0x00000200
#endif

// Define TRUE if not already defined
#ifndef TRUE
#define TRUE 1
#endif

// Define NUM_IDS with an appropriate value
#define NUM_IDS 128
  #include "lib/debug.h"
  #include <lib/gcc.h>
  #include <lib/seg.h>
  #include "lib/trap.h"

  #ifndef CPU_GDT_UDATA
  #define CPU_GDT_UDATA 0x10 // Example value for user data segment
  #endif
  #include <lib/x86.h>
  #include "pcpu/PCPUIntro/export.h"
  #include "vmm/MPTOp/export.h" // For pgd_t

  #include "import.h"
#include "proc.h" // For struct proc

// Define struct proc if not already defined
#ifndef PROC_STRUCT_DEFINED
#define PROC_STRUCT_DEFINED
struct proc {
    unsigned int pid;
    unsigned int brk;
    unsigned int heap_start;
    void *pgd; // Assuming pgd_t is a pointer type
    int superpage_enabled;
    int contiguous_enabled;
};
#endif
  

// Define tf_t if not already defined
#ifndef TF_T_DEFINED
#define TF_T_DEFINED
typedef struct {
    unsigned int eip;
    unsigned int esp;
    unsigned int eflags;
    unsigned int cs;
    unsigned int ds;
    unsigned int es;
    unsigned int ss;
} tf_t;
#endif

  extern tf_t uctx_pool[NUM_IDS];
  static struct proc proc_table[NUM_IDS]; // Process table

  void proc_start_user(void)
  {
      unsigned int cur_pid = get_curid();
      static int first = TRUE;
      kstack_switch(cur_pid);
      set_pdir_base(cur_pid);

      trap_return((void *) &uctx_pool[cur_pid]);
  }

  unsigned int proc_create(void *elf_addr, unsigned int quota)
  {
      unsigned int pid, id;

      id = get_curid();
      pid = thread_spawn((void *) proc_start_user, id, quota);

      if (pid == NUM_IDS) {
          KERN_DEBUG("proc_create: thread_spawn failed\n");
          return NUM_IDS;
      }

      elf_load(elf_addr, pid);
      uctx_pool[pid].es = CPU_GDT_UDATA | 3; // Ensure CPU_GDT_UDATA is defined in the included headers
      uctx_pool[pid].es = CPU_GDT_UDATA | 3;
      uctx_pool[pid].ds = CPU_GDT_UDATA | 3;
    #ifndef CPU_GDT_UCODE
    #define CPU_GDT_UCODE 0x08 // Example value for user code segment
    #endif
          uctx_pool[pid].cs = CPU_GDT_UCODE | 3;
      uctx_pool[pid].ss = CPU_GDT_UDATA | 3;
      #ifndef VM_USERHI
      #define VM_USERHI 0xBFFFFFFF // Example value for the top of user space
      #endif
      uctx_pool[pid].esp = VM_USERHI;
      uctx_pool[pid].eflags = FL_IF;
      uctx_pool[pid].eip = elf_entry(elf_addr);

      seg_init_proc(get_pcpu_idx(), pid);

      // Initialize proc_table entry
      proc_table[pid].pid = pid;
    #ifndef VM_USERLO
    #define VM_USERLO 0x40000000 // Example value for the start of user space
    #endif
          proc_table[pid].brk = VM_USERLO; // Example heap start
      proc_table[pid].heap_start = VM_USERLO;
      proc_table[pid].pgd = get_pdir(pid); // Assuming get_pdir exists
      proc_table[pid].superpage_enabled = 0;
      proc_table[pid].contiguous_enabled = 0;

      return pid;
  }

  struct proc *proc_get(unsigned int pid)
  {
      if (pid >= NUM_IDS) {
          KERN_DEBUG("proc_get: invalid pid %d\n", pid);
          return NULL;
      }
      return &proc_table[pid];
  }