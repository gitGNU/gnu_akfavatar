/*
 * ISO-8859-8 (Hebrew) support for AKFAvatar
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

#include "avtaddons.h"

static const struct avt_char_map map = {
  .start = 0xA1,.end = 0xFF,
  .table = {
	    INVALID_WCHAR,	// 0xA1
	    0x00A2,		// 0xA2
	    0x00A3,		// 0xA3
	    0x00A4,		// 0xA4
	    0x00A5,		// 0xA5
	    0x00A6,		// 0xA6
	    0x00A7,		// 0xA7
	    0x00A8,		// 0xA8
	    0x00A9,		// 0xA9
	    0x00D7,		// 0xAA
	    0x00AB,		// 0xAB
	    0x00AC,		// 0xAC
	    0x00AD,		// 0xAD
	    0x00AE,		// 0xAE
	    0x00AF,		// 0xAF
	    0x00B0,		// 0xB0
	    0x00B1,		// 0xB1
	    0x00B2,		// 0xB2
	    0x00B3,		// 0xB3
	    0x00B4,		// 0xB4
	    0x00B5,		// 0xB5
	    0x00B6,		// 0xB6
	    0x00B7,		// 0xB7
	    0x00B8,		// 0xB8
	    0x00B9,		// 0xB9
	    0x00F7,		// 0xBA
	    0x00BB,		// 0xBB
	    0x00BC,		// 0xBC
	    0x00BD,		// 0xBD
	    0x00BE,		// 0xBE
	    INVALID_WCHAR,	// 0xBF
	    INVALID_WCHAR,	// 0xC0
	    INVALID_WCHAR,	// 0xC1
	    INVALID_WCHAR,	// 0xC2
	    INVALID_WCHAR,	// 0xC3
	    INVALID_WCHAR,	// 0xC4
	    INVALID_WCHAR,	// 0xC5
	    INVALID_WCHAR,	// 0xC5
	    INVALID_WCHAR,	// 0xC7
	    INVALID_WCHAR,	// 0xC8
	    INVALID_WCHAR,	// 0xC9
	    INVALID_WCHAR,	// 0xCA
	    INVALID_WCHAR,	// 0xCB
	    INVALID_WCHAR,	// 0xCC
	    INVALID_WCHAR,	// 0xCD
	    INVALID_WCHAR,	// 0xCE
	    INVALID_WCHAR,	// 0xCF
	    INVALID_WCHAR,	// 0xD0
	    INVALID_WCHAR,	// 0xD1
	    INVALID_WCHAR,	// 0xD2
	    INVALID_WCHAR,	// 0xD3
	    INVALID_WCHAR,	// 0xD4
	    INVALID_WCHAR,	// 0xD5
	    INVALID_WCHAR,	// 0xD6
	    INVALID_WCHAR,	// 0xD7
	    INVALID_WCHAR,	// 0xD8
	    INVALID_WCHAR,	// 0xD9
	    INVALID_WCHAR,	// 0xDA
	    INVALID_WCHAR,	// 0xDB
	    INVALID_WCHAR,	// 0xDC
	    INVALID_WCHAR,	// 0xDD
	    INVALID_WCHAR,	// 0xDE
	    0x2017,		// 0xDF
	    0x05D0,		// 0xE0
	    0x05D1,		// 0xE1
	    0x05D2,		// 0xE2
	    0x05D3,		// 0xE3
	    0x05D4,		// 0xE4
	    0x05D5,		// 0xE5
	    0x05D6,		// 0xE6
	    0x05D7,		// 0xE7
	    0x05D8,		// 0xE8
	    0x05D9,		// 0xE9
	    0x05DA,		// 0xEA
	    0x05DB,		// 0xEB
	    0x05DC,		// 0xEC
	    0x05DD,		// 0xED
	    0x05DE,		// 0xEE
	    0x05DF,		// 0xEF
	    0x05E0,		// 0xF0
	    0x05E1,		// 0xF1
	    0x05E2,		// 0xF2
	    0x05E3,		// 0xF3
	    0x05E4,		// 0xF4
	    0x05E5,		// 0xF5
	    0x05E6,		// 0xF6
	    0x05E7,		// 0xF7
	    0x05E8,		// 0xF8
	    0x05E9,		// 0xF9
	    0x05EA,		// 0xFA
	    INVALID_WCHAR,	// 0xFB
	    INVALID_WCHAR,	// 0xFC
	    0x200E,		// 0xFD
	    0x200F,		// 0xFE
	    INVALID_WCHAR	// 0xFF
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &map,
  .decode = map_to_unicode,
  .encode = map_from_unicode
};


extern const struct avt_charenc *
avt_iso8859_8 (void)
{
  return &converter;
}
