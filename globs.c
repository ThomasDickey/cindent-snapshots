/* Copyright (c) 1993,1994, Joseph Arceneaux.  All rights reserved.

   Copyright (C) 1986, 1989, 1992 Free Software Foundation, Inc. All rights
   reserved.

   This file is subject to the terms of the GNU General Public License as
   published by the Free Software Foundation.  A copy of this license is
   included with this software distribution in the file COPYING.  If you
   do not have a copy, you may obtain a copy by writing to the Free
   Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

   This software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details. */


#include "sys.h"
#include "indent.h"

#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>

/* Like malloc but get error if no storage available.  size really should be
   size_t, but not all systems have size_t, so I hope "unsigned" will work.
   It works for GNU style machines, where it is 32 bits, and works on
   MS-DOS.  */

char *
xmalloc (unsigned size)
{
  char *val = (char *) malloc (size);
  if (!val)
    {
      fprintf (stderr, "indent: Virtual memory exhausted.\n");
      exit (system_error);
    }

#if defined (DEBUG)
  /* Fill it with garbage to detect code which depends on stuff being
     zero-filled.  */
  memset (val, 'x', size);
#endif

  return val;
}

/* Like realloc but get error if no storage available.  */

char *
xrealloc (char *ptr, unsigned size)
{
  char *val = (char *) realloc (ptr, size);
  if (!val)
    {
      fprintf (stderr, "indent: Virtual memory exhausted.\n");
      exit (system_error);
    }

  return val;
}

void
message (int warnings, char *string, ...)
{
  if (((warnings > 0) && verbose) || (warnings <= 0))
    {
      va_list ap;

      if (warnings >= 0)
	{
	  fprintf (stderr, "%s:%d", in_name, in_line_no);
	  if (warnings > 1)
	    fprintf (stderr, ":%d", warnings);	/* column for broken-line */
	  fprintf (stderr, ": ");
	}
      fprintf (stderr, "%s: ", warnings ? "Warning" : "Error");

      va_start (ap, string);
      vfprintf (stderr, string, ap);
      va_end (ap);

      fprintf (stderr, "\n");
    }
}

/* Print a fatal error message and exit, or, if compiled with
   "DEBUG" defined, abort (). */

void
fatal (char *string, ...)
{
  va_list ap;

  fprintf (stderr, "indent: Fatal Error: ");

  va_start (ap, string);
  vfprintf (stderr, string, ap);
  va_end (ap);

  fprintf (stderr, "\n");

#ifdef DEBUG
  abort ();
#endif /* DEBUG */

  if (errno)
    {
      fprintf (stderr, "indent: System Error: ");
      perror (0);
    }

  exit (indent_fatal);
}
