#include "pmm.h"

static unsigned num_pages = 0;  // Global variable to store the number of physical pages

unsigned get_nps(void) {
    return num_pages;
}

void set_nps(unsigned nps) {
    num_pages = nps;
}

struct page *alloc_pages(unsigned int order);
void free_pages(struct page *page, unsigned int order);
struct page *alloc_super_page(void);
uint32_t page_to_phys(struct page *page);
struct page *phys_to_page(uint32_t phys_addr);