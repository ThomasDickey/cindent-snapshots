/*
   Copyright 1999-2002,2010, Thomas E. Dickey

   Copyright (c) 1993,1994, Joseph Arceneaux.  All rights reserved.

   This file is subject to the terms of the GNU General Public License as
   published by the Free Software Foundation.  A copy of this license is
   included with this software distribution in the file COPYING.  If you
   do not have a copy, you may obtain a copy by writing to the Free
   Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

   This software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details. */

#include <stdio.h>

/* Values of special characters. */
#define TAB '\t'
#define EOL '\n'
#define BACKSLASH '\\'
#define NULL_CHAR '\0'

extern int debug;

#define INLINE

#ifdef VMS
# define ONE_DOT_PER_FILENAME 1
# define NODIR 1
# define PROFILE_FORMAT "%s%s"
# define BACKUP_SUFFIX_STR    "_"
# define BACKUP_SUFFIX_CHAR   '_'
# define BACKUP_SUFFIX_FORMAT "%s._%d_"
# define SYS_READ vms_read	/* Defined in io.c */
# ifdef VAXC
#  include <unixio.h>
# endif
#endif /* VMS */

/* Contributed by Beverly Brown, <beverly@norton.datacube.com> */
#ifdef _WIN32
# define USG 1
# define NODIR 1
#endif /* _WIN32 */

struct file_buffer
  {
    char *name;
    size_t size;
    char *data;
  };

extern struct file_buffer *read_file (const char *);
extern struct file_buffer *read_stdin (void);
