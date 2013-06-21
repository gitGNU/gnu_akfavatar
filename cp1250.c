/*
 * WINDOWS-1250 (central and eastern European) support for AKFAvatar
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
  .start = 0x80,.end = 0xFF,
  .table = {
	    0x20AC,		// 0x80
	    AVT_INVALID_WCHAR,	// 0x81
	    0x201A,		// 0x82
	    AVT_INVALID_WCHAR,	// 0x83
	    0x201E,		// 0x84
	    0x2026,		// 0x85
	    0x2020,		// 0x86
	    0x2021,		// 0x87
	    AVT_INVALID_WCHAR,	// 0x88
	    0x2030,		// 0x89
	    0x0160,		// 0x8A
	    0x2039,		// 0x8B
	    0x015A,		// 0x8C
	    0x0164,		// 0x8D
	    0x017D,		// 0x8E
	    0x0179,		// 0x8F
	    AVT_INVALID_WCHAR,	// 0x90
	    0x2018,		// 0x91
	    0x2019,		// 0x92
	    0x201C,		// 0x93
	    0x201D,		// 0x94
	    0x2022,		// 0x95
	    0x2013,		// 0x96
	    0x2014,		// 0x97
	    AVT_INVALID_WCHAR,	// 0x98
	    0x2122,		// 0x99
	    0x0161,		// 0x9A
	    0x203A,		// 0x9B
	    0x015B,		// 0x9C
	    0x0165,		// 0x9D
	    0x017E,		// 0x9E
	    0x017A,		// 0x9F
	    0x00A0,		// 0xA0
	    0x02C7,		// 0xA1
	    0x02D8,		// 0xA2
	    0x0141,		// 0xA3
	    0x00A4,		// 0xA4
	    0x0104,		// 0xA5
	    0x00A6,		// 0xA6
	    0x00A7,		// 0xA7
	    0x00A8,		// 0xA8
	    0x00A9,		// 0xA9
	    0x015E,		// 0xAA
	    0x00AB,		// 0xAB
	    0x00AC,		// 0xAC
	    0x00AD,		// 0xAD
	    0x00AE,		// 0xAE
	    0x017B,		// 0xAF
	    0x00B0,		// 0xB0
	    0x00B1,		// 0xB1
	    0x02DB,		// 0xB2
	    0x0142,		// 0xB3
	    0x00B4,		// 0xB4
	    0x00B5,		// 0xB5
	    0x00B6,		// 0xB6
	    0x00B7,		// 0xB7
	    0x00B8,		// 0xB8
	    0x0105,		// 0xB9
	    0x015F,		// 0xBA
	    0x00BB,		// 0xBB
	    0x013D,		// 0xBC
	    0x02DD,		// 0xBD
	    0x013E,		// 0xBE
	    0x017C,		// 0xBF
	    0x0154,		// 0xC0
	    0x00C1,		// 0xC1
	    0x00C2,		// 0xC2
	    0x0102,		// 0xC3
	    0x00C4,		// 0xC4
	    0x0139,		// 0xC5
	    0x0106,		// 0xC6
	    0x00C7,		// 0xC7
	    0x010C,		// 0xC8
	    0x00C9,		// 0xC9
	    0x0118,		// 0xCA
	    0x00CB,		// 0xCB
	    0x011A,		// 0xCC
	    0x00CD,		// 0xCD
	    0x00CE,		// 0xCE
	    0x010E,		// 0xCF
	    0x0110,		// 0xD0
	    0x0143,		// 0xD1
	    0x0147,		// 0xD2
	    0x00D3,		// 0xD3
	    0x00D4,		// 0xD4
	    0x0150,		// 0xD5
	    0x00D6,		// 0xD6
	    0x00D7,		// 0xD7
	    0x0158,		// 0xD8
	    0x016E,		// 0xD9
	    0x00DA,		// 0xDA
	    0x0170,		// 0xDB
	    0x00DC,		// 0xDC
	    0x00DD,		// 0xDD
	    0x0162,		// 0xDE
	    0x00DF,		// 0xDF
	    0x0155,		// 0xE0
	    0x00E1,		// 0xE1
	    0x00E2,		// 0xE2
	    0x0103,		// 0xE3
	    0x00E4,		// 0xE4
	    0x013A,		// 0xE5
	    0x0107,		// 0xE6
	    0x00E7,		// 0xE7
	    0x010D,		// 0xE8
	    0x00E9,		// 0xE9
	    0x0119,		// 0xEA
	    0x00EB,		// 0xEB
	    0x011B,		// 0xEC
	    0x00ED,		// 0xED
	    0x00EE,		// 0xEE
	    0x010F,		// 0xEF
	    0x0111,		// 0xF0
	    0x0144,		// 0xF1
	    0x0148,		// 0xF2
	    0x00F3,		// 0xF3
	    0x00F4,		// 0xF4
	    0x0151,		// 0xF5
	    0x00F6,		// 0xF6
	    0x00F7,		// 0xF7
	    0x0159,		// 0xF8
	    0x016F,		// 0xF9
	    0x00FA,		// 0xFA
	    0x0171,		// 0xFB
	    0x00FC,		// 0xFC
	    0x00FD,		// 0xFD
	    0x0163,		// 0xFE
	    0x02D9		// 0xFF
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &map,
  .decode = map_to_unicode,
  .encode = map_from_unicode
};


extern const struct avt_charenc *
avt_cp1250 (void)
{
  return &converter;
}
