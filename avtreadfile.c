/*
 * reading a file
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

#include "avtaddons.h"
#include <fcntl.h>
#include <unistd.h>

/*
 * some weird systems needs O_BINARY, most others not
 * so I define a dummy value for sane systems
 */
#ifndef O_BINARY
#  define O_BINARY 0
#endif

/* read in a file */
extern char *
avta_read_file (const char *f, size_t * data_size, avt_bool_t textmode)
{
  int fd;
  char *buf;
  size_t size, capacity;
  ssize_t nread;

  buf = NULL;
  size = capacity = 0;
  nread = 0;

  /* STDIN only for textmode */
  if (f == NULL && !textmode)
    return NULL;

  if (f == NULL)
    fd = STDIN_FILENO;
  else				/* f != NULL */
    {
      if (textmode)
	fd = open (f, O_RDONLY);
      else
	fd = open (f, O_RDONLY | O_BINARY);
    }

  if (fd > -1)
    {
      do
	{
	  /* do we need more capacity? */
	  if (size >= capacity)
	    {
	      char *nbuf;

	      capacity += 1024;
	      if (textmode)
		nbuf = (char *) realloc (buf, capacity + 4);
	      else
		nbuf = (char *) realloc (buf, capacity);

	      if (nbuf)
		buf = nbuf;
	      else
		break;
	    }

	  nread = read (fd, buf + size, capacity - size);
	  if (nread > 0)
	    size += nread;
	}
      while (nread > 0);

      if (fd != STDIN_FILENO)
        (void) close (fd);

      if (buf)
	{
	  char *nbuf;

	  if (textmode)
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
    }

  if (data_size)
    *data_size = size;

  return buf;
}
