/*
 * filechooser - filechooser dialog for AKFAvatar
 * Copyright (c) 2008,2009,2010,2011,2012,2013,2014
 * Andreas K. Foerster <info@akfoerster.de>
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

#define _BSD_SOURCE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "akfavatar.h"
#include "avtaddons.h"
#include "avtinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iso646.h>

// black left-pointing pointer
#define PARENT_DIRECTORY L" \u25C4 "

// black right-pointing pointer
#define DIRECTORY L"\u25BA"

// 3 dots
#define LONGER L"\u2026"

#ifdef _WIN32
#  define HAS_DRIVE_LETTERS true
#else
#  define HAS_DRIVE_LETTERS false
#  define is_root_dir(void) false	// dummy
#  define avt_ask_drive(max_idx) 0	// dummy
#endif

struct avt_fc_data
{
  avt_color markcolor;
  struct dirent **namelist;
};


// entries or marks that are not files
static inline void
marked_text (wchar_t * s, avt_color markcolor)
{
  avt_set_text_background_color (markcolor);
  avt_say (s);
  avt_set_text_background_ballooncolor ();
}

static bool
is_directory (const char *name)
{
  struct stat buf;

  if (stat (name, &buf) > -1)
    return (bool) S_ISDIR (buf.st_mode);
  else
    return false;
}

#ifdef _DIRENT_HAVE_D_TYPE
#  define is_dirent_directory(d) \
	    (d->d_type == DT_DIR \
	      or ((d->d_type == DT_UNKNOWN or d->d_type == DT_LNK) \
	             and is_directory (d->d_name)))
#else
#  define is_dirent_directory(d) (is_directory (d->d_name))
#endif // _DIRENT_HAVE_D_TYPE

static void
show_directory (avt_color markcolor)
{
  char dirname[4096];

  if (getcwd (dirname, sizeof (dirname)))
    {
      avt_set_text_background_color (markcolor);
      avt_clear_line ();	// mark the line
      avt_say_char (dirname);
      avt_set_text_background_ballooncolor ();
    }
}

#ifndef _WIN32

#ifdef __USE_GNU
#  define FILTER_DIRENT_T  const struct dirent
#else
#  define FILTER_DIRENT_T  struct dirent
#endif

static int
filter_dirent (FILTER_DIRENT_T * d, avt_filter filter, void *filter_data)
{
  // allow nothing that starts with a dot
  if (not d or d->d_name[0] == '.')
    return false;
  else if (not filter or is_dirent_directory (d))
    return true;
  else
    return filter (d->d_name, filter_data);
}

#else // _WIN32

static int
filter_dirent (const struct dirent *d, avt_filter filter, void *filter_data)
{
  // don't allow "." and ".." and apply filter
  if (not d or strcmp (".", d->d_name) == 0 or strcmp ("..", d->d_name) == 0)
    return false;
  else if (not filter or is_dirent_directory (d))
    return true;
  else
    return filter (d->d_name, filter_data);
}

static inline bool
is_root_dir (void)
{
  char dirname[4096];

  return (getcwd (dirname, sizeof (dirname))
	  and dirname[1] == ':' and dirname[3] == '\0');
}

#endif // _WIN32

static void
show_info (void)
{
  avt_set_balloon_size (4, 70);
  avt_say (L"AKFAvatar ");
  avt_say (avt_wide_version ());
  avt_new_line ();
  avt_say (avt_wide_copyright ());
  avt_new_line ();
  avt_new_line ();
  avt_say (avt_wide_license ());
  avt_wait_button ();
  avt_set_balloon_size (0, 0);
}

static void
ask_directory (avt_color markcolor)
{
  char dirname[AVT_LINELENGTH * 4 + 1];

  avt_move_xy (1, 1);
  avt_set_text_background_color (markcolor);
  avt_clear_line ();
  avt_ask_char (dirname, sizeof (dirname));
  avt_set_text_background_ballooncolor ();

  if (strcmp (dirname, "about:akfavatar") == 0)
    show_info ();
  else
    {
      if (chdir (dirname) == -1)
	avt_bell ();
    }
}

static int
compare_dirent (const void *a, const void *b)
{
  return strcoll ((*(struct dirent **) a)->d_name,
		  (*(struct dirent **) b)->d_name);
}

static int
get_directory (struct dirent ***list, avt_filter filter, void *filter_data)
{
  int max_entries;
  struct dirent **mylist;
  struct dirent *d;
  int entries;
  DIR *dir;

  *list = NULL;
  mylist = NULL;
  entries = max_entries = 0;

  dir = opendir (".");
  if (not dir)
    return -1;

  while ((d = readdir (dir)))
    {
      // need more memory?
      if (entries >= max_entries)
	{
	  struct dirent **tmp;

	  if (max_entries > 0)
	    max_entries *= 2;
	  else
	    max_entries = 10;

	  tmp = (struct dirent **)
	    realloc (mylist, max_entries * sizeof (struct dirent *));

	  if (not tmp)
	    {
	      while (entries--)
		free (mylist[entries]);

	      free (mylist);
	      closedir (dir);

	      return -1;
	    }

	  mylist = tmp;
	}

      if (filter_dirent (d, filter, filter_data))
	{
	  struct dirent *n;

	  n = (struct dirent *) malloc (sizeof (struct dirent));

	  if (not n)
	    break;

	  memcpy (n, d, sizeof (struct dirent));
	  mylist[entries++] = n;
	}
    }

  // sort
  if (mylist)
    qsort (mylist, entries, sizeof (struct dirent *), compare_dirent);

  closedir (dir);

  if (not mylist)
    return -1;

  *list = mylist;
  return entries;
}

// show entry nr
static void
show (int nr, void *fc_data)
{
  struct avt_fc_data *data = fc_data;
  avt_color markcolor = data->markcolor;

  if (1 == nr)
    show_directory (markcolor);
  else if (2 == nr)
    marked_text (PARENT_DIRECTORY, markcolor);
  else
    {
      struct dirent *d = data->namelist[nr - 3];
      int max_x = avt_get_max_x ();

      avt_say_char (d->d_name);

      // is it a directory?
      if (is_dirent_directory (d))
	{
	  // mark as directory
	  if (avt_where_x () > max_x - 1)
	    {
	      avt_move_x (max_x - 1);
	      marked_text (LONGER, markcolor);
	    }
	  marked_text (DIRECTORY, markcolor);
	}
      else			// not directory
	{
	  if (avt_where_x () > max_x)
	    {
	      avt_move_x (max_x);
	      marked_text (LONGER, markcolor);
	    }
	}
    }
}

/*
 * filechooser
 * coose a file, starting in working directory
 * return -1 on error or 0 on success
 */
