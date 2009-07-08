/* 
 * avtwindows - windows specific functions for avatarsay
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
 *
 * There is no additional functionality!
 * these functions are almost only replacements for missing POSIX 
 * compatibility
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
#include "avtmsg.h"
#include <windows.h>
#include <direct.h>		/* for _chdrive */
#include <stdlib.h>
#include <string.h>

#define START_CODE 0xE000

void
edit_file (const char *name)
{
  /* enforce window mode for the other window to be shown */
  avt_switch_mode (AVT_WINDOW);

  ShellExecute (GetActiveWindow (), "open", "notepad.exe", name,
		NULL /* dir */ , SW_SHOWNORMAL);

  /*
   * program returns immediately,
   * so leave some time to see the message
   */
  avt_wait (AVT_SECONDS (5));
}

void
open_document (const char *start_dir, const char *name)
{
  /* enforce window mode for the other window to be shown */
  avt_switch_mode (AVT_WINDOW);

  ShellExecute (GetActiveWindow (), "open", name,
		NULL, start_dir, SW_SHOWNORMAL);
}

/* get user's home direcory */
void
get_user_home (char *home_dir, size_t size)
{
  char *homepath, *homedrive;

  homepath = getenv ("HOMEPATH");
  homedrive = getenv ("HOMEDRIVE");

  /* do we need to add the drive-letter? */
  if (homedrive && homepath && *(homepath + 1) != ':'
      && *(homepath + 1) != '\\')
    {
      strncpy (home_dir, homedrive, size);
      strncat (home_dir, homepath, size - strlen (home_dir) - 1);
    }
  else if (homepath)
    {
      strncpy (home_dir, homepath, size);
      if (size > 0)
	home_dir[size - 1] = '\0';
    }
  else if (size >= 4)
    strcpy (home_dir, "C:\\");
  else				/* worst case */
    home_dir[0] = '\0';
}

/* ask for drive letter */
int
ask_drive (int max_idx)
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

  status = avt_choice (&choice, 1, number + 1, 0, AVT_FALSE, AVT_FALSE);

  if (choice == 1) /* home selected */
    status = AVT_QUIT;

  if (status == AVT_NORMAL)
    {
      if (_chdrive (drives[choice - 1 - 1]) < 0)
	{
	  warning_msg (strerror (errno), NULL);
	  goto ask;
	}
    }

  return status;
}
