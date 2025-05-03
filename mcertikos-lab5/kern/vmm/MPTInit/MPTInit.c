#include "import.h"

/**
 * Check if the CPU supports Page Size Extension (PSE) using CPUID.
 * Returns 1 if supported, 0 otherwise.
 */
static int check_pse_support(void) {
    uint32_t edx;
    // CPUID with EAX=1 returns feature flags in EDX
    asm volatile("cpuid" : "=d"(edx) : "a"(1) : "ecx", "ebx");
    return (edx & (1 << 3)) ? 1 : 0; // Bit 3 = PSE support
}

/**
 * Enable PSE in CR4 if supported.
 */
static void enable_pse(void) {
    if (!check_pse_support()) {
        cprintf("PSE not supported by CPU\n");
        panic("Cannot enable super pages");
    }

    // Read CR4
    uint32_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    // Set CR4.PSE (bit 4) to enable 4MB pages
    cr4 |= (1 << 4);
    asm volatile("mov %0, %%cr4" : : "r"(cr4));

    cprintf("PSE enabled for super pages\n");
}

/**
 * Initializes the page structures,
 * move to the page structure # 0 (kernel),
 * and turn on the paging.
 */
void paging_init(unsigned int mbi_addr) {
    pt_spinlock_init();
    pdir_init_kern(mbi_addr);
    
    // Enable PSE before turning on paging
    enable_pse();

    pt_spinlock_acquire();
    set_pdir_base(0);
    enable_paging();
    pt_spinlock_release();
}

/**
 * Initializes paging for application processors (APs).
 */
void paging_init_ap(void) {
    pt_spinlock_acquire();
    set_pdir_base(0);
    enable_paging();
    pt_spinlock_release();
}