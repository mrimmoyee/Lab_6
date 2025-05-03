#include "lib/debug.h"
#include "lib/seg.h"
#include "lib/kstack.h"

// Define the kstack structure if not already defined
struct kstack {
	uint32_t magic;
	uint32_t cpu_idx;
	// Add other fields as necessary
};

// Define KSTACK_SIZE with an appropriate value
#define KSTACK_SIZE 4096
#include "lib/x86.h"
#include <stdbool.h> // Include for bool type

// Define NUM_CPUS with an appropriate value (e.g., the number of CPUs in your system)
#define NUM_CPUS 4  // Example value: 4 CPUs

// Define tsc_per_ms with an appropriate value (e.g., based on your system's TSC frequency)
#define tsc_per_ms 1000000  // Example value: 1 million ticks per millisecond

// Define KSTACK_MAGIC with an appropriate value
#define KSTACK_MAGIC 0xdeadbeef

#include "spinlock.h" // Ensure this header defines spinlock_t
#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t lock;
    uint32_t lock_holder;
} spinlock_t;

#endif

void
spinlock_init(spinlock_t *lk)
{
	lk->lock_holder = NUM_CPUS + 1;
	lk->lock = 0;
}


bool
spinlock_holding(spinlock_t *lock)
{
	if(!lock->lock) return false;

	struct kstack *kstack =
		(struct kstack *) ROUNDDOWN(get_stack_pointer(), KSTACK_SIZE);
	KERN_ASSERT(kstack->magic == KSTACK_MAGIC);
	return lock->lock_holder == kstack->cpu_idx;
}

#ifdef DEBUG_DEADLOCK
void
spinlock_acquire_A(spinlock_t *lk)
{
	uint64_t start_tsc = rdtscp();

	while(&lk->lock != 0) {
		if (rdtscp() - start_tsc > tsc_per_ms * 3000)
			KERN_WARN("Possible deadlock 0x%08x.\n", lk);
		pause();
	}
	struct kstack *kstack =
		(struct kstack *) ROUNDDOWN(get_stack_pointer(), KSTACK_SIZE);
	KERN_ASSERT(kstack->magic == KSTACK_MAGIC);
	lk->lock_holder = kstack->cpu_idx;
}

#else

void 
spinlock_acquire_A(spinlock_t *lk)
{
	uint64_t start_tsc = rdtscp();  // Declare and initialize start_tsc
	while(lk->lock != 0) {  // Fixed from '&lk->lock != 0' to 'lk->lock != 0'
		if (rdtscp() - start_tsc > tsc_per_ms * 3000)
			KERN_WARN("Possible deadlock 0x%08x.\n", lk);
		pause();
	}

	struct kstack *kstack =
		(struct kstack *) ROUNDDOWN(get_stack_pointer(), KSTACK_SIZE);
	KERN_ASSERT(kstack->magic == KSTACK_MAGIC);
	lk->lock_holder = kstack->cpu_idx;
}

#endif

int
spinlock_try_acquire_A(spinlock_t *lk)
{
	uint32_t old_val = xchg(&lk->lock, 1);
	if(old_val == 0) {
		struct kstack *kstack =
			(struct kstack *) ROUNDDOWN(get_stack_pointer(), KSTACK_SIZE);
		KERN_ASSERT(kstack->magic == KSTACK_MAGIC);
		lk->lock_holder = kstack->cpu_idx;
	}
	return old_val;
}

void
spinlock_release_A(spinlock_t *lk)
{
	lk->lock_holder = NUM_CPUS + 1;
	xchg(&lk->lock, 0);
}

#ifdef DEBUG_LOCKHOLDING

void
spinlock_acquire_(spinlock_t *lk, ...)
{
	if(spinlock_holding(lk)) {
		while(1) {
			if(rdtsc() % 3) {
				va_list ap;
				va_start(ap, lk);
				cons_vcprintf(&vtys[0],
					"Tried to self-deadlock at %s:%d\n", ap);
				//KERN_PANIC("Tried to self-deadlock at %s:%d\n", file, line);
				va_end(ap);
			}
		}
	}

	spinlock_acquire_A(lk);
}

void
spinlock_release_(spinlock_t *lk, const char *file, int line)
{
	if(!spinlock_holding(lk)) {
		KERN_PANIC("Tried to release unheld lock at %s:%d\n", file, line);
	}

	spinlock_release_A(lk);
}

int
spinlock_try_acquire_(spinlock_t *lk, const char *file, int line)
{
	if(spinlock_holding(lk)) {
		KERN_PANIC("Tried to self-deadlock at %s:%d\n", file, line);
	}

	return spinlock_try_acquire_A(lk);
}

#else

void
spinlock_acquire(spinlock_t *lk)
{
  spinlock_acquire_A(lk);
}

void 
spinlock_release(spinlock_t *lk)
{
  spinlock_release_A(lk);
}

int 
spinlock_try_acquire(spinlock_t *lk)
{
  return spinlock_try_acquire_A(lk);
}

#endif


