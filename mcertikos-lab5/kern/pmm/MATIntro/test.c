#include <lib/debug.h>
#include <pmm/MATIntro/export.h>
#include "../pmm.h"

int MATIntro_test1()
{
  int rn10[] = {1,3,5,6,78,3576,32,8,0,100};
  int i;
  int nps = get_nps();
  for(i = 0; i< 10; i++) {
    set_nps(rn10[i]);
    if (get_nps() != rn10[i]) {
      set_nps(nps);
      dprintf("test 1 failed.\n");
      return 1;
    }
  }
  set_nps(nps);
  dprintf("test 1 passed.\n");
  return 0;
}

int MATIntro_test2()
{
  at_set_perm(0, 0);
  if (at_is_norm(0) != 0 || at_is_allocated(0) != 0) {
    at_set_perm(0, 0);
    dprintf("test 2 failed.\n");
    return 1;
  }
  at_set_perm(0, 1);
  if (at_is_norm(0) != 0 || at_is_allocated(0) != 0) {
    at_set_perm(0, 0);
    dprintf("test 2 failed.\n");
    return 1;
  }
  at_set_perm(0, 2);
  if (at_is_norm(0) != 1 || at_is_allocated(0) != 0) {
    at_set_perm(0, 0);
    dprintf("test 2 failed.\n");
    return 1;
  }
  at_set_perm(0, 100);
  if (at_is_norm(0) != 1 || at_is_allocated(0) != 0) {
    at_set_perm(0, 0);
    dprintf("test 2 failed.\n");
    return 1;
  }
  at_set_perm(0, 0);
  dprintf("test 2 passed.\n");
  return 0;
}

int MATIntro_test3()
{
  at_set_allocated(1, 0);
  if (at_is_allocated(1) != 0) {
    at_set_allocated(1, 0);
    dprintf("test 3 failed.\n");
    return 1;
  }
  at_set_allocated(1, 1);
  if (at_is_allocated(1) != 1) {
    at_set_allocated(1, 0);
    dprintf("test 3 failed.\n");
    return 1;
  }
  at_set_allocated(1, 100);
  if (at_is_allocated(1) != 1) {
    at_set_allocated(1, 0);
    dprintf("test 3 failed.\n");
    return 1;
  }
  at_set_allocated(1, 0);
  dprintf("test 3 passed.\n");
  return 0;
}

int MATIntro_test_own()
{
  // TODO (optional)
  // dprintf("own test passed.\n");
  return 0;
}

int test_MATIntro()
{
  return MATIntro_test1() + MATIntro_test2() + MATIntro_test3() + MATIntro_test_own();
}
