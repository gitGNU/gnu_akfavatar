/*
 * character maps support for AKFAvatar
 * Copyright (c) 2013 Andreas K. Foerster <akf@akfoerster.de>
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


extern size_t
map_to_unicode (const struct avt_charenc *self, avt_char * dest,
		const char *src)
{
  const struct avt_char_map *map = self->data;
  const unsigned char s = (const unsigned char) *src;

  if (map and s >= map->start and s <= map->end)
    *dest = map->table[s - map->start];
  else
    *dest = (avt_char) s;

  return 1;
}


extern size_t
map_from_unicode (const struct avt_charenc *self, char *dest,
		  size_t size, avt_char src)
{
  const struct avt_char_map *map = self->data;

  if (size == 0)
    return 0;

  if (not map or src < map->start or (src <= 0xFF and src > map->end))
    *dest = (char) src;
  else				// search table
    {
      char dch = AVT_INVALID_CHAR;

      for (int i = map->end - map->start; i >= 0; --i)
	{
	  if (src == map->table[i])
	    {
	      dch = map->start + i;
	      break;
	    }
	}

      *dest = dch;
    }

  return 1;
}
