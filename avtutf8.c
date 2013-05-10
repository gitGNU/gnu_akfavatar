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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <iso646.h>

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
// returns number of bytes read from utf8 or 0 on fatal error
static size_t
utf8_to_unicode (const char *utf8, avt_char * ch)
{
  size_t bytes = 0;
  avt_char c = BROKEN_WCHAR;
  const unsigned char *u8 = (const unsigned char *) utf8;

  if (*u8 <= 0x7Fu)
    {
      bytes = 1u;
      c = *u8;
    }
  else if (*u8 <= 0xBFu)
    bytes = 1u;			// runaway continuation byte
  else if (*u8 <= 0xDFu)
    {
      bytes = check_char_length (u8, 2);
      if (bytes == 2u)
	c = ((u8[0] bitand compl 0xC0u) << 6)
	  bitor (u8[1] bitand compl 0x80u);
    }
  else if (*u8 <= 0xEFu)
    {
      bytes = check_char_length (u8, 3);
      if (bytes == 3u)
	c = ((u8[0] bitand compl 0xE0u) << (2 * 6))
	  bitor ((u8[1] bitand compl 0x80u) << 6)
	  bitor (u8[2] bitand compl 0x80u);
    }
  else if (*u8 <= 0xF4u)
    {
      bytes = check_char_length (u8, 4);
      if (bytes == 4u)
	c = ((u8[0] bitand compl 0xF0u) << (3 * 6))
	  bitor ((u8[1] bitand compl 0x80u) << (2 * 6))
	  bitor ((u8[2] bitand compl 0x80u) << 6)
	  bitor (u8[3] bitand compl 0x80u);
    }
  else if (*u8 <= 0xFBu)	// no valid Unicode
    bytes = check_char_length (u8, 5);
  else if (*u8 <= 0xFDu)	// no valid Unicode
    bytes = check_char_length (u8, 6);
  else
    bytes = 1;			// skip invalid byte

  // checks for security
  if (c > 0x10FFFFu
      or (c >= 0xD800u and c <= 0xDFFFu)
      or (bytes >= 2u and c <= 0x7Fu) or (bytes >= 3u and c <= 0x7FFu))
    c = BROKEN_WCHAR;

  if (ch)
    *ch = c;

  return bytes;
}

// result is not terminated unless len includes the terminator
// no support for UTF-16 surrogates!
// that would make things only more complicated
static size_t
utf8_to_wchar (const char *txt, size_t len, wchar_t * wide, size_t wide_len)
{
  size_t charnum = 0;

  while (len and charnum < wide_len)
    {
      avt_char ch;
      size_t bytes = utf8_to_unicode (txt, &ch);
      if (not bytes)
	break;

      if (sizeof (wchar_t) >= 3 or ch <= 0xFFFFu)
	wide[charnum] = ch;
      else
	wide[charnum] = BROKEN_WCHAR;

      if (bytes > len)
	bytes = len;

      txt += bytes;
      len -= bytes;
      ++charnum;
    }

  return charnum;
}


// again no support for UTF-16 surrogates! (marked as broken chars)
static void
wchar_to_utf8 (const wchar_t * txt, size_t len, char *utf8, size_t utf8_size)
{
  size_t p = 0;			// position in utf8 string

  for (size_t i = 0; i < len; ++i)
    {
      register wchar_t ch = txt[i];

      if (ch > 0x10FFFF or (ch >= 0xD800 and ch <= 0xDFFF))
	ch = BROKEN_WCHAR;

      if (ch <= L'\x7F' and p + 1 < utf8_size)
	utf8[p++] = (char) ch;
      else if (ch <= L'\x7FF' and p + 2 < utf8_size)
	{
	  utf8[p++] = 0xC0u bitor (ch >> 6);
	  utf8[p++] = 0x80u bitor (ch bitand 0x3Fu);
	}
      else if (ch <= L'\xFFFF' and p + 3 < utf8_size)
	{
	  utf8[p++] = 0xE0u bitor (ch >> (2 * 6));
	  utf8[p++] = 0x80u bitor ((ch >> 6) bitand 0x3Fu);
	  utf8[p++] = 0x80u bitor (ch bitand 0x3Fu);
	}
      else if (p + 4 < utf8_size)
	{
	  utf8[p++] = 0xF0u bitor (ch >> (3 * 6));
	  utf8[p++] = 0x80u bitor ((ch >> (2 * 6)) bitand 0x3Fu);
	  utf8[p++] = 0x80u bitor ((ch >> 6) bitand 0x3Fu);
	  utf8[p++] = 0x80u bitor (ch bitand 0x3Fu);
	}

      if (not ch)
	break;
    }

  utf8[utf8_size - 1] = '\0';
}


extern int
avt_say_u8_len (const char *txt, size_t len)
{
  // nothing to do, when txt == NULL
  // but do allow a text to start with zeros here
  if (not txt or _avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return avt_update ();

  if (len)
    {
      wchar_t wide[len];
      size_t chars = utf8_to_wchar (txt, len, wide, len);
      avt_say_len (wide, chars);
    }

  return _avt_STATUS;
}


extern int
avt_say_u8 (const char *txt)
{
  if (txt and * txt)
    avt_say_u8_len (txt, strlen (txt));

  return _avt_STATUS;
}


extern int
avt_tell_u8_len (const char *txt, size_t len)
{
  if (txt)
    {
      if (not len or len > 0x80000000u)
	len = strlen (txt);

      wchar_t wide[len];
      size_t chars = utf8_to_wchar (txt, len, wide, len);

      avt_tell_len (wide, chars);
    }

  return _avt_STATUS;
}


extern int
avt_tell_u8 (const char *txt)
{
  if (txt and * txt)
    avt_tell_u8_len (txt, 0);

  return _avt_STATUS;
}


extern int
avt_set_avatar_name_u8 (const char *name)
{
  if (not name or not * name)
    avt_set_avatar_name (NULL);
  else
    {
      size_t len = strlen (name) + 1;	// with terminator
      wchar_t wide[len];

      utf8_to_wchar (name, len, wide, len);
      avt_set_avatar_name (wide);
    }

  return _avt_STATUS;
}


extern int
avt_pager_u8 (const char *txt, size_t len, int startline)
{
  if (txt and _avt_STATUS == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      wchar_t *wctext = malloc (len * sizeof (wchar_t));

      if (wctext)
	{
	  size_t chars = utf8_to_wchar (txt, len, wctext, len);
	  avt_pager (wctext, chars, startline);
	  free (wctext);
	}
    }

  return _avt_STATUS;
}


extern int
avt_credits_u8 (const char *txt, bool centered)
{
  if (_avt_STATUS == AVT_NORMAL and txt and * txt and avt_initialized ())
    {
      size_t len = strlen (txt) + 1;	// with terminator
      wchar_t *wctext = malloc (len * sizeof (wchar_t));

      if (wctext)
	{
	  utf8_to_wchar (txt, len, wctext, len);
	  avt_credits (wctext, centered);
	  free (wctext);
	}
    }

  return _avt_STATUS;
}


extern int
avt_ask_u8 (char *s, size_t size)
{
  if (s and size)
    {
      memset (s, '\0', size);

      wchar_t buf[size];

      if (avt_ask (buf, sizeof (buf)) == AVT_NORMAL)
	wchar_to_utf8 (buf, sizeof (buf), s, size);
    }

  return _avt_STATUS;
}
