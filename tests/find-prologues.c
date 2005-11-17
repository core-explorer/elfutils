/* Test program for dwarf_entry_breakpoints.
   Copyright (C) 2005 Red Hat, Inc.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#include <config.h>
#include <assert.h>
#include <inttypes.h>
#include ELFUTILS_HEADER(dwfl)
#include <dwarf.h>
#include <argp.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <locale.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <fnmatch.h>


struct args
{
  Dwfl *dwfl;
  Dwarf_Die *cu;
  Dwarf_Addr dwbias;
  char **argv;
};

static int
handle_function (Dwarf_Die *func, void *arg)
{
  struct args *a = arg;

  const char *name = dwarf_diename (func);
  char **argv = a->argv;
  if (argv[0] != NULL)
    {
      bool match;
      do
	match = fnmatch (*argv, name, 0) == 0;
      while (!match && *++argv);
      if (!match)
	return 0;
    }

  if (dwarf_func_inline (func))
    return 0;

  Dwarf_Addr entrypc;
  if (dwarf_entrypc (func, &entrypc) != 0)
    error (EXIT_FAILURE, 0, "dwarf_entrypc: %s: %s",
	   dwarf_diename (func), dwarf_errmsg (-1));
  entrypc += a->dwbias;

  printf ("%-16s %#.16" PRIx64, dwarf_diename (func), entrypc);

  Dwarf_Addr *bkpts = NULL;
  int result = dwarf_entry_breakpoints (func, &bkpts);
  if (result <= 0)
    printf ("\t%s\n", dwarf_errmsg (-1));
  else
    {
      for (int i = 0; i < result; ++i)
	printf (" %#.16" PRIx64 "%s", bkpts[i] + a->dwbias,
		i == result - 1 ? "\n" : "");
      free (bkpts);
    }

  return 0;
}


int
main (int argc, char *argv[])
{
  int remaining;

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  struct args a = { .dwfl = NULL, .cu = NULL };

  (void) argp_parse (dwfl_standard_argp (), argc, argv, 0, &remaining,
		     &a.dwfl);
  assert (a.dwfl != NULL);
  a.argv = &argv[remaining];

  int result = 0;

  while ((a.cu = dwfl_nextcu (a.dwfl, a.cu, &a.dwbias)) != NULL)
    dwarf_getfuncs (a.cu, &handle_function, &a, 0);

  return result;
}