extern int
avt_file_selection (char *filename, int filename_size,
		    avt_filter filter, void *filter_data)
{
  int rcode;			// return code
  int choice;
  int entries;
  struct avt_fc_data data;
  const struct avt_charenc *old_encoding;

  rcode = -1;

  if (not filename or filename_size <= 0)
    return -1;

  avt_set_text_background_ballooncolor ();
  memset (filename, 0, filename_size);

  old_encoding = avt_char_encoding (avt_systemencoding ());

  data.markcolor = avt_get_balloon_color ();
  if (avt_brightness (data.markcolor) >= 0x88)
    data.markcolor = avt_darker (data.markcolor, 0x20);
  else
    data.markcolor = avt_brighter (data.markcolor, 0x88);

  data.namelist = NULL;

  // don't show the balloon
  avt_show_avatar ();

  // set maximum size
  avt_set_balloon_size (0, 0);

  while (rcode < 0)
    {
      entries = get_directory (&data.namelist, filter, filter_data);
      if (entries < 0)
	break;

      avt_move_xy (1, 1);
      // entry 1 is directory name, entry 2 is parent directory
      if (avt_menu (&choice, entries + 2, show, &data))
	break;

      if (1 == choice)		// path
	ask_directory (data.markcolor);
      else if (2 == choice)	// parent directory
	{
	  if (HAS_DRIVE_LETTERS and is_root_dir ())	// ask for drive?
	    {
	      if (avt_ask_drive (avt_get_max_y ()) != AVT_NORMAL)
		break;

	      avt_set_balloon_size (0, 0);
	    }
	  else if (chdir ("..") == -1)
	    avt_bell ();
	}
      else			// normal entry
	{
	  struct dirent *d = data.namelist[choice - 3];

	  if (is_dirent_directory (d))
	    {
	      if (chdir (d->d_name))
		avt_bell ();
	    }
	  else if (strlen (d->d_name) < (size_t) filename_size)
	    {
	      strcpy (filename, d->d_name);
	      rcode = 0;	// found
	    }
	}

      // free namelist
      while (entries--)
	free (data.namelist[entries]);
      free (data.namelist);
      data.namelist = NULL;
    }

  if (data.namelist)
    {
      while (entries--)
	free (data.namelist[entries]);
      free (data.namelist);
      data.namelist = NULL;
    }

  avt_char_encoding (old_encoding);

  return rcode;
}
