/*
 * UTF-8 support for AKFAvatar
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
#include "avtinternals.h"

#include <stddef.h>
#include <iso646.h>

#define UNICODE_MAXIMUM  (0x10FFFFu)

#define surrogate(ch)  ((ch) >= 0xD800u and (ch) <= 0xDFFFu)


// check number of bytes in char up to max_bytes
static size_t
check_char_length (const unsigned char *utf8, size_t max_bytes)
{
  size_t byte = 1;		// first byte assumed to be correct

  while (byte < max_bytes and (utf8[byte] bitand 0xC0) == 0x80)
    ++byte;

  return byte;
}


// reads next char from utf8 and places code in ch
// returns number of bytes read from src
static size_t
utf8_to_unicode (const struct avt_charenc *self, avt_char * ch,
		 const char *src)
{
  (void) self;

  size_t bytes = 0;
  avt_char c = BROKEN_WCHAR;
  const unsigned char *u8 = (const unsigned char *) src;

  if (*u8 <= 0x7Fu)
    {
      bytes = 1u;
      c = *u8;
    }
  else if (*u8 <= 0xBFu)
    bytes = 1u;			// runaway continuation byte
  else if (*u8 <= 0xDFu)
    {
      bytes = check_char_length (u8, 2u);
      if (bytes == 2u)
	c = ((u8[0] bitand compl 0xC0u) << 6)
	  bitor (u8[1] bitand compl 0x80u);
    }
  else if (*u8 <= 0xEFu)
    {
      bytes = check_char_length (u8, 3u);
      if (bytes == 3u)
	c = ((u8[0] bitand compl 0xE0u) << (2 * 6))
	  bitor ((u8[1] bitand compl 0x80u) << 6)
	  bitor (u8[2] bitand compl 0x80u);
    }
  else if (*u8 <= 0xF4u)
    {
      bytes = check_char_length (u8, 4u);
      if (bytes == 4u)
	c = ((u8[0] bitand compl 0xF0u) << (3 * 6))
	  bitor ((u8[1] bitand compl 0x80u) << (2 * 6))
	  bitor ((u8[2] bitand compl 0x80u) << 6)
	  bitor (u8[3] bitand compl 0x80u);
    }
  else if (*u8 <= 0xFBu)	// no valid Unicode
    bytes = check_char_length (u8, 5u);
  else if (*u8 <= 0xFDu)	// no valid Unicode
    bytes = check_char_length (u8, 6u);
  else
    bytes = 1;			// skip invalid byte

  // checks for security
  if (c > UNICODE_MAXIMUM or surrogate (c)
      or (bytes >= 2u and c <= 0x7Fu) or (bytes >= 3u and c <= 0x7FFu))
    c = BROKEN_WCHAR;

  *ch = c;

  return bytes;
}


static size_t
utf8_from_unicode (const struct avt_charenc *self, char *dest,
		   size_t dest_size, avt_char ch)
{
  (void) self;

  size_t size = 0;

  // this also ensures that there is never more than 4 bytes
  if (ch > UNICODE_MAXIMUM or surrogate (ch))
    ch = BROKEN_WCHAR;

  if (ch <= 0x7Fu and dest_size >= 1)
    {
      *dest = (char) ch;
      size = 1;
    }
  else if (ch <= 0x7FFu and dest_size >= 2)
    {
      *dest++ = 0xC0u bitor (ch >> 6);
      *dest = 0x80u bitor (ch bitand 0x3Fu);
      size = 2;
    }
  else if (ch <= 0xFFFFu and dest_size >= 3)
    {
      *dest++ = 0xE0u bitor (ch >> (2 * 6));
      *dest++ = 0x80u bitor ((ch >> 6) bitand 0x3Fu);
      *dest = 0x80u bitor (ch bitand 0x3Fu);
      size = 3;
    }
  else if (dest_size >= 4)
    {
      *dest++ = 0xF0u bitor (ch >> (3 * 6));
      *dest++ = 0x80u bitor ((ch >> (2 * 6)) bitand 0x3Fu);
      *dest++ = 0x80u bitor ((ch >> 6) bitand 0x3Fu);
      *dest = 0x80u bitor (ch bitand 0x3Fu);
      size = 4;
    }

  return size;
}

static const struct avt_charenc converter = {
  .data = NULL,
  .decode = utf8_to_unicode,
  .encode = utf8_from_unicode
};

extern const struct avt_charenc *
avt_utf8 (void)
{
  return &converter;
}
