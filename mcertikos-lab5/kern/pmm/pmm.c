#include "pmm.h"

static unsigned num_pages = 0;  // Global variable to store the number of physical pages

unsigned get_nps(void) {
    return num_pages;
}

void set_nps(unsigned nps) {
    num_pages = nps;
}
