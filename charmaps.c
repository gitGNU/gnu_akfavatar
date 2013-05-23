/*
 * character maps support for AKFAvatar
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
#include "avtaddons.h"

#include <stddef.h>
#include <iso646.h>

static const struct avt_char_map *map;


static size_t
map_to_unicode (avt_char * dest, const char *src)
{
  avt_char c;
  const unsigned char s = (const unsigned char) *src;

  if (map and s >= map->start and s <= map->end)
    c = (avt_char) map->table[s - map->start];
  else
    c = (avt_char) s;

  if (dest)
    *dest = c;

  return 1;
}


static size_t
map_from_unicode (char *dest, size_t size, avt_char src)
{
  if (size == 0)
    return 0;

  if (not map or src < (avt_char) map->start
      or (src <= 0xFF and src > (avt_char) map->end))
    *dest = (char) src;
  else				// search table
    {
      char dch = INVALID_CHAR;

      for (int j = map->end - map->start; j >= 0; --j)
	{
	  if (src == (avt_char) map->table[j])
	    {
	      dch = map->start + j;
	      break;
	    }
	}

      *dest = dch;
    }

  return 1;
}


extern struct avt_charenc *
avt_charmap (const struct avt_char_map *m)
{
  static struct avt_charenc converter;

  map = m;
  converter.data = (void *) m;
  converter.to_unicode = map_to_unicode;
  converter.from_unicode = map_from_unicode;

  return &converter;
}
