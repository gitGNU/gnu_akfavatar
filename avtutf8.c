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
// returns number of bytes read from utf8
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

// result is not terminated unless len includes the terminator
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
      else if (charnum + 1 < wide_len)	// UTF-16 surrogates
	{
	  avt_char c = ch - 0x10000u;

	  wide[charnum] = 0xD800 bitor ((c >> 10) bitand 0x3FF);
	  ++charnum;
	  wide[charnum] = 0xDC00 bitor (c bitand 0x3FF);
	}

      if (bytes > len)
	bytes = len;

      txt += bytes;
      len -= bytes;
      ++charnum;
    }

  return charnum;
}


static void
wchar_to_utf8 (const wchar_t * txt, size_t len, char *utf8, size_t utf8_size)
{
  size_t p = 0;			// position in utf8 string

  for (size_t i = 0; i < len; ++i)
    {
      register avt_char ch = txt[i];

      // support UTF-16
      // if and only if wchar_t is < 3, otherwise optimized away
      if (sizeof (wchar_t) < 3 and 0xD800u <= ch and ch <= 0xDBFFu)
	{
	  avt_char ch2 = txt[i + 1];

	  if (0xDC00u <= ch2 and ch2 <= 0xDFFFu)
	    {
	      ch = (((ch bitand 0x3FFu) << 10) bitor (ch2 bitand 0x3FFu))
		+ 0x10000u;
	      ++i;
	    }
	}

      if (ch > UNICODE_MAXIMUM or surrogate (ch))
	ch = BROKEN_WCHAR;

      if (ch <= 0x7Fu and p + 1 < utf8_size)
	utf8[p++] = (char) ch;
      else if (ch <= 0x7FFu and p + 2 < utf8_size)
	{
	  utf8[p++] = 0xC0u bitor (ch >> 6);
	  utf8[p++] = 0x80u bitor (ch bitand 0x3Fu);
	}
      else if (ch <= 0xFFFFu and p + 3 < utf8_size)
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
      size_t wide_len = len;

      if (sizeof (wchar_t) < 4)
	wide_len *= 2;		// space for UTF-16 surrogates

      wchar_t wide[wide_len];
      size_t chars = utf8_to_wchar (txt, len, wide, wide_len);
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

      size_t wide_len = len;

      if (sizeof (wchar_t) < 4)
	wide_len *= 2;		// space for UTF-16 surrogates

      wchar_t wide[wide_len];
      size_t chars = utf8_to_wchar (txt, len, wide, wide_len);

      avt_tell_len (wide, chars);
    }

  return _avt_STATUS;
}


extern int
avt_tell_u8 (const char *txt)
{
  if (txt and * txt)
    avt_tell_u8_len (txt, strlen (txt));

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

      size_t wide_len = len;

      if (sizeof (wchar_t) < 4)
	wide_len *= 2;		// space for UTF-16 surrogates

      wchar_t wide[wide_len];

      utf8_to_wchar (name, len, wide, wide_len);
      avt_set_avatar_name (wide);
    }

  return _avt_STATUS;
}

/*
 * The pager and credits may be used with longer texts, so I use malloc.
 * Wide strings are up to 4 bytes per char,
 * in UTF-32 but also in UTF-16, because of surrogate codes.
 */

extern int
avt_pager_u8 (const char *txt, size_t len, int startline)
{
  if (txt and _avt_STATUS == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      wchar_t *wctext = malloc (len * 4);

      if (wctext)
	{
	  size_t chars =
	    utf8_to_wchar (txt, len, wctext, len * 4 / sizeof (wchar_t));
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
      wchar_t *wctext = malloc (len * 4);

      if (wctext)
	{
	  utf8_to_wchar (txt, len, wctext, len * 4 / sizeof (wchar_t));
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
