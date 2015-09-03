/* 
 * askdrive - windows specific function:
 * ask for drive letter for avtfilechooser
 * Copyright (c) 2007,2008,2009,2010,2012,2013
 * Andreas K. Foerster <akf@akfoerster.de>
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
#include <direct.h>		/* for _chdrive */
#include <string.h>
#include <errno.h>

/* ask for drive letter */
int
avt_ask_drive (int max_idx)
{
  int drives[26];
  int number;

  /* what drives are accessible? */
  number = 0;
  for (int i = 1; i <= 26; i++)
    {
      if (_chdrive (i) == 0)
	{
	  drives[number] = i;
	  number++;
	  /* maximum number of entries reached? */
	  if (number == max_idx)
	    break;
	}
    }

ask:
  avt_set_balloon_size (number + 1, 2 * 8 + 1);
  avt_lock_updates (true);
  avt_clear ();

  /* show double arrow up */
  avt_next_tab ();
  avt_put_char (L'\u21D1');

  /* show drives */
  for (int i = 0; i < number; i++)
    {
      avt_new_line ();
      avt_next_tab ();
      avt_put_char (drives[i] - 1 + 'A');
      avt_put_char (L':');
    }

  avt_lock_updates (false);

  int choice;
  int status = avt_choice (&choice, 1, number + 1, 0, false, false);

  if (choice == 1)		/* home selected */
    status = AVT_QUIT;

  if (status == AVT_NORMAL)
    {
      if (_chdrive (drives[choice - 1 - 1]) < 0)
	goto ask;
    }

  return status;
}
