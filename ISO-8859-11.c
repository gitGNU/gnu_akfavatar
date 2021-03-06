/*
 * ISO-8859-11 (Thai) support for AKFAvatar
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

#include "avtaddons.h"

static const struct avt_char_map map = {
  .start = 0xA1,.end = 0xFF,
  .table = {
	    0x0E01,		// 0xA1
	    0x0E02,		// 0xA2
	    0x0E03,		// 0xA3
	    0x0E04,		// 0xA4
	    0x0E05,		// 0xA5
	    0x0E06,		// 0xA6
	    0x0E07,		// 0xA7
	    0x0E08,		// 0xA8
	    0x0E09,		// 0xA9
	    0x0E0A,		// 0xAA
	    0x0E0B,		// 0xAB
	    0x0E0C,		// 0xAC
	    0x0E0D,		// 0xAD
	    0x0E0E,		// 0xAE
	    0x0E0F,		// 0xAF
	    0x0E10,		// 0xB0
	    0x0E11,		// 0xB1
	    0x0E12,		// 0xB2
	    0x0E13,		// 0xB3
	    0x0E14,		// 0xB4
	    0x0E15,		// 0xB5
	    0x0E16,		// 0xB6
	    0x0E17,		// 0xB7
	    0x0E18,		// 0xB8
	    0x0E19,		// 0xB9
	    0x0E1A,		// 0xBA
	    0x0E1B,		// 0xBB
	    0x0E1C,		// 0xBC
	    0x0E1D,		// 0xBD
	    0x0E1E,		// 0xBE
	    0x0E1F,		// 0xBF
	    0x0E20,		// 0xC0
	    0x0E21,		// 0xC1
	    0x0E22,		// 0xC2
	    0x0E23,		// 0xC3
	    0x0E24,		// 0xC4
	    0x0E25,		// 0xC5
	    0x0E26,		// 0xC6
	    0x0E27,		// 0xC7
	    0x0E28,		// 0xC8
	    0x0E29,		// 0xC9
	    0x0E2A,		// 0xCA
	    0x0E2B,		// 0xCB
	    0x0E2C,		// 0xCC
	    0x0E2D,		// 0xCD
	    0x0E2E,		// 0xCE
	    0x0E2F,		// 0xCF
	    0x0E30,		// 0xD0
	    0x0E31,		// 0xD1
	    0x0E32,		// 0xD2
	    0x0E33,		// 0xD3
	    0x0E34,		// 0xD4
	    0x0E35,		// 0xD5
	    0x0E36,		// 0xD6
	    0x0E37,		// 0xD7
	    0x0E38,		// 0xD8
	    0x0E39,		// 0xD9
	    0x0E3A,		// 0xDA
	    AVT_INVALID_WCHAR,	// 0xDB
	    AVT_INVALID_WCHAR,	// 0xDC
	    AVT_INVALID_WCHAR,	// 0xDD
	    AVT_INVALID_WCHAR,	// 0xDE
	    0x0E3F,		// 0xDF
	    0x0E40,		// 0xE0
	    0x0E41,		// 0xE1
	    0x0E42,		// 0xE2
	    0x0E43,		// 0xE3
	    0x0E44,		// 0xE4
	    0x0E45,		// 0xE5
	    0x0E46,		// 0xE6
	    0x0E47,		// 0xE7
	    0x0E48,		// 0xE8
	    0x0E49,		// 0xE9
	    0x0E4A,		// 0xEA
	    0x0E4B,		// 0xEB
	    0x0E4C,		// 0xEC
	    0x0E4D,		// 0xED
	    0x0E4E,		// 0xEE
	    0x0E4F,		// 0xEF
	    0x0E50,		// 0xF0
	    0x0E51,		// 0xF1
	    0x0E52,		// 0xF2
	    0x0E53,		// 0xF3
	    0x0E54,		// 0xF4
	    0x0E55,		// 0xF5
	    0x0E56,		// 0xF6
	    0x0E57,		// 0xF7
	    0x0E58,		// 0xF8
	    0x0E59,		// 0xF9
	    0x0E5A,		// 0xFA
	    0x0E5B,		// 0xFB
	    AVT_INVALID_WCHAR,	// 0xFC
	    AVT_INVALID_WCHAR,	// 0xFD
	    AVT_INVALID_WCHAR,	// 0xFE
	    AVT_INVALID_WCHAR	// 0xFF
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &map,
  .decode = map_to_unicode,
  .encode = map_from_unicode
};


extern const struct avt_charenc *
avt_iso8859_11 (void)
{
  return &converter;
}
