#include <lib/debug.h>
#include "import.h"
#include "../pmm.h"

#define PAGESIZE	4096
#define VM_USERLO	0x40000000
#define VM_USERHI	0xF0000000
#define VM_USERLO_PI	(VM_USERLO / PAGESIZE)
#define VM_USERHI_PI	(VM_USERHI / PAGESIZE)

void
pmem_init(unsigned int mbi_addr)
{
  unsigned int nps;

  // Define your local variables here.
  unsigned int i, j, isnorm, maxs, size, flag;
  unsigned int s, l;

  // Calls the lower layer initialization primitives.
  // The parameter mbi_addr shall not be used in the further code.
  devinit(mbi_addr);

  mem_spinlock_init();
  /**
   * Calculate the number of actual number of available physical pages and store it into the local variable nps.
   * Hint: Think of it as the highest address possible in the ranges of the memory map table,
   *       divided by the page size.
   */
  i = 0;
  size = get_size();
  nps = 0;
  while (i < size) {
    s = get_mms(i);
    l = get_mml(i);
    maxs = (s + l) / PAGESIZE + 1;
    if (maxs > nps)
      nps = maxs;
    i++;
  }

  set_nps(nps); // Setting the value computed above to NUM_PAGES.

  /**
   * Initialization of the physical allocation table (AT).
   *
   * In CertiKOS, the entire addresses < VM_USERLO or >= VM_USERHI are reserved by the kernel.
   * That corresponds to the physical pages from 0 to VM_USERLO_PI-1, and from VM_USERHI_PI to NUM_PAGES-1.
   * The rest of pages that correspond to addresses [VM_USERLO, VM_USERHI), can be used freely ONLY IF
   * the entire page falls into one of the ranges in the memory map table with the permission marked as usable.
   */
  i = 0;
  while (i < nps) {
    if (i < VM_USERLO_PI || i >= VM_USERHI_PI) {
      at_set_perm(i, 1);
    } else {
      j = 0;
      flag = 0;
      isnorm = 0;
      while (j < size && flag == 0) {
        s = get_mms(j);
        l = get_mml(j);
        isnorm = is_usable(j);
        if (s <= i * PAGESIZE && l + s >= (i + 1) * PAGESIZE) {
          flag = 1;
        }
        j++;
      }
      if (flag == 1 && isnorm == 1)
        at_set_perm(i, 2);
      else
        at_set_perm(i, 0);
    }
    i++;
  }
}
