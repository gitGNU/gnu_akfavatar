/* 
 * avtwindows - windows specific functions for avatarsay
 * Copyright (c) 2007, 2008, 2009, 2010 Andreas K. Foerster <info@akfoerster.de>
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
#include "avtaddons.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: write integrated editor! */

void
edit_file (const char *name, const char *encoding)
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

/* Warning: this is basically untested! */
FILE *
open_config_file (const char *name, bool writing)
{
  FILE *f;
  char *appdata;
  char path[1024];

  f = NULL;

  appdata = getenv ("APPDATA");

  if (appdata)
    snprintf (path, sizeof (path), "%s\\akfavatar\\%s.cnf", appdata, name);
  else				/* use current directory as fallback */
    snprintf (path, sizeof (path), "%s.cnf", name);

  if (!writing)
    {
      f = fopen (path, "r");
    }
  else				/* writing */
    {
      f = fopen (path, "w");

      /* if that fails, try to create the directory */
      if (!f && appdata)
	{
	  snprintf (path, sizeof (path), "%s\\akfavatar", appdata);
	  if (mkdir (path) > -1)
	    {
	      snprintf (path, sizeof (path), "%s\\akfavatar\\%s.cnf",
			appdata, name);
	      f = fopen (path, "w");
	    }
	}
    }

  return f;
}
