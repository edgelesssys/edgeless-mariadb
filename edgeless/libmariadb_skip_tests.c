// This file disables tests in libmariadb that are incompatible with our
// changes. Unit tests in libmariadb define an array "my_tests" where you can
// set the field "skipmsg" to disable a test. As libmariadb is a submodule that
// we don't want to touch, we add this file to the executables (see
// CMakeLists). The array is then modified at runtime.

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

__attribute__((constructor)) static void skip_tests(void)
{
  extern struct my_tests_st my_tests[];
  for (struct my_tests_st *t= my_tests; t->name; ++t)
  {
    if (strcmp(t->name, "test_frm_bug") == 0)
      t->skipmsg= "EDG: needs frm files on disk";
#if WITH_EDB
    else if (strcmp(t->name, "test_bug1500") == 0 ||
             strcmp(t->name, "test_conc68") == 0 ||
             strcmp(t->name, "test_conc70") == 0)
      t->skipmsg= "EDG: needs MyISAM";
    else if (strcmp(t->name, "test_conc496") == 0)
      t->skipmsg= "EDG: needs InnoDB";
    else if (strcmp(t->name, "test_connection_timeout2") == 0)
      t->skipmsg= "EDG: needs unencrypted connection";
#endif
  }
}
