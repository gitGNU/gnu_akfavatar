/*
 * Latin-1 (ISO-8859-1) support for AKFAvatar
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

/*
 * Latin-1 is just a small subset of the supported characters,
 * but it's still widely used and easy to implement
 */

#include "akfavatar.h"
#include "avtinternals.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <iso646.h>

// substitution character for characters not available in Latin-1
#define SUB '\x1A'

// result is not terminated unless len includes the terminator
static void
lat1_to_wide (const char *l1, wchar_t * wide, size_t len)
{
  const unsigned char *t = (const unsigned char *) l1;

  while (len--)
    *wide++ = (wchar_t) (*t++);
}


extern int
avt_say_l1_len (const char *txt, size_t len)
{
  // nothing to do, when txt == NULL
  // but do allow a text to start with zeros here
  if (not txt or _avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return avt_update ();

  const unsigned char *p = (const unsigned char *) txt;

  while (len--)
    if (avt_put_char (*p++) != AVT_NORMAL)
      break;

  return _avt_STATUS;
}


extern int
avt_say_l1 (const char *txt)
{
  if (_avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return _avt_STATUS;

  if (not txt or not * txt)
    return avt_update ();

  const unsigned char *p = (const unsigned char *) txt;

  while (*p)
    if (avt_put_char (*p++) != AVT_NORMAL)
      break;

  return _avt_STATUS;
}


extern int
avt_tell_l1_len (const char *txt, size_t len)
{
  if (txt)
    {
      if (not len or len > 0x80000000)
	len = strlen (txt);

      wchar_t wide[len];
      lat1_to_wide (txt, wide, len);
      avt_tell_len (wide, len);
    }

  return _avt_STATUS;
}


extern int
avt_tell_l1 (const char *txt)
{
  if (txt and * txt)
    avt_tell_l1_len (txt, 0);

  return _avt_STATUS;
}


extern int
avt_set_avatar_name_l1 (const char *name)
{
  if (not name or not * name)
    avt_set_avatar_name (NULL);
  else
    {
      size_t len = strlen (name);
      wchar_t wide[len + 1];
      lat1_to_wide (name, wide, len + 1);

      avt_set_avatar_name (wide);
    }

  return _avt_STATUS;
}


extern int
avt_pager_l1 (const char *txt, size_t len, int startline)
{
  if (txt and _avt_STATUS == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      wchar_t *wctext = malloc (len * sizeof (wchar_t));

      if (wctext)
	{
	  lat1_to_wide (txt, wctext, len);
	  avt_pager (wctext, len, startline);
	  free (wctext);
	}
    }

  return _avt_STATUS;
}


extern int
avt_credits_l1 (const char *txt, bool centered)
{
  if (_avt_STATUS == AVT_NORMAL and txt and * txt and avt_initialized ())
    {
      size_t len = strlen (txt);
      wchar_t *wctext = malloc ((len + 1) * sizeof (wchar_t));

      if (wctext)
	{
	  lat1_to_wide (txt, wctext, len + 1);
	  avt_credits (wctext, centered);
	  free (wctext);
	}
    }

  return _avt_STATUS;
}


extern avt_char
avt_input_l1 (char *s, size_t size, const char *default_text,
	      int position, int mode)
{
  avt_char ch = AVT_KEY_NONE;

  if (s and size)
    {
      wchar_t buf[size], wcs_default_text[size];

      memset (s, '\0', size);
      wcs_default_text[0] = L'\0';

      if (default_text and * default_text)
	lat1_to_wide (default_text, wcs_default_text, size);

      ch = avt_input (buf, sizeof (buf), wcs_default_text, position, mode);

      if (_avt_STATUS != AVT_NORMAL)
	return AVT_KEY_NONE;

      for (size_t i = 0; i < size; ++i)
	{
	  register wchar_t ch = buf[i];
	  s[i] = (ch <= L'\xFF') ? (char) ch : SUB;

	  if (not ch)
	    break;
	}

      s[size - 1] = '\0';
    }

  return ch;
}


extern int
avt_ask_l1 (char *s, size_t size)
{
  if (s and size)
    avt_input_l1 (s, size, NULL, -1, 0);

  return _avt_STATUS;
}
