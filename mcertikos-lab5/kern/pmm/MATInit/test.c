#include <lib/debug.h>
#include <pmm/MATIntro/export.h>
#include "../pmm.h"

int MATInit_test1()
{
  int i;
  int nps = get_nps();
  if (nps <= 1000) {
    dprintf("test 1 failed.\n");
    return 1;
  }
  for(i = 0; i < nps; i ++) {
    if (at_is_allocated(i) != 0) {
      dprintf("allocated: %d: %d\n", i, at_is_allocated(i));
      dprintf("test 1 failed.\n");
      return 1;
    }
    if ((i < 0x40000000/4096 || i >= 0xF0000000/4096) && at_is_norm(i) != 0) {
      dprintf("norm: %d: %d\n", i, at_is_norm(i));
      dprintf("test 1 failed.\n");
      return 1;
    }
  }
  dprintf("test 1 passed.\n");
  return 0;
}

int MATInit_test_own()
{
  // TODO (optional)
  // dprintf("own test passed.\n");
  return 0;
}

int test_MATInit()
{
  return MATInit_test1() + MATInit_test_own();
}
