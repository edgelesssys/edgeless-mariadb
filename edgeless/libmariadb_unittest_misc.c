// This file disables the test "test_frm_bug".
// In libmariadb/unittest/libmariadb/misc.c there's an array "my_tests" where
// you can set the field "skipmsg" to disable a test. As libmariadb is a
// submodule that we don't want to touch, we add this file to the executable
// (see CMakeLists). The array is then modified at runtime.

#include <string.h>

struct my_tests_st
{
  const char *name;
  void *function;
  int connection;
  unsigned long connect_flags;
  struct my_option_st *options;
  const char *skipmsg;
};

__attribute__((constructor)) static void skip_test(void)
{
  extern struct my_tests_st my_tests[];
  for (struct my_tests_st *t= my_tests; t->name; ++t)
  {
    if (strcmp(t->name, "test_frm_bug") == 0)
    {
      t->skipmsg= "EDG: needs frm files on disk";
      break;
    }
  }
}
