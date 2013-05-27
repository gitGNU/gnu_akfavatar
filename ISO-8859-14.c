/*
 * ISO-8859-14 (ISO Latin-8) support for AKFAvatar
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
	    0x1E02,		// 0xA1
	    0x1E03,		// 0xA2
	    0x00A3,		// 0xA3
	    0x010A,		// 0xA4
	    0x010B,		// 0xA5
	    0x1E0A,		// 0xA6
	    0x00A7,		// 0xA7
	    0x1E80,		// 0xA8
	    0x00A9,		// 0xA9
	    0x1E82,		// 0xAA
	    0x1E0B,		// 0xAB
	    0x1EF2,		// 0xAC
	    0x00AD,		// 0xAD
	    0x00AE,		// 0xAE
	    0x0178,		// 0xAF
	    0x1E1E,		// 0xB0
	    0x1E1F,		// 0xB1
	    0x0120,		// 0xB2
	    0x0121,		// 0xB3
	    0x1E40,		// 0xB4
	    0x1E41,		// 0xB5
	    0x00B6,		// 0xB6
	    0x1E56,		// 0xB7
	    0x1E81,		// 0xB8
	    0x1E57,		// 0xB9
	    0x1E83,		// 0xBA
	    0x1E60,		// 0xBB
	    0x1EF3,		// 0xBC
	    0x1E84,		// 0xBD
	    0x1E85,		// 0xBE
	    0x1E61,		// 0xBF
	    0x00C0,		// 0xC0
	    0x00C1,		// 0xC1
	    0x00C2,		// 0xC2
	    0x00C3,		// 0xC3
	    0x00C4,		// 0xC4
	    0x00C5,		// 0xC5
	    0x00C6,		// 0xC6
	    0x00C7,		// 0xC7
	    0x00C8,		// 0xC8
	    0x00C9,		// 0xC9
	    0x00CA,		// 0xCA
	    0x00CB,		// 0xCB
	    0x00CC,		// 0xCC
	    0x00CD,		// 0xCD
	    0x00CE,		// 0xCE
	    0x00CF,		// 0xCF
	    0x0174,		// 0xD0
	    0x00D1,		// 0xD1
	    0x00D2,		// 0xD2
	    0x00D3,		// 0xD3
	    0x00D4,		// 0xD4
	    0x00D5,		// 0xD5
	    0x00D6,		// 0xD6
	    0x1E6A,		// 0xD7
	    0x00D8,		// 0xD8
	    0x00D9,		// 0xD9
	    0x00DA,		// 0xDA
	    0x00DB,		// 0xDB
	    0x00DC,		// 0xDC
	    0x00DD,		// 0xDD
	    0x0176,		// 0xDE
	    0x00DF,		// 0xDF
	    0x00E0,		// 0xE0
	    0x00E1,		// 0xE1
	    0x00E2,		// 0xE2
	    0x00E3,		// 0xE3
	    0x00E4,		// 0xE4
	    0x00E5,		// 0xE5
	    0x00E6,		// 0xE6
	    0x00E7,		// 0xE7
	    0x00E8,		// 0xE8
	    0x00E9,		// 0xE9
	    0x00EA,		// 0xEA
	    0x00EB,		// 0xEB
	    0x00EC,		// 0xEC
	    0x00ED,		// 0xED
	    0x00EE,		// 0xEE
	    0x00EF,		// 0xEF
	    0x0175,		// 0xF0
	    0x00F1,		// 0xF1
	    0x00F2,		// 0xF2
	    0x00F3,		// 0xF3
	    0x00F4,		// 0xF4
	    0x00F5,		// 0xF5
	    0x00F6,		// 0xF6
	    0x1E6B,		// 0xF7
	    0x00F8,		// 0xF8
	    0x00F9,		// 0xF9
	    0x00FA,		// 0xFA
	    0x00FB,		// 0xFB
	    0x00FC,		// 0xFC
	    0x00FD,		// 0xFD
	    0x0177,		// 0xFE
	    0x00FF,		// 0xFF
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &map,
  .to_unicode = map_to_unicode,
  .from_unicode = map_from_unicode
};


extern const struct avt_charenc *
avt_iso8859_14 (void)
{
  return &converter;
}
