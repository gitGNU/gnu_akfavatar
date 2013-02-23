/*
 * menu for AKFAvatar
 * Copyright (c) 2013 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99
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
#include <wchar.h>
#include <stdbool.h>
#include <iso646.h>

// three arrows up
#define BACK L"\x2191    \x2191    \x2191"

// three arrows down
#define CONTINUE L"\x2193    \x2193    \x2193"

#define MARK(S) \
         do { \
           avt_set_text_background_color (markcolor); \
           avt_clear_line (); \
           avt_move_x (mid_x-(sizeof(S)/sizeof(wchar_t)-1)/2); \
           avt_say(S); \
           avt_normal_text(); \
         } while(0)

static inline avt_color
avt_darker (avt_color color, int amount)
{
  int r, g, b;

  r = avt_red (color);
  g = avt_green (color);
  b = avt_blue (color);

  r = r > amount ? r - amount : 0;
  g = g > amount ? g - amount : 0;
  b = b > amount ? b - amount : 0;

  return avt_rgb (r, g, b);
}

extern int
avt_menu (int *choice, int number,
	  void (*show) (int nr, void *data), void *data)
{
  // check required parameters
  if (number <= 0 or not show)
    return AVT_FAILURE;

  avt_set_text_delay (0);
  avt_normal_text ();
  avt_lock_updates (true);

  avt_color markcolor = avt_darker (avt_get_balloon_color (), 0x22);

  int start_line = avt_where_y ();
  if (start_line < 1)		// no balloon yet?
    start_line = 1;

  int mid_x = avt_get_max_x () / 2;	// used by MARK()
  int max_idx = avt_get_max_y () - start_line + 1;

  // check if it's a short menu
  bool small = (number <= max_idx);

  if (choice)
    *choice = 0;

  int result = 0;
  int page_nr = 0;
  int items_per_page = small ? max_idx : max_idx - 2;

  bool old_auto_margin = avt_get_auto_margin ();
  bool old_newline_mode = avt_get_newline_mode ();
  avt_set_auto_margin (false);
  avt_newline_mode (true);

  while (not result)
    {
      int items;

      avt_move_xy (1, start_line);
      avt_clear_down ();

      items = 0;

      if (page_nr > 0)
	{
	  MARK (BACK);
	  items = 1;
	}

      do
	{
	  items++;
	  if (items > 1)
	    avt_new_line ();
	  show (items + (page_nr * items_per_page), data);
	}
      while (items <= items_per_page
             and items + (page_nr * items_per_page) != number);

      // are there more items?
      if (number > items + (page_nr * items_per_page))
	{
	  avt_new_line ();
	  MARK (CONTINUE);
	  items = max_idx;
	}

      avt_lock_updates (false);
      int page_choice;		// choice for this page
      if (avt_choice (&page_choice, start_line, items, AVT_KEY_NONE,
		      page_nr > 0,
		      not small and (items == max_idx or page_nr == 0)))
	return AVT_FAILURE;

      avt_lock_updates (true);

      if (page_nr > 0 and page_choice == 1)
	page_nr--;		// page back
      else if (not small and page_choice == max_idx)
	page_nr++;		// page forward
      else if (page_nr == 0)
	result = page_choice;
      else
	result = page_choice + (page_nr * items_per_page);
    }

  avt_set_auto_margin (old_auto_margin);
  avt_newline_mode (old_newline_mode);
  avt_clear ();
  avt_lock_updates (false);

  if (choice)
    *choice = result;

  return _avt_STATUS;
}
