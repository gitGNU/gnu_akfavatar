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

#ifndef ARCH_H
#define ARCH_H 1

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/*
 * return file descriptor, if it's an archive
 * or -1 on error
 */
int arch_open (const char *archive);

/* 
 * finds a member in the archive 
 * and leaves the fileposition at its start
 * the member name may not be longer than 15 characters 
 * returns size of the member, or 0 if not found 
 */
size_t arch_find_member (int fd, const char *member);

/* 
 * finds first archive member
 * and leaves the fileposition at its start
 * if member is not NULL it will get the name of the member
 * member must have at least 16 bytes
 * returns size of first member
 */
size_t arch_first_member (int fd, char *member);

/* 
 * read in whole member of a named archive
 * the member name may not be longer than 15 characters 
 * the buffer is allocated with malloc and must be freed by the caller
 * the buffer gets some binary zeros added, so it can be used as string
 * returns size or 0 on error 
 */
size_t arch_get_data (const char *archive, const char *member,
   	              void **buf, size_t * size);

#endif /* ARCH_H */
