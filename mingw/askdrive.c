/* 
 * askdrive - windows specific function:
 * ask for drive letter for avtfilechooser
 * Copyright (c) 2007,2008,2009,2010,2012 Andreas K. Foerster <info@akfoerster.de>
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
avta_ask_drive (int max_idx)
{
  char drive[4] = "X:";
  int drives[26];
  int choice;
  int i, number;
  int status;

  status = AVT_NORMAL;

  /* what drives are accessible? */
  number = 0;
  for (i = 1; i <= 26; i++)
    {
      if (!_chdrive (i))
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
  avt_say (L"\x21D1");

  /* show drives */
  for (i = 0; i < number; i++)
    {
      drive[0] = drives[i] + 'A' - 1;
      avt_new_line ();
      avt_next_tab ();
      avt_say_mb (drive);
    }

  avt_lock_updates (false);
  status = avt_choice (&choice, 1, number + 1, 0, false, false);

  if (choice == 1)		/* home selected */
    status = AVT_QUIT;

  if (status == AVT_NORMAL)
    {
      if (_chdrive (drives[choice - 1 - 1]) < 0)
        goto ask;
    }

  return status;
}
