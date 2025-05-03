#include <lib/debug.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/trap.h>
#include <lib/syscall.h>
#include <dev/intr.h>
#include <lib/ipc.h>
#include <pcpu/PCPUIntro/export.h>
#include <pmm/pmm.h>          
#include <mm/vmm.h>           
#include <proc/PProc/export.h>
#include <vmm/MPTComm/export.h> // For map_super_page, map_page, etc.

#include "import.h"

#include "import.h"

extern struct MsgBlock msgBlock[NUM_IDS];
extern spinlock_t msg_lock;
extern void sys_brk(tf_t *tf);
void *syscall_table[] = {
    // Existing entries
    [SYS_puts] = sys_puts,
    [SYS_spawn] = sys_spawn,
    [SYS_yield] = sys_yield,
    [SYS_brk] = sys_brk, // Add
};
// Existing syscalls (unchanged)
void sys_sync_send(tf_t *tf){
   unsigned int cur_pid;
   unsigned int recv_pid, user_addr, length;
   spinlock_acquire(&msg_lock); 
   recv_pid = syscall_get_arg2(tf);
   user_addr = syscall_get_arg3(tf);
   length = syscall_get_arg4(tf);
   cur_pid = get_curid();
   msgBlock[cur_pid].recv_pid = recv_pid; 
   msgBlock[cur_pid].buffer_addr = user_addr;
   msgBlock[cur_pid].length = length;
   msg_enqueue(cur_pid);
   thread_wakeup(&msgBlock[cur_pid].send_cv); 
   while(msg_getBlockBySendID(cur_pid) != NUM_IDS){
      thread_sleep(&msgBlock[cur_pid].recv_cv, &msg_lock);
   } 
   syscall_set_errno(tf, E_SUCC);
   spinlock_release(&msg_lock); 
}

void sys_sync_recv(tf_t *tf){
   unsigned int cur_pid;
   unsigned int send_pid, user_recv_addr, recv_length, send_length, copy_length, user_send_addr;
   spinlock_acquire(&msg_lock); 
   
   send_pid = syscall_get_arg2(tf);
   user_recv_addr = syscall_get_arg3(tf);
   recv_length = syscall_get_arg4(tf);
   cur_pid = get_curid();
   
   while(msg_getBlockBySendID(send_pid) == NUM_IDS || msgBlock[send_pid].recv_pid != cur_pid){
      thread_sleep(&msgBlock[send_pid].send_cv, &msg_lock);
   }
   user_send_addr = msgBlock[send_pid].buffer_addr;
   send_length = msgBlock[send_pid].length;

   copy_length = send_length < recv_length? send_length: recv_length;
   ipc_copy(cur_pid, user_recv_addr, send_pid, user_send_addr, copy_length);
   msg_remove(send_pid);
   thread_wakeup(&msgBlock[send_pid].recv_cv);
   syscall_set_errno(tf, E_SUCC);
   syscall_set_retval1(tf, copy_length);
   spinlock_release(&msg_lock); 
}

static char sys_buf[NUM_IDS][PAGESIZE];

void sys_puts(tf_t *tf)
{
  unsigned int cur_pid;
  unsigned int str_uva, str_len;
  unsigned int remain, cur_pos, nbytes;

  cur_pid = get_curid();
  str_uva = syscall_get_arg2(tf);
  str_len = syscall_get_arg3(tf);

  if (!(VM_USERLO <= str_uva && str_uva + str_len <= VM_USERHI)) {
    syscall_set_errno(tf, E_INVAL_ADDR);
    return;
  }

  remain = str_len;
  cur_pos = str_uva;

  while (remain) {
    if (remain < PAGESIZE - 1)
      nbytes = remain;
    else
      nbytes = PAGESIZE - 1;

    if (pt_copyin(cur_pid,
		  cur_pos, sys_buf[cur_pid], nbytes) != nbytes) {
      syscall_set_errno(tf, E_MEM);
      return;
    }

    sys_buf[cur_pid][nbytes] = '\0';
    KERN_INFO("%s", sys_buf[cur_pid]);

    remain -= nbytes;
    cur_pos += nbytes;
  }

  syscall_set_errno(tf, E_SUCC);
}

extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];
extern uint8_t _binary___obj_user_pingpong_ding_start[];
extern uint8_t _binary___obj_user_fstest_fstest_start[];

void sys_spawn(tf_t *tf)
{
  unsigned int new_pid;
  unsigned int elf_id, quota;
  void *elf_addr;
  unsigned int qok, nc, curid;

  elf_id = syscall_get_arg2(tf);
  quota = syscall_get_arg3(tf);

  qok = container_can_consume(curid, quota);
  nc = container_get_nchildren(curid);
  curid = get_curid();
  if (qok == 0) {
    syscall_set_errno(tf, E_EXCEEDS_QUOTA);
    syscall_set_retval1(tf, NUM_IDS);
    return;
  }
  else if (NUM_IDS < curid * MAX_CHILDREN + 1 + MAX_CHILDREN) {
    syscall_set_errno(tf, E_MAX_NUM_CHILDEN_REACHED);
    syscall_set_retval1(tf, NUM_IDS);
    return;
  }
  else if (nc == MAX_CHILDREN) {
    syscall_set_errno(tf, E_INVAL_CHILD_ID);
    syscall_set_retval1(tf, NUM_IDS);
    return;
  }

  if (elf_id == 1) {
    elf_addr = _binary___obj_user_pingpong_ping_start;
  } else if (elf_id == 2) {
    elf_addr = _binary___obj_user_pingpong_pong_start;
  } else if (elf_id == 3) {
    elf_addr = _binary___obj_user_pingpong_ding_start;
  } else if (elf_id == 4) {
    elf_addr = _binary___obj_user_fstest_fstest_start;
  } else {
    syscall_set_errno(tf, E_INVAL_PID);
    syscall_set_retval1(tf, NUM_IDS);
    return;
  }

  new_pid = proc_create(elf_addr, quota);

  if (new_pid == NUM_IDS) {
    syscall_set_errno(tf, E_INVAL_PID);
    syscall_set_retval1(tf, NUM_IDS);
  } else {
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, new_pid);
  }
}

