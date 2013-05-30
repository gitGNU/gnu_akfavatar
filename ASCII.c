/*
 * ASCII support for AKFAvatar
 * Copyright (c) 2013 Andreas K. Foerster <info@akfoerster.de>
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

#include "akfavatar.h"
#include "avtinternals.h"
#include <stddef.h>
#include <iso646.h>

#define INVALID_CHAR '\x1A'

static size_t
ascii_decode (const struct avt_charenc *self, avt_char * dest,
		 const char *src)
{
  (void) self;

  *dest = ((*src bitand 0x80) == 0) ? *src : BROKEN_WCHAR;

  return 1;
}


static size_t
ascii_encode (const struct avt_charenc *self, char *dest, size_t size,
		   avt_char src)
{
  (void) self;

  if (size == 0)
    return 0;

  *dest = (src < 0x80) ? (char) src : INVALID_CHAR;

  return 1;
}


static const struct avt_charenc converter = {
  .data = NULL,
  .decode = ascii_decode,
  .encode = ascii_encode
};


extern const struct avt_charenc *
avt_ascii (void)
{
  return &converter;
}
