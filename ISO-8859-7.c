/*
 * ISO-8859-7 (Greek) support for AKFAvatar
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
	    0x2018,		// 0xA1
	    0x2019,		// 0xA2
	    0x00A3,		// 0xA3
	    0x20AC,		// 0xA4
	    0x20AF,		// 0xA5
	    0x00A6,		// 0xA6
	    0x00A7,		// 0xA7
	    0x00A8,		// 0xA8
	    0x00A9,		// 0xA9
	    0x037A,		// 0xAA
	    0x00AB,		// 0xAB
	    0x00AC,		// 0xAC
	    0x00AD,		// 0xAD
	    AVT_INVALID_WCHAR,	// 0xAE
	    0x2015,		// 0xAF
	    0x00B0,		// 0xB0
	    0x00B1,		// 0xB1
	    0x00B2,		// 0xB2
	    0x00B3,		// 0xB3
	    0x0384,		// 0xB4
	    0x0385,		// 0xB5
	    0x0386,		// 0xB6
	    0x00B7,		// 0xB7
	    0x0388,		// 0xB8
	    0x0389,		// 0xB9
	    0x038A,		// 0xBA
	    0x00BB,		// 0xBB
	    0x038C,		// 0xBC
	    0x00BD,		// 0xBD
	    0x038E,		// 0xBE
	    0x038F,		// 0xBF
	    0x0390,		// 0xC0
	    0x0391,		// 0xC1
	    0x0392,		// 0xC2
	    0x0393,		// 0xC3
	    0x0394,		// 0xC4
	    0x0395,		// 0xC5
	    0x0396,		// 0xC6
	    0x0397,		// 0xC7
	    0x0398,		// 0xC8
	    0x0399,		// 0xC9
	    0x039A,		// 0xCA
	    0x039B,		// 0xCB
	    0x039C,		// 0xCC
	    0x039D,		// 0xCD
	    0x039E,		// 0xCE
	    0x039F,		// 0xCF
	    0x03A0,		// 0xD0
	    0x03A1,		// 0xD1
	    AVT_INVALID_WCHAR,	// 0xD2
	    0x03A3,		// 0xD3
	    0x03A4,		// 0xD4
	    0x03A5,		// 0xD5
	    0x03A6,		// 0xD6
	    0x03A7,		// 0xD7
	    0x03A8,		// 0xD8
	    0x03A9,		// 0xD9
	    0x03AA,		// 0xDA
	    0x03AB,		// 0xDB
	    0x03AC,		// 0xDC
	    0x03AD,		// 0xDD
	    0x03AE,		// 0xDE
	    0x03AF,		// 0xDF
	    0x03B0,		// 0xE0
	    0x03B1,		// 0xE1
	    0x03B2,		// 0xE2
	    0x03B3,		// 0xE3
	    0x03B4,		// 0xE4
	    0x03B5,		// 0xE5
	    0x03B6,		// 0xE6
	    0x03B7,		// 0xE7
	    0x03B8,		// 0xE8
	    0x03B9,		// 0xE9
	    0x03BA,		// 0xEA
	    0x03BB,		// 0xEB
	    0x03BC,		// 0xEC
	    0x03BD,		// 0xED
	    0x03BE,		// 0xEE
	    0x03BF,		// 0xEF
	    0x03C0,		// 0xF0
	    0x03C1,		// 0xF1
	    0x03C2,		// 0xF2
	    0x03C3,		// 0xF3
	    0x03C4,		// 0xF4
	    0x03C5,		// 0xF5
	    0x03C6,		// 0xF6
	    0x03C7,		// 0xF7
	    0x03C8,		// 0xF8
	    0x03C9,		// 0xF9
	    0x03CA,		// 0xFA
	    0x03CB,		// 0xFB
	    0x03CC,		// 0xFC
	    0x03CD,		// 0xFD
	    0x03CE,		// 0xFE
	    AVT_INVALID_WCHAR	// 0xFF
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &map,
  .decode = map_to_unicode,
  .encode = map_from_unicode
};


extern const struct avt_charenc *
avt_iso8859_7 (void)
{
  return &converter;
}
