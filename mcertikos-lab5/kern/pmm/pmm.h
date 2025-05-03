#ifndef KERN_PMM_H
#define KERN_PMM_H

#ifdef KERN

#include <lib/types.h>
#include <lib/list.h>

#define PAGE_SIZE    4096          // 4KB page size
#define SUPER_PAGE_SIZE (PAGE_SIZE << 10) // 4MB (2^10 pages)
#define MAX_ORDER    10            // Max order for 4MB (2^10 pages)
#define MB           (1 << 20)     // 1MB in bytes

// Page allocation flags
#define PMM_CONTIGUOUS  0x1        // Request contiguous pages
#define PMM_SUPERPAGE   0x2        // Request 4MB super page

// Structure to represent a physical page
struct page {
    struct list_head list;         // Link in Ascending list for free list
    unsigned int order;           // Order of the block (0 to MAX_ORDER)
    int allocated;                 // 1 if allocated, 0 if free
};

// Physical memory management functions
unsigned get_nps(void);            // Get the number of physical pages
void set_nps(unsigned nps);        // Set the number of physical pages
void pmm_init(uint32_t mem_start, uint32_t mem_end); // Initialize physical memory manager
struct page *alloc_pages(unsigned int order, int flags); // Allocate 2^order pages
void free_pages(struct page *page, unsigned int order);  // Free 2^order pages
uint32_t page_to_phys(struct page *page); // Convert page to physical address
struct page *phys_to_page(uint32_t phys_addr); // Convert physical address to page

#endif /* KERN */
#endif /* !KERN_PMM_H */