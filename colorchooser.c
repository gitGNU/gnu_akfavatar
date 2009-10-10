/*
 * colorchooser - colorchooser dialog for AKFAvatar
 * Copyright (c) 2009 Andreas K. Foerster <info@akfoerster.de>
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

/* House symbol */
#define HOME L" \x2302 "

/* Keyboard symbol */
#define KEYBOARD L" \x2328 "

/* three arrows up */
#define BACK L" \x2191 \x2191 \x2191 "

/* three arrows down */
#define CONTINUE L" \x2193 \x2193 \x2193 "

/* entries or marks that are not colors */
#define MARK(S) \
         do { avt_set_text_background_color (0xdd, 0xdd, 0xdd); \
         avt_say (S); avt_normal_text (); } while(0)

static char *
manual_entry (void)
{
  static char manual_color[80];	/* must be static! */
  int red, green, blue;

  avt_set_balloon_height (1);
  avt_say (L"> ");
  avt_lock_updates (AVT_FALSE);

  if (avt_ask_mb (manual_color, sizeof (manual_color)) != AVT_NORMAL)
    return NULL;

  /* check, if it's a valid color name */
  if (avt_name_to_color (manual_color, &red, &green, &blue) > -1)
    return manual_color;
  else
    return NULL;
}

extern const char *
avta_color_selection (void)
{
  const char *color, *c;
  int i;
  int max_idx, items, offset, page_nr;
  int choice;

  avt_set_text_delay (0);
  avt_normal_text ();
  avt_lock_updates (AVT_TRUE);

  /* set maximum size */
  avt_set_balloon_size (0, 30);

  color = c = NULL;
  max_idx = avt_get_max_y ();

  page_nr = 0;

  avt_auto_margin (AVT_FALSE);

  while (!color)
    {
      avt_clear ();

      MARK ((page_nr > 0) ? BACK : HOME);
      avt_new_line ();
      items = offset = 1;

      if (page_nr == 0)
	{
	  MARK (KEYBOARD);
	  avt_new_line ();
	  items++;
	  offset++;
	}

      for (i = 0; i < max_idx - offset - 1; i++)
	{
	  c = avt_get_color_name (i + (page_nr * (max_idx - offset)));
	  if (c)
	    {
	      avt_set_text_background_color_name (c);
	      avt_say (L"  ");
	      avt_set_text_background_ballooncolor ();
	      avt_forward ();
	      avt_say_mb (c);
	      avt_new_line ();
	      items++;
	    }
	}

      if (c != NULL)
	{
	  MARK (CONTINUE);
	  items = max_idx;
	}

      avt_lock_updates (AVT_FALSE);
      if (avt_choice (&choice, 1, items, 0, (page_nr > 0), (c != NULL)))
	break;
      else
	avt_lock_updates (AVT_TRUE);

      if (choice == 1 && page_nr > 0)
	page_nr--;		/* page back */
      else if (choice == 1)
	break;			/* home */
      else if (choice == max_idx)
	page_nr += (c == NULL) ? 0 : 1;	/* page forward */
      else if (page_nr == 0 && choice == 2)
	{
	  color = (const char *) manual_entry ();
	  break;
	}
      else
	color = avt_get_color_name (choice - 1 - offset
				    + (page_nr * (max_idx - offset)));
    }

  avt_auto_margin (AVT_TRUE);
  avt_clear ();
  avt_lock_updates (AVT_FALSE);

  return color;
}
