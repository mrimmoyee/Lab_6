#include <lib/debug.h>
#include <lib/x86.h>
#include "import.h"
#include "../pmm.h"

struct SContainer {
  int quota;      // maximum memory quota of the process
  int usage;      // the current memory usage of the process
  int parent;     // the id of the parent process
  int nchildren;  // the number of child processes
  int used;       // whether current container is used by a process
};

// mCertiKOS supports up to NUM_IDS processes
static struct SContainer CONTAINER[NUM_IDS]; 

void
container_init(unsigned int mbi_addr)
{
  unsigned int real_quota;
  unsigned int nps, i, norm, used;

  pmem_init(mbi_addr);
  real_quota = 0;

  nps = get_nps();
  i = 1;
  while (i < nps) {
    norm = at_is_norm(i);
    used = at_is_allocated(i);
    if (norm == 1 && used == 0)
      real_quota++;
    i++;
  }
  KERN_DEBUG("\nreal quota: %d\n\n", real_quota);

  CONTAINER[0].quota = real_quota;
  CONTAINER[0].usage = 0;
  CONTAINER[0].parent = 0;
  CONTAINER[0].nchildren = 0;
  CONTAINER[0].used = 1;
}

unsigned int container_get_parent(unsigned int id)
{
  return CONTAINER[id].parent;
}

unsigned int container_get_nchildren(unsigned int id)
{
  return CONTAINER[id].nchildren;
}

unsigned int container_get_quota(unsigned int id)
{
  return CONTAINER[id].quota;
}

unsigned int container_get_usage(unsigned int id)
{
  return CONTAINER[id].usage;
}

unsigned int container_can_consume(unsigned int id, unsigned int n)
{
  if (CONTAINER[id].usage + n > CONTAINER[id].quota) return 0;
  return 1;
}

unsigned int container_split(unsigned int id, unsigned int quota)
{
  unsigned int child, nc;

  nc = CONTAINER[id].nchildren;
  child = id * MAX_CHILDREN + 1 + nc; // container index for the child process

  CONTAINER[child].used = 1;
  CONTAINER[child].quota = quota;
  CONTAINER[child].usage = 0;
  CONTAINER[child].parent = id;
  CONTAINER[child].nchildren = 0;

  CONTAINER[id].usage += quota;
  CONTAINER[id].nchildren = nc + 1;

  return child;
}

unsigned int container_alloc(unsigned int id)
{
  unsigned int u, q, i;
  u = CONTAINER[id].usage;
  q = CONTAINER[id].quota;
  if (u == q) return 0;

  CONTAINER[id].usage = u + 1;
  i = palloc();
  return i;
}

void container_free(unsigned int id, unsigned int page_index)
{
  if (at_is_allocated(page_index)) {
    pfree(page_index);
    if (CONTAINER[id].usage > 0)
      CONTAINER[id].usage -= 1;
  }
}
