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

#define SCROLL_DELAY 10

// three arrows up
#define BACK L"\u2191    \u2191    \u2191"

// three arrows down
#define CONTINUE L"\u2193    \u2193    \u2193"

#define MARK(S) \
         do { \
           avt_set_text_background_color (markcolor); \
           avt_clear_line (); \
           avt_move_x (mid_x-(sizeof(S)/sizeof(wchar_t)-1)/2); \
           avt_say(S); \
           avt_normal_text(); \
         } while(0)

extern int
avt_menu (int *choice, int items,
	  void (*show) (int nr, void *data), void *data)
{
  // check required parameters
  if (items <= 0 or not show)
    return AVT_FAILURE;

  avt_set_text_delay (0);
  avt_normal_text ();

  avt_color markcolor = avt_darker (avt_get_balloon_color (), 0x22);

  int start_line = avt_where_y ();
  if (start_line < 1)		// no balloon yet?
    start_line = 1;

  int mid_x = avt_get_max_x () / 2;	// used by MARK()
  int max_idx = avt_get_max_y () - start_line + 1;

  // check if it's a short menu
  bool small = (items <= max_idx);

  if (choice)
    *choice = 0;

  int result = 0;
  int page_nr = 0;
  int items_per_page = small ? max_idx : max_idx - 2;

  // move the screen?
  enum
  { MOVE_NONE, MOVE_UP, MOVE_DOWN } move = MOVE_NONE;

  bool old_auto_margin = avt_get_auto_margin ();
  avt_set_auto_margin (false);

  avt_move_xy (1, start_line);

  // display speed test
  avt_lock_updates (false);
  size_t delay = avt_ticks ();
  avt_clear_down ();
  delay = avt_elapsed (delay);

  while (not result)
    {
      int page_items = 0;

      // don't animate if display speed is slow
      if (delay >= 200)
	move = MOVE_NONE;

      switch (move)
	{
	case MOVE_NONE:
	  page_items = 1;
	  avt_lock_updates (true);
	  avt_move_xy (1, start_line);
	  avt_clear_down ();

	  if (page_nr > 0)
	    MARK (BACK);
	  else
	    {
	      avt_clear_line ();
	      show (page_items, data);
	    }

	  for (int i = items_per_page; i > 0; --i)
	    {
	      if (page_items + (page_nr * items_per_page) != items)
		{
		  avt_move_xy (1, start_line + page_items);
		  page_items++;
		  show (page_items + (page_nr * items_per_page), data);
		}
	    }

	  // are there more items?
	  if (items > page_items + (page_nr * items_per_page))
	    {
	      avt_move_xy (1, start_line + max_idx);
	      MARK (CONTINUE);
	      page_items = max_idx;
	    }

	  avt_lock_updates (false);
	  break;

	case MOVE_UP:
	  page_items = 1;
	  avt_move_xy (1, start_line + max_idx);

	  if (page_nr > 0)
	    MARK (BACK);
	  else
	    {
	      avt_clear_line ();
	      show (page_items, data);
	    }

	  for (int i = items_per_page; i > 0; --i)
	    {
	      avt_move_xy (1, start_line + max_idx);
	      avt_delete_lines (start_line, 1);

	      if (page_items + (page_nr * items_per_page) != items)
		{
		  page_items++;
		  show (page_items + (page_nr * items_per_page), data);
		}

	      avt_delay (SCROLL_DELAY);
	    }

	  avt_delete_lines (start_line, 1);

	  // are there more items?
	  if (items > page_items + (page_nr * items_per_page))
	    {
	      avt_move_xy (1, start_line + max_idx);
	      MARK (CONTINUE);
	      page_items = max_idx;
	    }
	  break;

	case MOVE_DOWN:
	  page_items = max_idx;
	  avt_move_xy (1, start_line);
	  MARK (CONTINUE);

	  for (int i = items_per_page; i > 0; --i)
	    {
	      avt_move_xy (1, start_line);
	      avt_insert_lines (start_line, 1);

	      page_items--;
	      show (page_items + (page_nr * items_per_page), data);

	      avt_delay (SCROLL_DELAY);
	    }

	  avt_insert_lines (start_line, 1);
	  avt_move_xy (1, start_line);

	  if (page_nr > 0)
	    MARK (BACK);
	  else
	    show (1, data);

	  page_items = max_idx;
	  break;
	}

      int page_choice;		// choice for this page
      if (avt_choice (&page_choice, start_line, page_items, AVT_KEY_NONE,
		      page_nr > 0,
		      not small and (page_items == max_idx or page_nr == 0)))
	break;

      if (page_nr > 0 and page_choice == 1)
	{
	  page_nr--;		// page back
	  move = MOVE_DOWN;
	}
      else if (not small and page_choice == max_idx)
	{
	  page_nr++;		// page forward
	  move = MOVE_UP;
	}
      else if (page_nr == 0)
	result = page_choice;
      else
	result = page_choice + (page_nr * items_per_page);
    }				// while

  avt_set_auto_margin (old_auto_margin);

  if (choice)
    *choice = result;

  return _avt_STATUS;
}
