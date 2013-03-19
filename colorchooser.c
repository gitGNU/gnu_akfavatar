/*
 * colorchooser - colorchooser dialog for AKFAvatar
 * Copyright (c) 2009,2010,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
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
#include <stdlib.h>
#include <wchar.h>
#include <iso646.h>

#define WIDTH 35

// Manual input
#define MANUAL L" > "

static char *
manual_entry (void)
{
  static char manual_color[WIDTH + 1 - 2];	// must be static!
  wchar_t name[WIDTH + 1 - 2];

  avt_set_balloon_height (1);
  avt_say (L"> ");

  if (avt_ask (name, sizeof (name)) != AVT_NORMAL)
    return NULL;

  wcstombs (manual_color, name, sizeof (manual_color));

  // check, if it's a valid color name
  if (avt_colorname (manual_color) > -1)
    return manual_color;
  else
    return NULL;
}

static void
show_color (int nr, void *data)
{
  avt_color *darker = data;

  if (nr == 1)
    {
      avt_set_text_background_color (*darker);
      avt_say (MANUAL);
      avt_normal_text ();
    }
  else
    {
      const char *color_name;
      int colornr;

      // nr - 2, because manual entry and 0-based
      color_name = avt_palette (nr - 2, &colornr);

      if (color_name)
	{
	  // show colored spaces
	  avt_set_text_background_color (colornr);
	  avt_say (L"  ");
	  avt_set_text_background_ballooncolor ();
	  avt_forward ();

	  wchar_t desc[AVT_LINELENGTH + 1];
	  swprintf (desc, sizeof (desc) / sizeof (desc[0]),
		    L"#%06X: %s\n", colornr, color_name);

	  avt_say (desc);
	}
    }
}

extern const char *
avta_color_selection (void)
{
  const char *result;
  int choice;
  avt_color darker;

  darker = avt_darker (avt_get_balloon_color (), 0x22);

  // set maximum size
  avt_set_balloon_size (0, WIDTH);

  if (avt_menu (&choice, avt_palette_size () + 1, show_color, &darker))
    return NULL;

  if (choice == 1)
    result = (const char *) manual_entry ();
  else
    result = avt_palette (choice - 2, NULL);

  return result;
}