void sys_yield(tf_t *tf)
{
  thread_yield();
  syscall_set_errno(tf, E_SUCC);
}

void sys_produce(tf_t *tf)
{
  unsigned int i;
  for(i = 0; i < 5; i++) {
    intr_local_disable();
    KERN_DEBUG("CPU %d: Process %d: Produced %d\n", get_pcpu_idx(), get_curid(), i);
    intr_local_enable();
  }
  syscall_set_errno(tf, E_SUCC);
}

void sys_consume(tf_t *tf)
{
  unsigned int i;
  for(i = 0; i < 5; i++) {
    intr_local_disable();
    KERN_DEBUG("CPU %d: Process %d: Consumed %d\n", get_pcpu_idx(), get_curid(), i);
    intr_local_enable();
  }
  syscall_set_errno(tf, E_SUCC);
}

// New sys_brk for Project 10
#define BRK_CONTIGUOUS 0x1
#define BRK_SUPERPAGE  0x2

void sys_brk(tf_t *tf)
{
    unsigned int pid = get_curid();
    struct proc *curproc = proc_get(pid);
    uint32_t addr = syscall_get_arg2(tf);
    size_t n_pages = syscall_get_arg3(tf);
    int flags = syscall_get_arg4(tf);
    uint32_t old_brk = curproc->brk;

    KERN_DEBUG("sys_brk: pid=%d, addr=0x%08x, n_pages=%d, flags=0x%x\n", pid, addr, n_pages, flags);

    if (addr < curproc->heap_start || addr > VM_USERHI) {
        KERN_DEBUG("Invalid brk address: 0x%08x\n", addr);
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    curproc->superpage_enabled = (flags & BRK_SUPERPAGE) ? 1 : 0;
    curproc->contiguous_enabled = (flags & BRK_CONTIGUOUS) ? 1 : 0;

    if (addr > old_brk) {
        size_t pages_needed = (addr - old_brk + PAGE_SIZE - 1) / PAGE_SIZE;
        if (curproc->superpage_enabled) {
            if (pages_needed != 1024 || addr & 0x3FFFFF) {
                KERN_DEBUG("Invalid super page request: addr=0x%08x, pages=%d\n", addr, pages_needed);
                syscall_set_retval1(tf, -1);
                syscall_set_errno(tf, E_INVAL_ADDR);
                return;
            }
            struct page *page = alloc_super_page();
            if (!page) {
                KERN_DEBUG("Super page allocation failed\n");
                syscall_set_retval1(tf, -1);
                syscall_set_errno(tf, E_NOMEM);
                return;
            }
            uint32_t phys_addr = page_to_phys(page);
            if (map_super_page(old_brk, phys_addr, curproc->pgd) < 0) {
                free_pages(page, MAX_ORDER);
                KERN_DEBUG("Super page mapping failed\n");
                syscall_set_retval1(tf, -1);
                syscall_set_errno(tf, E_NOMEM);
                return;
            }
        } else {
            unsigned int order = 0;
            if (curproc->contiguous_enabled && pages_needed > 1) {
                order = ilog2(pages_needed);
                if ((1 << order) < pages_needed)
                    order++;
            }
            struct page *page = alloc_pages(order);
            if (!page) {
                KERN_DEBUG("Page allocation failed\n");
                syscall_set_retval1(tf, -1);
                syscall_set_errno(tf, E_NOMEM);
                return;
            }
            uint32_t phys_addr = page_to_phys(page);
            for (uint32_t va = old_brk; va < addr; va += PAGE_SIZE) {
                if (map_page(va, phys_addr, curproc->pgd, PTE_W | PTE_U | PTE_P) == MagicNumber) {
                    free_pages(page, order);
                    KERN_DEBUG("Page mapping failed\n");
                    syscall_set_retval1(tf, -1);
                    syscall_set_errno(tf, E_NOMEM);
                    return;
                }
                phys_addr += PAGE_SIZE;
            }
        }
        curproc->brk = addr;
    } else if (addr < old_brk) {
        for (uint32_t va = addr; va < old_brk; va += PAGE_SIZE) {
            uint32_t phys_addr = lookup_phys_addr(curproc->pgd, va);
            if (phys_addr) {
                unmap_page(va, curproc->pgd);
                free_pages(phys_to_page(phys_addr), 0);
            }
        }
        curproc->brk = addr;
    }

    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
}