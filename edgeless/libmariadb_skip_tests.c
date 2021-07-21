/* Copyright (c) 2021, Edgeless Systems GmbH

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1335
   USA */

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
#if EDG_WITH_EDB
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
