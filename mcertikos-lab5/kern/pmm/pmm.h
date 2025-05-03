#ifndef _KERN_PMM_H_
#define _KERN_PMM_H_

#ifdef _KERN_

#include <lib/types.h>

unsigned get_nps(void);  // Get the number of physical pages
void set_nps(unsigned nps);  // Set the number of physical pages

#endif /* _KERN_ */
#endif /* !_KERN_PMM_H_ */
