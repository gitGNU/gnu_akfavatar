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
#include <stdio.h>		// for snprintf
#include <wchar.h>
#include <iso646.h>

#define WIDTH 35

// Manual input
#define MANUAL L" > "

// entries or marks that are not colors
static inline void
marked_text (const wchar_t * s)
{
  avt_set_text_background_color (0xDDDDDD);
  avt_say (s);
  avt_normal_text ();
}

static char *
manual_entry (void)
{
  static char manual_color[WIDTH + 1 - 2];	// must be static!

  avt_set_balloon_height (1);
  avt_say (L"> ");

  if (avt_ask_mb (manual_color, sizeof (manual_color)) != AVT_NORMAL)
    return NULL;

  // check, if it's a valid color name
  if (avt_colorname (manual_color) > -1)
    return manual_color;
  else
    return NULL;
}

static void
show_color (int nr, void *data)
{
  (void) data;

  if (nr == 1)
    marked_text (MANUAL);
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

	  char desc[AVT_LINELENGTH + 1];
	  snprintf (desc, sizeof (desc), "#%06X: %s\n", colornr, color_name);

	  avt_say_mb (desc);
	}
    }
}

extern const char *
avta_color_selection (void)
{
  const char *result;
  int choice;

  // set maximum size
  avt_set_balloon_size (0, WIDTH);

  if (avt_menu (&choice, avt_palette_size () + 1, show_color, NULL))
    return NULL;

  if (choice == 1)
    result = (const char *) manual_entry ();
  else
    result = avt_palette (choice - 2, NULL);

  return result;
}
