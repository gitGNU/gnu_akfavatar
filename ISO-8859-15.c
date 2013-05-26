/*
 * ISO-8859-15 (ISO Latin-9) support for AKFAvatar
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

static size_t
lat9_to_unicode (const struct avt_charenc *self, avt_char * dest,
		 const char *src)
{
  (void) self;

  switch (*src)
    {
    case '\xA4':
      *dest = 0x20AC;		// Euro
      break;

    case '\xA6':
      *dest = 0x0160;		// S caron
      break;

    case '\xA8':
      *dest = 0x0161;		// s caron
      break;

    case '\xB4':
      *dest = 0x017D;		// Z caron
      break;

    case '\xB8':
      *dest = 0x017E;		// z caron
      break;

    case '\xBC':
      *dest = 0x0152;		// OE ligature
      break;

    case '\xBD':
      *dest = 0x0153;		// oe ligature
      break;

    case '\xBE':
      *dest = 0x0178;		// Y diaresis
      break;

    default:
      *dest = (avt_char) (unsigned char) *src;
      break;
    }

  return 1;
}


static size_t
lat9_from_unicode (const struct avt_charenc *self, char *dest, size_t size,
		   avt_char src)
{
  (void) self;

  if (size == 0)
    return 0;

  switch (src)
    {
    case 0x20AC:
      *dest = '\xA4';		// Euro
      break;

    case 0x0160:
      *dest = '\xA6';		// S caron
      break;

    case 0x0161:
      *dest = '\xA8';		// s caron
      break;

    case 0x017D:
      *dest = '\xB4';		// Z caron
      break;

    case 0x017E:
      *dest = '\xB8';		// z caron
      break;

    case 0x0152:
      *dest = '\xBC';		// OE ligature
      break;

    case 0x0153:
      *dest = '\xBD';		// oe ligature
      break;

    case 0x0178:
      *dest = '\xBE';		// Y diaresis
      break;

      // those are no longer valid
    case 0x00A4:
    case 0x00A6:
    case 0x00A8:
    case 0x00B4:
    case 0x00B8:
    case 0x00BC:
    case 0x00BD:
    case 0x00BE:
      *dest = INVALID_CHAR;
      break;

    default:
      *dest = (src <= 0xFFu) ? (char) src : INVALID_CHAR;
      break;
    }

  return 1;
}


static const struct avt_charenc converter = {
  .data = NULL,
  .to_unicode = lat9_to_unicode,
  .from_unicode = lat9_from_unicode
};


extern const struct avt_charenc *
avt_iso8859_15 (void)
{
  return &converter;
}
