/* 
 * some functions for handling ar archives
 * Copyright (c) 2008 Andreas K. Foerster <info@akfoerster.de>
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

/* $Id: arch.c,v 2.5 2008-10-09 18:39:03 akf Exp $ */

#include "arch.h"

/*
 * some weird systems needs O_BINARY, most others not
 * so I define a dummy value for sane systems
 */
#ifndef O_BINARY
#  define O_BINARY 0
#endif

/* structure of an ar member header */
struct arch_member
{
  char name[16];
  char date[12];
  char uid[6], gid[6];
  char mode[8];
  char size[10];
  char magic[2];
};

/*
 * return file descriptor, if it's an archive
 * or -1 on error
 */
int
arch_open (const char *archive)
{
  int fd;
  char archive_magic[8];

  fd = open (archive, O_RDONLY | O_BINARY);
  if (fd < 0)
    return -1;

  read (fd, &archive_magic, 8);

  if (memcmp ("!<arch>\n", archive_magic, 8) != 0)
    {
      close (fd);
      fd = -1;
    }

  return fd;
}


/* finds a member in the archive */
/* returns size of the file, or 0 if not found or on error */
size_t
arch_find_member (int fd, const char *member)
{
  size_t name_length;
  size_t skip_size;
  struct arch_member header;

  name_length = strlen (member);

  if (name_length > 15)
    return 0;

  lseek (fd, 8, SEEK_SET);	/* go to first entry */
  read (fd, &header, sizeof (header));

  /* check magic entry */
  if (memcmp (&header.magic, "`\n", 2) != 0)
    return 0;

  /* check name */
  while (memcmp (&header.name, member, name_length) != 0
	 || (header.name[name_length] != ' '
	     && header.name[name_length] != '/'))
    {
      /* skip block */
      skip_size = strtoul ((const char *) &header.size, NULL, 10);
      if (skip_size % 2 != 0)
	skip_size++;
      lseek (fd, skip_size, SEEK_CUR);

      /* read next block-header */
      if (read (fd, &header, sizeof (header)) <= 0)
	return 0;		/* end reached - not found */

      /* check magic entry */
      if (memcmp (&header.magic, "`\n", 2) != 0)
	return 0;
    }

  return strtoul ((const char *) &header.size, NULL, 10);
}

/* 
 * finds first archive member
 * if member is not NULL it will get the name of the member
 * member must have at least 16 bytes
 * returns size of first member
 */
size_t
arch_first_member (int fd, char *member)
{
  size_t size;
  struct arch_member header;
  char *end;

  lseek (fd, 8, SEEK_SET);	/* go to first entry */
  read (fd, &header, sizeof (header));

  /* check magic entry */
  if (memcmp (&header.magic, "`\n", 2) != 0)
    return 0;

  size = strtoul ((const char *) &header.size, NULL, 10);

  if (member != NULL)
    {
      memcpy (member, header.name, sizeof (header.name));

      /* find end of the name */
      /* either terminated by / or by space */
      end = (char *) memchr (member, '/', sizeof (header.name));

      if (!end)
	end = (char *) memchr (member, ' ', sizeof (header.name));

      if (end)
	*end = '\0';		/* make it a valid C-string */
      else
	*member = '\0';
    }

  return size;
}

/* 
 * read in whole member of a named archive
 * the buffer is allocated with malloc and must be freed by the caller
 * the buffer gets some binary zeros added, so it can be used as string
 * returns size or 0 on error 
 */
size_t
arch_get_data (const char *archive, const char *member,
	       void **buf, size_t * size)
{
  int archive_fd;

  if (buf == NULL || size == NULL)
    return 0;

  *size = 0;
  *buf = NULL;

  archive_fd = arch_open (archive);
  if (archive_fd < 0)
    return 0;

  *size = arch_find_member (archive_fd, member);
  if (*size > 0)
    {
      /* we add 4 0-Bytes as possible string-terminator */
      *buf = (void *) malloc (*size + 4);
      
      if (*buf != NULL)
        {
	  read (archive_fd, *buf, *size);
	  memset (*buf + *size, '\0', 4);
	}
      else
	*size = 0;
    }

  close (archive_fd);

  return *size;
}
