/*
 * avtfilechooser - filechooser dialog for AKFAvatar
 * Copyright (c) 2008 Andreas K. Foerster <info@akfoerster.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "akfavatar.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* entries or marks that are not files */
#define MARK(S) \
         avt_set_text_background_color (0xdd, 0xdd, 0xdd); \
         avt_say (S); avt_normal_text ()

/* three arrows left */
#define PARENT_DIRECTORY L" \x2190 \x2190 \x2190 "

/* three arrows up */
#define RESTART L" \x2191 \x2191 \x2191 "

/* three arrows down */
#define CONTINUE L" \x2193 \x2193 \x2193 "

/* 3 dots */
#define LONGER L"\x2026"

/* slash */
#define DIRECTORY L"/"

static avt_bool_t
is_directory (const char *name)
{
  struct stat buf;

  if (stat (name, &buf) > -1)
    return (avt_bool_t) S_ISDIR (buf.st_mode);
  else
    return AVT_FALSE;
}

static void
show_idx (int idx)
{
  char str[5];

  snprintf (str, sizeof (str), "%c) ", idx + (int) 'a');
  avt_say_mb (str);
}

/* 
 * filechooser
 * lists files in working directory
 * return -1 on error or 0 on success
 */
int
get_file (char *filename)
{
  int rcode;
  avt_bool_t single_page;
  DIR *dir;
  struct dirent *d;
  int max_x, max_y;
  int idx;
  int filenr;
  wchar_t ch;
  char entry[100][256];

  avt_set_text_delay (0);
  avt_normal_text ();
  max_x = avt_get_max_x ();
  max_y = avt_get_max_y ();

start:
  /* returncode: assume failure as default */
  rcode = -1;
  filenr = -1;
  filename[0] = '\0';
  idx = 0;
  single_page = AVT_TRUE;
  avt_clear ();

  dir = opendir (".");
  if (dir == NULL)
    return rcode;

  avt_auto_margin (AVT_FALSE);

  /* entry for parent directory */
  strcpy (entry[idx], "..");
  show_idx (idx);
  MARK (PARENT_DIRECTORY);
  idx++;
  avt_new_line ();

  while (filename[0] == '\0')
    {
      d = readdir (dir);

      if (d == NULL)		/* end reached */
	{
	  /* no entries? */
	  if (idx == 0)
	    break;

	  if (single_page)
	    {
	      if (avt_get_menu (&ch, 1, idx, L'a'))
		break;
	      filenr = (int) (ch - L'a');
	    }
	  else			/* not single_page */
	    {
	      /* restart entry */
	      show_idx (idx);
	      MARK (RESTART);

	      if (avt_get_menu (&ch, 1, idx + 1, L'a'))
		break;
	      filenr = (int) (ch - L'a');

	      /* restart */
	      if (filenr == idx)
		{
		  closedir (dir);
		  goto start;
		}
	    }

	  if (filenr >= 0)
	    {
	      strcpy (filename, entry[filenr]);
	      rcode = 0;
	    }
	}
      else if (d->d_name[0] != '.')
	{
	  /* end of textfield reached? */
	  if (idx == max_y - 1)
	    {
	      /* continue entry */
	      show_idx (idx);
	      MARK (CONTINUE);

	      if (avt_get_menu (&ch, 1, max_y, L'a'))
		break;

	      avt_clear ();
	      single_page = AVT_FALSE;
	      filenr = (int) (ch - L'a');

	      /* continue? */
	      if (filenr == idx)
		idx = 0;
	      else
		{
		  strcpy (filename, entry[filenr]);
		  rcode = 0;
		  break;
		}
	    }

	  /* copy name into entry */
	  strncpy (entry[idx], d->d_name, sizeof (entry[idx]));
	  show_idx (idx);
	  avt_say_mb (entry[idx]);

	  /* is it a directory? */
#ifdef _DIRENT_HAVE_D_TYPE
	  if (d->d_type == DT_DIR)	/* faster */
#else
	  if (is_directory (d->d_name))
#endif /* _DIRENT_HAVE_D_TYPE */
	    {
	      /* mark as directory */
	      if (avt_where_x () > max_x)
		{
		  avt_move_x (max_x - 1);
		  MARK (LONGER);
		}
	      MARK (DIRECTORY);
	    }
	  else			/* not directory */
	    {
	      if (avt_where_x () > max_x)
		{
		  avt_move_x (max_x);
		  MARK (LONGER);
		}
	    }

	  idx++;
	  avt_new_line ();
	}
    }

  if (closedir (dir) == -1)
    rcode = -1;

  if (is_directory (filename))
    {
      chdir (filename);
      goto start;
    }

  avt_auto_margin (AVT_TRUE);
  avt_clear ();

  return rcode;
}
