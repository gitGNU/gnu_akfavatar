/*
 * reading data from a file
 * Copyright (c) 2009 Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is part of AKFAvatar
 *
 * AKFAvatar is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AKFAvatar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* feature test macro needed for popen/pclose */
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 2
#endif

#include "akfavatar.h"
#include "avtaddons.h"
#include <stdio.h>

static int
read_stream (FILE * f, char **buffer, avt_bool_t terminate)
{
  char *buf;
  int size, capacity;
  ssize_t nread;

  *buffer = buf = NULL;
  size = capacity = 0;
  nread = 0;

  do
    {
      /* do we need more capacity? */
      if (size >= capacity)
	{
	  char *nbuf;

	  capacity += 1024;
	  if (terminate)
	    nbuf = (char *) realloc (buf, capacity + 4);
	  else
	    nbuf = (char *) realloc (buf, capacity);

	  if (nbuf)
	    buf = nbuf;
	  else
	    break;
	}

      nread = fread (buf + size, 1, capacity - size, f);
      if (nread > 0)
	size += nread;
    }
  while (nread > 0);

  /* make buffer smaller if possible and eventually add terminator */
  if (buf)
    {
      char *nbuf;

      if (terminate)
	{
	  /* I terminate with 4 zeros, in case UTF-32 is used */
	  memset (buf + size, '\0', 4);
	  nbuf = (char *) realloc (buf, size + 4);
	}
      else
	nbuf = (char *) realloc (buf, size);

      if (nbuf)
	buf = nbuf;
    }

  *buffer = buf;
  return size;			/* size without terminator */
}

extern int
avta_read_textfile (const char *file_name, char **buffer)
{
  FILE *f;
  char *buf;
  int size;

  if (!buffer)
    return 0;

  buf = NULL;
  size = 0;

  if (file_name == NULL)
    f = stdin;
  else
    f = fopen (file_name, "rt");

  if (f)
    {
      size = read_stream (f, &buf, AVT_TRUE);

      if (f != stdin)
	(void) fclose (f);
    }

  *buffer = buf;
  return size;
}

extern int
avta_read_datafile (const char *file_name, void **buffer)
{
  FILE *f;
  char *buf;
  int size;

  if (!buffer || !file_name)
    return 0;

  buf = NULL;
  size = 0;

  f = fopen (file_name, "rb");

  if (f)
    {
      size = read_stream (f, &buf, AVT_FALSE);
      (void) fclose (f);
    }

  *buffer = (void *) buf;
  return size;
}

extern int
avta_read_command (const char *command, char **buffer)
{
  FILE *f;
  char *buf;
  int size;

  if (!buffer || !command)
    return 0;

  buf = NULL;
  size = 0;

  f = popen (command, "r");

  if (f)
    {
      size = read_stream (f, &buf, AVT_TRUE);
      (void) pclose (f);
    }

  *buffer = buf;
  return size;
}
