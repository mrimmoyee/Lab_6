#include "pmm.h"
#include "../lib/debug.h"
#include <stdint.h> // Include for uint32_t and other fixed-width integer types
#include <stddef.h> // Include for NULL definition
 // Update the path to the correct location of list.h

#ifndef LIST_H
#define LIST_H

struct list_head {
    struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr);         \
    (ptr)->prev = (ptr);         \
} while (0)

#define list_first_entry(ptr, type, member) \
    ((type *)((char *)(ptr)->next - offsetof(type, member)))

#define list_empty(head) ((head)->next == (head))

#define list_add(new, head) do { \
    (new)->next = (head)->next;  \
    (new)->prev = (head);        \
    (head)->next->prev = (new);  \
    (head)->next = (new);        \
} while (0)

#define list_del(entry) do { \
    (entry)->next->prev = (entry)->prev; \
    (entry)->prev->next = (entry)->next; \
} while (0)

#endif // LIST_H
// Define PMM_SUPERPAGE flag
#define PMM_SUPERPAGE (1 << 0)
struct page {
    int allocated;
    unsigned int order;
    struct list_head list; // Ensure this field is correctly declared for list operations
};

#define MAX_ORDER 10 // Define MAX_ORDER with an appropriate value
#define PAGE_SIZE 4096 // Define PAGE_SIZE as 4KB
#define SUPER_PAGE_SIZE (PAGE_SIZE * (1 << MAX_ORDER)) // Define SUPER_PAGE_SIZE as a super-page size

static unsigned num_pages = 0;         // Total number of physical pages
static struct page *page_array;        // Array of page metadata
static struct list_head free_lists[MAX_ORDER + 1]; // Free lists for each order
static uint32_t mem_base;              // Base physical memory address

unsigned get_nps(void) {
    return num_pages;
}

void set_nps(unsigned nps) {
    num_pages = nps;
}

void pmm_init(uint32_t mem_start, uint32_t mem_end) {
    mem_base = mem_start;
    num_pages = (mem_end - mem_start) / PAGE_SIZE;
    page_array = (struct page *)mem_start; // Use start of memory for metadata

    // Initialize page metadata
    for (unsigned i = 0; i < num_pages; i++) {
        page_array[i].allocated = 0;
        page_array[i].order = 0;
        INIT_LIST_HEAD(&page_array[i].list);
    }

    // Initialize free lists
    for (unsigned i = 0; i <= MAX_ORDER; i++) {
        INIT_LIST_HEAD(&free_lists[i]);
    }

    // Add all memory as largest possible blocks
    uint32_t block_size = SUPER_PAGE_SIZE;
    for (uint32_t addr = mem_start; addr < mem_end; addr += block_size) {
        if (addr + block_size <= mem_end) {
            struct page *page = &page_array[(addr - mem_start) / PAGE_SIZE];
            page->order = MAX_ORDER;
            list_add(&page->list, &free_lists[MAX_ORDER]);
        }
    }
    cprintf("PMM initialized with %u pages\n", num_pages);
}

struct page *alloc_pages(unsigned int order, int flags) {
    if (order > MAX_ORDER)
        return NULL;

    // Find a free block of sufficient size
    for (unsigned curr_order = order; curr_order <= MAX_ORDER; curr_order++) {
        if (!list_empty(&free_lists[curr_order])) {
            struct page *page = list_first_entry(&free_lists[curr_order], struct page, list);
            list_del(&page->list);

            // Split larger blocks if necessary
            while (curr_order > order) {
                curr_order--;
                struct page *buddy = page + (1 << curr_order);
                buddy->order = curr_order;
                list_add(&buddy->list, &free_lists[curr_order]);
            }

            page->allocated = 1;
            page->order = order;

            // Super-page alignment check
            if (flags & PMM_SUPERPAGE) {
                if (order != MAX_ORDER || (page_to_phys(page) & (SUPER_PAGE_SIZE - 1))) {
                    free_pages(page, order);
                    return NULL;
                }
            }
            return page;
        }
    }
    return NULL; // No suitable block found
}

void free_pages(struct page *page, unsigned int order) {
    if (!page || !page->allocated || order > MAX_ORDER)
        return;

    page->allocated = 0;

    // Coalesce with buddies
    uint32_t base_idx = (page - page_array);
    while (order < MAX_ORDER) {
        uint32_t buddy_idx = base_idx ^ (1 << order);
        struct page *buddy = &page_array[buddy_idx];

        if (buddy_idx >= num_pages || buddy->allocated || buddy->order != order)
            break;

        list_del(&buddy->list);
        if (buddy < page)
            page = buddy; // Use lower address
        order++;
        page->order = order;
    }
    list_add(&page->list, &free_lists[order]);
}

uint32_t page_to_phys(struct page *page) {
    return mem_base + ((page - page_array) * PAGE_SIZE);
}

struct page *phys_to_page(uint32_t phys_addr) {
    if (phys_addr < mem_base || phys_addr >= mem_base + (num_pages * PAGE_SIZE))
        return NULL;
    return &page_array[(phys_addr - mem_base) / PAGE_SIZE];
}