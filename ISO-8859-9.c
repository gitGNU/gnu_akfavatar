/*
 * ISO-8859-9 (ISO Latin-5) support for AKFAvatar
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

static size_t
lat5_to_unicode (const struct avt_charenc *self, avt_char * dest,
		 const char *src)
{
  (void) self;

  switch (*src)
    {
    case '\xD0':
      *dest = 0x011E;		// G breve
      break;

    case '\xDD':
      *dest = 0x0130;		// I with dot
      break;

    case '\xDE':
      *dest = 0x015E;		// S cedilla
      break;

    case '\xF0':
      *dest = 0x011F;		// g breve
      break;

    case '\xFD':
      *dest = 0x0131;		// i dotless
      break;

    case '\xFE':
      *dest = 0x015F;		// s cedilla
      break;

    default:
      *dest = (avt_char) (unsigned char) *src;
      break;
    }

  return 1;
}


static size_t
lat5_from_unicode (const struct avt_charenc *self, char *dest, size_t size,
		   avt_char src)
{
  (void) self;

  if (size == 0)
    return 0;

  switch (src)
    {
    case 0x011E:
      *dest = '\xD0';		// G breve
      break;

    case 0x0130:
      *dest = '\xDD';		// I with dot
      break;

    case 0x015E:
      *dest = '\xDE';		// S cedilla
      break;

    case 0x011F:
      *dest = '\xF0';		// g breve
      break;

    case 0x0131:
      *dest = '\xFD';		// i dotless
      break;

    case 0x015F:
      *dest = '\xFE';		// s cedilla
      break;

      // those are no longer valid
    case 0x00D0:
    case 0x00DD:
    case 0x00DE:
    case 0x00F0:
    case 0x00FD:
    case 0x00FE:
      *dest = AVT_INVALID_CHAR;
      break;

    default:
      *dest = (src <= 0xFFu) ? (char) src : AVT_INVALID_CHAR;
      break;
    }

  return 1;
}


static const struct avt_charenc converter = {
  .data = NULL,
  .decode = lat5_to_unicode,
  .encode = lat5_from_unicode
};


extern const struct avt_charenc *
avt_iso8859_9 (void)
{
  return &converter;
}
