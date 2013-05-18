/*
 * character maps support for AKFAvatar
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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <iso646.h>

static const struct avt_char_map *map;

extern void
avt_set_char_map (const struct avt_char_map *m)
{
  map = m;
}

static inline avt_char
map_to_unicode (const unsigned char ch)
{
  if (map and ch >= map->start and ch <= map->end)
    return (avt_char) map->table[ch - map->start];
  else
    return (avt_char) ch;
}

// result is not terminated unless len includes the terminator
static void
map_to_wide (wchar_t * dest, const char *src, size_t len)
{
  while (len--)
    *dest++ = (wchar_t) map_to_unicode (*src++);
}

static void
wide_to_map (char *dest, const wchar_t * src, size_t len)
{
  for (size_t i = 0; i < len; ++i)
    {
      register wchar_t ch = src[i];
      if (not map or ch < map->start or (ch <= 0xFF and ch > map->end))
	dest[i] = (char) ch;
      else			// search table
	{
	  char dch = INVALID_CHAR;

	  for (int j = map->end - map->start; j >= 0; --j)
	    {
	      if (ch == map->table[j])
		{
		  dch = map->start + j;
		  break;
		}
	    }

	  dest[i] = dch;
	}

      if (not ch)
	break;
    }

  dest[len - 1] = '\0';
}


extern int
avt_say_map_len (const char *txt, size_t len)
{
  int status = avt_get_status ();

  // nothing to do, when txt == NULL
  // but do allow a text to start with zeros here
  if (not txt or status != AVT_NORMAL or not avt_initialized ())
    return avt_update ();

  while (len--)
    {
      status = avt_put_char (map_to_unicode (*txt++));
      if (status != AVT_NORMAL)
	break;
    }

  return status;
}


extern int
avt_say_map (const char *txt)
{
  int status = avt_get_status ();

  if (not avt_initialized ())
    return status;

  if (not txt or not * txt)
    return avt_update ();

  while (*txt)
    {
      status = avt_put_char (map_to_unicode (*txt++));
      if (status != AVT_NORMAL)
	break;
    }

  return status;
}


extern int
avt_tell_map_len (const char *txt, size_t len)
{
  int status = avt_get_status ();

  if (txt and status == AVT_NORMAL)
    {
      if (not len or len > 0x80000000)
	len = strlen (txt);
      wchar_t wide[len];
      map_to_wide (wide, txt, len);
      status = avt_tell_len (wide, len);
    }

  return status;
}


extern int
avt_tell_map (const char *txt)
{
  int status = avt_get_status ();

  if (txt and * txt)
    status = avt_tell_map_len (txt, strlen (txt));

  return status;
}


extern int
avt_set_avatar_name_map (const char *name)
{
  int status;

  if (not name or not * name)
    status = avt_set_avatar_name (NULL);
  else
    {
      size_t len = strlen (name);

      wchar_t wide[len + 1];

      map_to_wide (wide, name, len + 1);
      status = avt_set_avatar_name (wide);
    }

  return status;
}


extern int
avt_pager_map (const char *txt, size_t len, int startline)
{
  int status = avt_get_status ();

  if (txt and status == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      wchar_t *wctext = malloc (len * sizeof (wchar_t));

      if (wctext)
	{
	  map_to_wide (wctext, txt, len);
	  status = avt_pager (wctext, len, startline);
	  free (wctext);
	}
    }

  return status;
}


extern int
avt_credits_map (const char *txt, bool centered)
{
  int status = avt_get_status ();

  if (status == AVT_NORMAL and txt and * txt and avt_initialized ())
    {
      size_t len = strlen (txt);

      wchar_t *wctext = malloc ((len + 1) * sizeof (wchar_t));

      if (wctext)
	{
	  map_to_wide (wctext, txt, len + 1);
	  status = avt_credits (wctext, centered);
	  free (wctext);
	}
    }

  return status;
}


extern avt_char
avt_input_map (char *s, size_t size, const char *default_text,
		 int position, int mode)
{
  int status = avt_get_status ();
  avt_char ch = AVT_KEY_NONE;

  if (s and size)
    {
      wchar_t buf[size], wcs_default_text[AVT_LINELENGTH + 1];
      memset (s, '\0', size);
      wcs_default_text[0] = L'\0';

      if (default_text and * default_text)
	{
	  size_t len = strlen (default_text);
	  if (len > AVT_LINELENGTH)
	    len = AVT_LINELENGTH;
	  map_to_wide (wcs_default_text, default_text, len + 1);
	  wcs_default_text[AVT_LINELENGTH] = L'\0';
	}

      ch = avt_input (buf, sizeof (buf), wcs_default_text, position, mode);

      status = avt_get_status ();
      if (status != AVT_NORMAL)
	return AVT_KEY_NONE;
      wide_to_map (s, buf, size);
    }

  return ch;
}


extern int
avt_ask_map (char *s, size_t size)
{
  if (s and size)
    avt_input_map (s, size, NULL, -1, 0);

  return avt_get_status ();
}
