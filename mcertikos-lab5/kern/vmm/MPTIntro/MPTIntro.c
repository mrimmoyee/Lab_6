#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>
#include <lib/spinlock.h>

#include "import.h"

#define PT_PERM_UP 0
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)


static spinlock_t pt_lk;
/**
 * Page directory pool for NUM_IDS processes.
 * mCertiKOS maintains one page structure for each process.
 * Each PDirPool[index] represents the page directory of the page structure for the process # [index].
 * In mCertiKOS, we statically allocate page directories, and maintain the second level page tables dynamically.
 */
char * PDirPool[NUM_IDS][1024] gcc_aligned(PAGESIZE);

/**
 * In mCertiKOS, we use identity page table mappings for the kernel memory.
 * IDPTbl is a statically allocated, identity page table that will be reused for
 * all the kernel memory.
 * Every page directory entry of kernel memory links to corresponding entry in IDPTbl.
 */
unsigned int IDPTbl[1024][1024] gcc_aligned(PAGESIZE);



void pt_spinlock_init(){
	spinlock_init(&pt_lk);
}

void pt_spinlock_acquire(){
	spinlock_acquire(&pt_lk);
}

void pt_spinlock_release(){
	spinlock_release(&pt_lk);
}

// sets the CR3 register with the start address of the page structure for process # [index]
void set_pdir_base(unsigned int index)
{
	  set_cr3(PDirPool[index]);
}

// returns the page directory entry # [pde_index] of the process # [proc_index]
// this can be used to test whether the page directory entry is mapped
unsigned int get_pdir_entry(unsigned int proc_index, unsigned int pde_index)
{
    unsigned int pde;
    pde = (unsigned int)PDirPool[proc_index][pde_index];
    return pde;
}   

// sets specified page directory entry with the start address of physical page # [page_index].
// you should also set the permissions PTE_P, PTE_W, and PTE_U
void set_pdir_entry(unsigned int proc_index, unsigned int pde_index, unsigned int page_index)
{
    PDirPool[proc_index][pde_index] = (char *)(page_index * PAGESIZE + PT_PERM_PTU);
}   

// sets the page directory entry # [pde_index] for the process # [proc_index]
// with the initial address of page directory # [pde_index] in IDPTbl
// you should also set the permissions PTE_P, PTE_W, and PTE_U
// this will be used to map the page directory entry to identity page table.
void set_pdir_entry_identity(unsigned int proc_index, unsigned int pde_index)
{   
    PDirPool[proc_index][pde_index] = ((char *)(IDPTbl[pde_index])) + PT_PERM_PTU;
}   

// removes specified page directory entry (set the page directory entry to 0).
// don't forget to cast the value to (char *).
void rmv_pdir_entry(unsigned int proc_index, unsigned int pde_index)
{
    PDirPool[proc_index][pde_index] = (char *)PT_PERM_UP;
}   

// returns the specified page table entry.
// do not forget that the permission info is also stored in the page directory entries.
unsigned int get_ptbl_entry(unsigned int proc_index, unsigned int pde_index, unsigned int pte_index)
{   
    unsigned int addr;
    addr = ((unsigned int)PDirPool[proc_index][pde_index] & 0xfffff000) + pte_index * 4;
    return *((unsigned int *) addr);
}

// sets specified page table entry with the start address of physical page # [page_index]
// you should also set the given permission
void set_ptbl_entry(unsigned int proc_index, unsigned int pde_index, unsigned int pte_index, unsigned int page_index, unsigned int perm)
{   
    unsigned int addr;
    addr = ((unsigned int)PDirPool[proc_index][pde_index] & 0xfffff000) + pte_index * 4;
    *((unsigned int *) addr) = page_index * PAGESIZE + perm;
}   

// sets the specified page table entry in IDPTbl as the identity map.
// you should also set the given permission
void set_ptbl_entry_identity(unsigned int pde_index, unsigned int pte_index, unsigned int perm)
{
    IDPTbl[pde_index][pte_index] = (pde_index * 1024 + pte_index) * PAGESIZE + perm;
}

// sets the specified page table entry to 0
void rmv_ptbl_entry(unsigned int proc_index, unsigned int pde_index, unsigned int pte_index)
{
    unsigned int addr;
    addr = ((unsigned int)PDirPool[proc_index][pde_index] & 0xfffff000) + pte_index * 4;
    *((unsigned int *) addr) = 0;
}

void page_fault_handler(struct trapframe *tf) {
    // Get faulting address from CR2
    uint32_t fault_addr = rcr2();
    struct proc *curproc = current_proc();
    uint32_t aligned_addr = fault_addr & ~0x3FFFFF; // Align to 4MB boundary

    // Debug log
    cprintf("Page fault at addr 0x%x, proc %d\n", fault_addr, curproc->pid);

    // Check if fault is in heap and super pages are enabled
    if (fault_addr >= curproc->heap_start && fault_addr < curproc->brk &&
        curproc->superpage_enabled && (fault_addr == aligned_addr)) {
        // Allocate a 4MB super page (1024 * 4KB pages)
        struct page *page = alloc_super_page(); // From pmm.c (buddy allocator)
        if (!page) {
            cprintf("Super page allocation failed for addr 0x%x\n", fault_addr);
            kill_proc(curproc);
            return;
        }

        uint32_t phys_addr = page_to_phys(page);
        // Map the 4MB page
        if (map_super_page(aligned_addr, phys_addr, curproc->pgd) < 0) {
            cprintf("Super page mapping failed for addr 0x%x\n", aligned_addr);
            free_pages(page, MAX_ORDER); // Free on failure
            kill_proc(curproc);
            return;
        }
    } else {
        // Existing 4KB page fault handling (from Lab 5)
        struct page *page = alloc_pages(0); // Single page
        if (!page) {
            cprintf("Page allocation failed for addr 0x%x\n", fault_addr);
            kill_proc(curproc);
            return;
        }

        uint32_t phys_addr = page_to_phys(page);
        // Map 4KB page (likely an existing function in MPTComm)
        map_page(fault_addr & ~0xFFF, phys_addr, curproc->pgd, PG_PRESENT | PG_WRITE);
    }
}