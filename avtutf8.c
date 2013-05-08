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

// returns number of individual UTF-8 characters
// in a '\0' terminated string
// (wrong on invalid encodings)
static size_t
utf8_chars (const char *text)
{
  size_t len = 0;
  const unsigned char *u = (const unsigned char *) text;

  while (*u)
    {
      if ((*u bitand 0xC0) != 0x80)
	++len;

      ++u;
    }

  return len;
}


// returns number of individual UTF-8 characters
// in a string with a given size
// (wrong on invalid encodings)
static size_t
utf8_chars_len (const char *text, size_t size)
{
  size_t len = 0;
  const unsigned char *u = (const unsigned char *) text;

  for (size_t i = 0; i < size; ++i)
    {
      if ((*u bitand 0xC0) != 0x80)
	++len;

      ++u;
    }

  return len;
}


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

  if (not utf8)
    return 0;

  if (*u8 <= 0x7F)
    {
      bytes = 1;
      c = *u8;
    }
  else if (*u8 <= 0xBF)
    bytes = 1;			// runaway continuation byte
  else if (*u8 <= 0xDF)
    {
      bytes = check_char_length (u8, 2);
      if (bytes == 2)
	c = ((u8[0] bitand compl 0xC0) << 6) bitor (u8[1] bitand compl 0x80);
    }
  else if (*u8 <= 0xEF)
    {
      bytes = check_char_length (u8, 3);
      if (bytes == 3)
	c = ((u8[0] bitand compl 0xE0) << (2 * 6))
	  bitor ((u8[1] bitand compl 0x80) << 6)
	  bitor (u8[2] bitand compl 0x80);
    }
  else if (*u8 <= 0xF4)
    {
      bytes = check_char_length (u8, 4);
      if (bytes == 4)
	c = ((u8[0] bitand compl 0xF0) << (3 * 6))
	  bitor ((u8[1] bitand compl 0x80) << (2 * 6))
	  bitor ((u8[2] bitand compl 0x80) << 6)
	  bitor (u8[3] bitand compl 0x80);
    }
  else if (*u8 <= 0xFB)		// no valid Unicode
    bytes = check_char_length (u8, 5);
  else if (*u8 <= 0xFD)		// no valid Unicode
    bytes = check_char_length (u8, 6);
  else
    bytes = 1;			// skip invalid byte

  // checks for security
  if (c > 0x10FFFF
      or (bytes >= 2 and c <= 0x7F) or (bytes >= 3 and c <= 0x7FF))
    c = BROKEN_WCHAR;

  if (ch)
    *ch = c;

  return bytes;
}

// no support for UTF-16 surrogates!
// that would make things only more complicated
static void
utf8_to_wchar (const char *txt, size_t len, wchar_t * wide, size_t wide_len)
{
  size_t charnum = 0;

  while (len and charnum < wide_len)
    {
      avt_char ch;
      size_t bytes = utf8_to_unicode (txt, &ch);
      if (not bytes)
	break;

      if (sizeof (wchar_t) >= 3 or ch <= 0xFFFF)
	wide[charnum] = ch;
      else
	wide[charnum] = BROKEN_WCHAR;

      if (bytes > len)
	bytes = len;

      txt += bytes;
      len -= bytes;
      ++charnum;
    }

  wide[wide_len - 1] = L'\0';
}


static void
wchar_to_utf8 (const wchar_t * txt, size_t len, char *utf8, size_t utf8_size)
{
  memset (utf8, '\0', utf8_size);

  size_t p = 0;			// position in utf8 string

  for (size_t i = 0; i < len; ++i)
    {
      register wchar_t ch = txt[i];

      if (ch > 0x10FFFF)
	ch = BROKEN_WCHAR;

      if (ch <= L'\x7F' and p + 1 < utf8_size)
	utf8[p++] = (char) ch;
      else if (ch <= L'\x7FF' and p + 2 < utf8_size)
	{
	  utf8[p++] = 0xC0 bitor (ch >> 6);
	  utf8[p++] = 0x80 bitor (ch bitand 0x3F);
	}
      else if (ch <= L'\xFFFF' and p + 3 < utf8_size)
	{
	  utf8[p++] = 0xE0 bitor (ch >> (2 * 6));
	  utf8[p++] = 0x80 bitor ((ch >> 6) bitand 0x3F);
	  utf8[p++] = 0x80 bitor (ch bitand 0x3F);
	}
      else if (p + 4 < utf8_size)
	{
	  utf8[p++] = 0xF0 bitor (ch >> (3 * 6));
	  utf8[p++] = 0x80 bitor ((ch >> (2 * 6)) bitand 0x3F);
	  utf8[p++] = 0x80 bitor ((ch >> 6) bitand 0x3F);
	  utf8[p++] = 0x80 bitor (ch bitand 0x3F);
	}

      if (not ch)
	break;
    }

  utf8[utf8_size - 1] = '\0';
}


extern int
avt_say_u8 (const char *txt)
{
  if (_avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return _avt_STATUS;

  // nothing to do, when there is no text 
  if (not txt or not * txt)
    return avt_update ();

  while (*txt)
    {
      avt_char ch;

      size_t bytes = utf8_to_unicode (txt, &ch);
      if (not bytes or avt_put_char (ch) != AVT_NORMAL)
	break;

      txt += bytes;
    }

  return _avt_STATUS;
}


extern int
avt_say_u8_len (const char *txt, size_t len)
{
  // nothing to do, when txt == NULL
  // but do allow a text to start with zeros here
  if (not txt or _avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return avt_update ();

  while (len)
    {
      avt_char ch;
      size_t bytes = utf8_to_unicode (txt, &ch);
      if (not bytes or avt_put_char (ch) != AVT_NORMAL or bytes > len)
	break;

      txt += bytes;
      len -= bytes;
    }

  return _avt_STATUS;
}


extern int
avt_tell_u8_len (const char *txt, size_t len)
{
  if (txt)
    {
      if (not len or len > 0x80000000)
	len = strlen (txt);

      size_t chars = utf8_chars_len (txt, len);

      wchar_t wide[chars + 1];
      utf8_to_wchar (txt, len, wide, chars + 1);

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
      size_t len = strlen (name);
      size_t chars = utf8_chars_len (name, len);
      wchar_t wide[chars + 1];

      utf8_to_wchar (name, len, wide, chars + 1);

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

      size_t chars = utf8_chars_len (txt, len);
      wchar_t *wctext = malloc ((chars + 1) * sizeof (wchar_t));

      if (wctext)
	{
	  utf8_to_wchar (txt, len, wctext, chars + 1);
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
      size_t chars = utf8_chars (txt);
      wchar_t *wctext = malloc ((chars + 1) * sizeof (wchar_t));

      if (wctext)
	{
	  utf8_to_wchar (txt, strlen (txt), wctext, chars + 1);
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
      wchar_t buf[size];

      if (avt_ask (buf, sizeof (buf)) != AVT_NORMAL)
	return _avt_STATUS;

      wchar_to_utf8 (buf, sizeof (buf), s, size);
    }

  return _avt_STATUS;
}
