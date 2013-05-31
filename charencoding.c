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


static const struct avt_charenc *convert;


extern const struct avt_charenc *
avt_charencoding (const struct avt_charenc *encoding)
{
  const struct avt_charenc *old;

  old = convert;

  if (encoding)
    convert = encoding;

  return old;
}


// result is not terminated unless len includes the terminator
static size_t
char_to_wchar (wchar_t * dest, size_t dest_len,
	       const char *src, size_t src_len)
{
  size_t characters = 0;

  while (src_len and dest_len)
    {
      avt_char ch;
      size_t num = convert->decode (convert, &ch, src);

      if (sizeof (wchar_t) >= 3 or ch <= 0xFFFFu)
	*dest = (wchar_t) ch;
      else if (dest_len > 1)	// UTF-16 surrogates
	{
	  ch -= 0x10000u;
	  *dest = 0xD800 bitor ((ch >> 10) bitand 0x3FF);
	  ++dest;
	  --dest_len;
	  *dest = 0xDC00 bitor (ch bitand 0x3FF);
	  ++characters;
	}

      ++dest;
      --dest_len;
      ++characters;

      if (num > src_len)
	break;

      src += num;
      src_len -= num;
    }

  return characters;
}


static void
wchar_to_char (char *dest, size_t dest_len,
	       const wchar_t * src, size_t src_len)
{
  while (src_len and dest_len)
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
	      --src_len;
	    }
	}

      size_t bytes = convert->encode (convert, dest, dest_len, ch);

      if (not bytes)
	break;

      dest += bytes;
      dest_len -= bytes;
      ++src;
      --src_len;
    }
}


extern int
avt_say_char_len (const char *txt, size_t len)
{
  int status = avt_get_status ();

  // nothing to do when not txt
  // but do allow a text to start with zeros here
  if (not convert or not convert->decode
      or not txt or status != AVT_NORMAL or not avt_initialized ())
    return avt_update ();

  while (len)
    {
      avt_char ch;
      size_t num = convert->decode (convert, &ch, txt);

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

  if (not convert or not convert->decode or not txt or not * txt)
    return avt_update ();

  while (*txt)
    {
      avt_char ch;
      size_t num = convert->decode (convert, &ch, txt);

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

  if (convert and convert->decode and txt and status == AVT_NORMAL)
    {
      if (not len or len > 0x80000000)
	len = strlen (txt);

      size_t wlen = len * 4 / sizeof (wchar_t);
      wchar_t wide[wlen];
      size_t chars = char_to_wchar (wide, wlen, txt, len);
      status = avt_tell_len (wide, chars);
    }

  return status;
}


extern int
avt_tell_char (const char *txt)
{
  int status = avt_get_status ();

  if (convert and convert->decode and txt and * txt)
    status = avt_tell_char_len (txt, strlen (txt));

  return status;
}


extern int
avt_set_avatar_name_char (const char *name)
{
  int status;

  if (not name or not * name)
    status = avt_set_avatar_name (NULL);
  else if (not convert or not convert->decode)
    status = avt_get_status ();
  else
    {
      size_t len = strlen (name);
      size_t wlen = (len + 1) * 4 / sizeof (wchar_t);
      wchar_t wide[wlen];

      char_to_wchar (wide, wlen, name, len + 1);
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

  if (convert and convert->decode
      and txt and status == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      // no terminator needed
      wchar_t *wctext = malloc (len * 4);

      if (wctext)
	{
	  size_t wlen = len * 4 / sizeof (wchar_t);
	  size_t chars = char_to_wchar (wctext, wlen, txt, len);
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

  if (status == AVT_NORMAL and convert and convert->decode
      and txt and * txt and avt_initialized ())
    {
      size_t len = strlen (txt);

      wchar_t *wctext = malloc ((len + 1) * 4);

      if (wctext)
	{
	  size_t wlen = (len + 1) * 4 / sizeof (wchar_t);
	  char_to_wchar (wctext, wlen, txt, len + 1);
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

  if (s and size and convert and convert->encode)
    {
      wchar_t buf[size];
      size_t dtlen = (AVT_LINELENGTH + 1) * 4 / sizeof (wchar_t);
      wchar_t wcs_default_text[dtlen];

      memset (s, '\0', size);
      wcs_default_text[0] = L'\0';

      if (default_text and * default_text)
	{
	  size_t chars = char_to_wchar (wcs_default_text, dtlen,
					default_text, strlen (default_text));
	  wcs_default_text[chars] = L'\0';
	}

      ch = avt_input (buf, sizeof (buf), wcs_default_text, position, mode);

      status = avt_get_status ();
      if (status != AVT_NORMAL)
	return AVT_KEY_NONE;

      // size - 1 to keep the terminator
      wchar_to_char (s, size, buf, size - 1);
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


extern size_t
avt_recode_char (const struct avt_charenc *tocode,
		 char *dest, size_t dest_size,
		 const struct avt_charenc *fromcode,
		 const char *src, size_t src_size)
{
  size_t result_size = 0;

  // reserve 1 byte for terminator
  --dest_size;

  while (src_size and dest_size)
    {
      avt_char ch;

      size_t nsrc = fromcode->decode (fromcode, &ch, src);
      size_t ndest = tocode->encode (tocode, dest, dest_size, ch);

      if (not ndest)
	break;

      dest_size -= ndest;
      dest += ndest;
      result_size += ndest;

      if (not nsrc or nsrc > src_size)
	break;

      src_size -= nsrc;
      src += nsrc;
    }

  dest[result_size] = '\0';

  return result_size;
}
