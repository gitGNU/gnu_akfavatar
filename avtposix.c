/*
 * avtposix - system specific functions for avatarsay (posix?)
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

#include "avtaddons.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

/* TODO: write integrated editor! */

void
edit_file (const char *name, const char *encoding)
{
  char *editor;
  char *args[3];
  int fd;

  if ((editor = getenv ("VISUAL")) == NULL
      && (editor = getenv ("EDITOR")) == NULL)
    editor = "vi";

  args[0] = editor;
  args[1] = (char *) name;
  args[2] = (char *) NULL;

  fd = avta_term_start (encoding, NULL, args);
  if (fd > -1)
    avta_term_run (fd);
}

/* get user's home direcory */
void
get_user_home (char *home_dir, size_t size)
{
  char *home;

  home = getenv ("HOME");

  /* when the variable is not set, dig deeper */
  if (home == NULL || *home == '\0')
    {
      struct passwd *user_data;

      user_data = getpwuid (getuid ());
      if (user_data != NULL && user_data->pw_dir != NULL
	  && *user_data->pw_dir != '\0')
	home = user_data->pw_dir;
    }

  strncpy (home_dir, home, size);
  if (size > 0)
    home_dir[size - 1] = '\0';
}
