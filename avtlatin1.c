/*
 * Latin-1 (ISO-8859-1) support for AKFAvatar
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

/*
 * Latin-1 is just a small subset of the supported characters,
 * but it's still widely used and easy to implement
 */

#include "akfavatar.h"

#define INVALID_CHAR '\x1A'

static size_t
lat1_to_unicode (avt_char * dest, const char *src)
{
  *dest = (avt_char) ((unsigned char) *src);
  return 1;
}


static size_t
lat1_from_unicode (char *dest, avt_char src)
{
  *dest = (src <= 0xFFu) ? (char) src : INVALID_CHAR;
  return 1;
}


extern struct avt_charenc *
avt_latin1 (void)
{
  static struct avt_charenc converter;

  converter.to_unicode = lat1_to_unicode;
  converter.from_unicode = lat1_from_unicode;

  return &converter;
}
