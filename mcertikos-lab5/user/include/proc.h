#ifndef PROC_H
#define PROC_H

// Include the header that defines pgd_t
// Adjust path based on where pgd_t is defined
#include "vmm/MPTComm/export.h"
#include "vmm/MPTInit/export.h"


// Existing includes (e.g., from Lab 5)
#include "types.h"

// Process structure
struct proc {
    // Existing fields (from Lab 5 or earlier)
    uint32_t pid;          // Process ID
    uint32_t brk;          // Heap break
    uint32_t heap_start;   // Start of heap
    pgd_t *pgd;            // Page directory pointer (now defined)
    int superpage_enabled; // 1 if super pages enabled
    int contiguous_enabled; 
};
typedef struct pgd_entry pgd_t; // Assuming pgd_entry is defined in MPTComm/export.h
// Function prototypes (if any)
struct proc *current_proc(void);
void kill_proc(struct proc *p);

#endif // PROC_H