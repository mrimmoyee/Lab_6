#include <lib/x86.h>

#include "import.h"

/**
 * For each process from id 0 to NUM_IDS -1,
 * set the page directory entries sothat the kernel portion of the map as identity map,
 * and the rest of page directories are unmmaped.
 */
void pdir_init(unsigned int mbi_adr)
{
    unsigned int i, j;
    idptbl_init(mbi_adr);

    // TODO
    i = 0;
    while(i < NUM_IDS)
    {
        j = 0;
        while(j < 1024)    
        {
            if (j < 256 || j >= 960)    
              set_pdir_entry_identity(i, j);
            else
              rmv_pdir_entry(i, j);
            j++;
        }
        i++;
    }
}
int map_super_page(uint32_t virt_addr, uint32_t phys_addr, pgd_t *pgd) {
  // Validate 4MB alignment
  if (virt_addr & 0x3FFFFF || phys_addr & 0x3FFFFF) {
      cprintf("map_super_page: Invalid alignment - virt: 0x%x, phys: 0x%x\n", virt_addr, phys_addr);
      return -1; // EINVAL
  }

  // Get page directory entry
  pgd_t *pde = pgd_offset(pgd, virt_addr);
  if (!pde) {
      cprintf("map_super_page: Invalid PDE for virt_addr 0x%x\n", virt_addr);
      return -1;
  }

  // Set PDE for 4MB page (PS bit enables super page)
  pde->val = (phys_addr & 0xFFC00000) | PG_PRESENT | PG_WRITE | PG_PS;

  // Flush TLB for the virtual address
  asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");

  cprintf("Mapped super page: virt 0x%x -> phys 0x%x\n", virt_addr, phys_addr);
  return 0;
}

/**
 * Allocates a page (with container_alloc) for the page table,
 * and registers it in page directory for the given virtual address,
 * and clears (set to 0) the whole page table entries for this newly mapped page table.
 * It returns the page index of the newly allocated physical page.
 * In the case when there's no physical page available, it returns 0.
 */
unsigned int alloc_ptbl(unsigned int proc_index, unsigned int vadr)
{
  // TODO
  unsigned int i;
  unsigned int pi;
  unsigned int pde_index;
  pi = container_alloc(proc_index);
  if (pi != 0)
  {
    set_pdir_entry_by_va(proc_index, vadr, pi);
    pde_index = vadr / (4096 * 1024);
    i = 0;
    while (i < 1024)        
    {
      rmv_ptbl_entry(proc_index, pde_index, i);
      i ++;
    }     
  }       
  return pi;
}

// Reverse operation of alloc_ptbl.
// Removes corresponding page directory entry,
// and frees the page for the page table entries (with container_free).
void free_ptbl(unsigned int proc_index, unsigned int vadr)
{
  // TODO
  unsigned int pde;
  pde = get_pdir_entry_by_va(proc_index, vadr);
  rmv_pdir_entry_by_va(proc_index, vadr);
  container_free(proc_index, pde / PAGESIZE);
}
