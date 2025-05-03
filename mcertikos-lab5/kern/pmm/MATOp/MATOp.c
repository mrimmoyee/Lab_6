#include <lib/debug.h>
#include "import.h"
#include "../pmm.h"

#define KERN_LOW 262144
#define NUM_PROC 64

static unsigned int last_palloc_index = 262144;

unsigned int
palloc()
{
    unsigned int tnps;
    unsigned int palloc_index;
    unsigned int palloc_cur_at;
    unsigned int palloc_is_norm;
    unsigned int palloc_free_index;

    mem_lock();

    tnps = get_nps();
    palloc_index = last_palloc_index + 1;
    palloc_free_index = tnps;
    while( palloc_index < tnps && palloc_free_index == tnps )
    {
        palloc_is_norm = at_is_norm(palloc_index);
        if (palloc_is_norm == 1)
        {
            palloc_cur_at = at_is_allocated(palloc_index);
            if (palloc_cur_at == 0)
                palloc_free_index = palloc_index;
        }
        palloc_index ++;
    }
    if (palloc_free_index == tnps)
      palloc_free_index = 0;
    else
    {
      at_set_allocated(palloc_free_index, 1);
    }
    last_palloc_index = palloc_free_index % tnps;

    mem_unlock();

    return palloc_free_index;
} 

void
pfree(unsigned int pfree_index)
{
  mem_lock();
  at_set_allocated(pfree_index, 0);
  mem_unlock();
}
