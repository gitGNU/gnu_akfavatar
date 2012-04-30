/*
 * colorchooser - colorchooser dialog for AKFAvatar
 * Copyright (c) 2009,2010,2012 Andreas K. Foerster <info@akfoerster.de>
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
#include <stdio.h>		/* for sprintf */

/* House symbol */
#define HOME L"\x2302"

/* Keyboard symbol (not int the 7x14 font) */
/* #define KEYBOARD L" \x2328 " */

/* Manual input */
#define MANUAL L" > "

/* three arrows up */
#define BACK L"\x2191 \x2191 \x2191"

/* three arrows down */
#define CONTINUE L"\x2193 \x2193 \x2193"

/* entries or marks that are not colors */
#define marked_text(S) \
         do { avt_set_text_background_colornr (avt_rgb (0xdd, 0xdd, 0xdd)); \
         avt_say (S); avt_normal_text (); } while(0)

#define marked_line(S) \
         do { \
           avt_set_text_background_colornr (avt_rgb (0xdd, 0xdd, 0xdd)); \
           avt_clear_line (); \
           avt_move_x (mid_x-(sizeof(S)/sizeof(wchar_t)-1)/2); \
           avt_say(S); \
           avt_normal_text(); \
         } while(0)


static char *
manual_entry (void)
{
  static char manual_color[80];	/* must be static! */
  int red, green, blue;

  avt_set_balloon_height (1);
  avt_say (L"> ");
  avt_lock_updates (false);

  if (avt_ask_mb (manual_color, sizeof (manual_color)) != AVT_NORMAL)
    return NULL;

  /* check, if it's a valid color name */
  if (avt_colorname (manual_color) > -1)
    return manual_color;
  else
    return NULL;
}

extern const char *
avta_color_selection (void)
{
  const char *color, *color_name;
  char desc[AVT_LINELENGTH];
  int red, green, blue;
  int i;
  int mid_x;
  int max_idx, items, offset, page_nr;
  int choice;
  bool old_auto_margin;

  avt_set_text_delay (0);
  avt_normal_text ();
  avt_lock_updates (true);

  /* set maximum size */
  avt_set_balloon_size (0, 35);

  color = color_name = NULL;
  max_idx = avt_get_max_y ();
  mid_x = avt_get_max_x () / 2;	/* for marked_line() */

  page_nr = 0;

  old_auto_margin = avt_get_auto_margin ();
  avt_set_auto_margin (false);

  while (!color)
    {
      avt_clear ();

      if (page_nr > 0)
	marked_line (BACK);
      else
	marked_line (HOME);

      avt_new_line ();
      items = offset = 1;

      if (page_nr == 0)
	{
	  marked_text (MANUAL);
	  avt_new_line ();
	  items++;
	  offset++;
	}

      for (i = 0; i < max_idx - offset - 1; i++)
	{
	  color_name = avt_get_color (i + (page_nr * (max_idx - offset)),
				      &red, &green, &blue);

	  if (color_name)
	    {
	      /* show colored spaces */
	      avt_set_text_background_colornr (avt_rgb (red, green, blue));
	      avt_say (L"  ");
	      avt_set_text_background_ballooncolor ();
	      avt_forward ();

	      snprintf (desc, sizeof (desc), "#%02X%02X%02X: %s\n",
			red, green, blue, color_name);

	      /* show description */
	      avt_say_mb (desc);
	      items++;
	    }
	}

      if (color_name != NULL)
	{
	  marked_line (CONTINUE);
	  items = max_idx;
	}

      avt_lock_updates (false);
      if (avt_choice (&choice, 1, items, 0,
		      (page_nr > 0), (color_name != NULL)))
	break;
      else
	avt_lock_updates (true);

      if (choice == 1 && page_nr > 0)
	page_nr--;		/* page back */
      else if (choice == 1)
	break;			/* home */
      else if (choice == max_idx)
	page_nr += (color_name == NULL) ? 0 : 1;	/* page forward */
      else if (page_nr == 0 && choice == 2)
	{
	  color = (const char *) manual_entry ();
	  break;
	}
      else
	color = avt_get_color_name (choice - 1 - offset
				    + (page_nr * (max_idx - offset)));
    }

  avt_set_auto_margin (old_auto_margin);
  avt_clear ();
  avt_lock_updates (false);

  return color;
}
