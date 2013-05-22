/*
 * character encoding support for AKFAvatar
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
#include "avtinternals.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <iso646.h>


static struct avt_charenc *convert;


extern struct avt_charenc *
avt_charencoding (struct avt_charenc *encoding)
{
  struct avt_charenc *old;

  old = convert;

  if (encoding)
    convert = encoding;

  return old;
}


// result is not terminated unless len includes the terminator
static size_t
char_to_wchar (wchar_t * dest, const char *src, size_t len)
{
  size_t characters = 0;

  while (len)
    {
      avt_char ch;
      size_t num = convert->to_unicode (&ch, src);

      if (sizeof (wchar_t) >= 3 or ch <= 0xFFFFu)
	*dest = (wchar_t) ch;
      else			// UTF-16 surrogates
	{
	  ch -= 0x10000u;
	  *dest = 0xD800 bitor ((ch >> 10) bitand 0x3FF);
	  ++dest;
	  *dest = 0xDC00 bitor (ch bitand 0x3FF);
	  ++characters;
	}

      ++dest;
      ++characters;

      if (num > len)
	break;

      src += num;
      len -= num;
    }

  return characters;
}


static void
wchar_to_char (char *dest, const wchar_t * src, size_t len)
{
  while (len)
    {
      avt_char ch = (avt_char) * src;

      // support UTF-16
      // if and only if wchar_t is < 3, otherwise optimized away
      if (sizeof (wchar_t) < 3 and 0xD800u <= ch and ch <= 0xDBFFu)
	{
	  avt_char ch2 = *(src + 1);

	  if (0xDC00u <= ch2 and ch2 <= 0xDFFFu)
	    {
	      ch = (((ch bitand 0x3FFu) << 10) bitor (ch2 bitand 0x3FFu))
		+ 0x10000u;

	      ++src;
	    }
	}

      size_t bytes = convert->from_unicode (dest, ch);

      if (not * src)
	break;

      ++src;
      dest += bytes;
      --len;
    }
}


extern int
avt_say_char_len (const char *txt, size_t len)
{
  int status = avt_get_status ();

  // nothing to do when not txt
  // but do allow a text to start with zeros here
  if (not convert or not txt or status != AVT_NORMAL
      or not avt_initialized ())
    return avt_update ();

  while (len)
    {
      avt_char ch;
      size_t num = convert->to_unicode (&ch, txt);

      status = avt_put_char (ch);
      if (status != AVT_NORMAL or num > len)
	break;

      txt += num;
      len -= num;
    }

  return status;
}


extern int
avt_say_char (const char *txt)
{
  int status = avt_get_status ();

  if (not avt_initialized ())
    return status;

  if (not convert or not txt or not * txt)
    return avt_update ();

  while (*txt)
    {
      avt_char ch;
      size_t num = convert->to_unicode (&ch, txt);

      status = avt_put_char (ch);
      if (status != AVT_NORMAL)
	break;

      txt += num;
    }

  return status;
}


extern int
avt_tell_char_len (const char *txt, size_t len)
{
  int status = avt_get_status ();

  if (convert and txt and status == AVT_NORMAL)
    {
      if (not len or len > 0x80000000)
	len = strlen (txt);

      wchar_t wide[len * (4 / sizeof (wchar_t))];
      size_t chars = char_to_wchar (wide, txt, len);
      status = avt_tell_len (wide, chars);
    }

  return status;
}


extern int
avt_tell_char (const char *txt)
{
  int status = avt_get_status ();

  if (convert and txt and * txt)
    status = avt_tell_char_len (txt, strlen (txt));

  return status;
}


extern int
avt_set_avatar_name_char (const char *name)
{
  int status;

  if (not name or not * name)
    status = avt_set_avatar_name (NULL);
  else if (not convert)
    status = avt_get_status ();
  else
    {
      size_t len = strlen (name);

      wchar_t wide[(len + 1) * (4 / sizeof (wchar_t))];

      char_to_wchar (wide, name, len + 1);
      status = avt_set_avatar_name (wide);
    }

  return status;
}

/*
 * The pager and credits may be used with longer texts, so I use malloc.
 * Wide strings are up to 4 bytes per char,
 * in UTF-32 but also in UTF-16, because of surrogate codes.
 */

extern int
avt_pager_char (const char *txt, size_t len, int startline)
{
  int status = avt_get_status ();

  if (convert and txt and status == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      // no terminator needed
      wchar_t *wctext = malloc (len * 4);

      if (wctext)
	{
	  size_t chars = char_to_wchar (wctext, txt, len);
	  status = avt_pager (wctext, chars, startline);
	  free (wctext);
	}
    }

  return status;
}


extern int
avt_credits_char (const char *txt, bool centered)
{
  int status = avt_get_status ();

  if (status == AVT_NORMAL and convert and
      txt and * txt and avt_initialized ())
    {
      size_t len = strlen (txt);

      wchar_t *wctext = malloc ((len + 1) * 4);

      if (wctext)
	{
	  char_to_wchar (wctext, txt, len + 1);
	  status = avt_credits (wctext, centered);
	  free (wctext);
	}
    }

  return status;
}


extern avt_char
avt_input_char (char *s, size_t size, const char *default_text,
		int position, int mode)
{
  int status = avt_get_status ();
  avt_char ch = AVT_KEY_NONE;

  if (s and size)
    {
      wchar_t buf[size];
      wchar_t wcs_default_text[(AVT_LINELENGTH + 1) * (4 / sizeof (wchar_t))];

      memset (s, '\0', size);
      wcs_default_text[0] = L'\0';

      if (default_text and * default_text)
	{
	  size_t len = strlen (default_text);
	  if (len > AVT_LINELENGTH)
	    len = AVT_LINELENGTH;

	  char_to_wchar (wcs_default_text, default_text, len);
	  wcs_default_text[len] = L'\0';
	}

      ch = avt_input (buf, sizeof (buf), wcs_default_text, position, mode);

      status = avt_get_status ();
      if (status != AVT_NORMAL)
	return AVT_KEY_NONE;

      // size - 1 to keep the terminator
      wchar_to_char (s, buf, size - 1);
    }

  return ch;
}


extern int
avt_ask_char (char *s, size_t size)
{
  if (s and size)
    avt_input_char (s, size, NULL, -1, 0);

  return avt_get_status ();
}
