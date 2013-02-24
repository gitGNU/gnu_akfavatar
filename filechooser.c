/*
 * filechooser - filechooser dialog for AKFAvatar
 * Copyright (c) 2008,2009,2010,2011,2012,2013
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
#define PARENT_DIRECTORY L" \x25C4 "

// black right-pointing pointer
#define DIRECTORY L"\x25BA"

// 3 dots
#define LONGER L"\x2026"

#ifdef _WIN32
#  define HAS_DRIVE_LETTERS true
#else
#  define HAS_DRIVE_LETTERS false
#  define is_root_dir(void) false	// dummy
#  define avta_ask_drive(max_idx) 0	// dummy
#endif

// variable for custom filter
static avta_filter custom_filter = NULL;
static avt_color markcolor;

static inline void
marked (void)
{
  avt_set_text_background_color (markcolor);
}

// entries or marks that are not files
static inline void
marked_text (wchar_t * s)
{
  marked ();
  avt_say (s);
  avt_normal_text ();
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
show_directory (void)
{
  char dirname[4096];

  if (getcwd (dirname, sizeof (dirname)))
    {
      marked ();
      avt_clear_line ();
      avt_say_mb (dirname);
      avt_normal_text ();
    }
}

#ifndef _WIN32

#ifdef __USE_GNU
#  define FILTER_DIRENT_T  const struct dirent
#else
#  define FILTER_DIRENT_T  struct dirent
#endif

static int
filter_dirent (FILTER_DIRENT_T * d)
{
  // allow nothing that starts with a dot
  if (d == NULL or d->d_name[0] == '.')
    return false;
  else if (is_dirent_directory (d))
    return true;
  else
    return (custom_filter == NULL or (*custom_filter) (d->d_name));
}

#else // _WIN32

static int
filter_dirent (const struct dirent *d)
{
  // don't allow "." and ".." and apply custom_filter
  if (d == NULL)
    return false;
  else if (strcmp (".", d->d_name) == 0 or strcmp ("..", d->d_name) == 0)
    return false;
  else if (is_dirent_directory (d))
    return true;
  else
    return (custom_filter == NULL or (*custom_filter) (d->d_name));
}

static inline bool
is_root_dir (void)
{
  char dirname[4096];

  return (getcwd (dirname, sizeof (dirname))
	  and dirname[1] == ':' and dirname[3] == '\0');
}

#endif // _WIN32

static int
compare_dirent (const void *a, const void *b)
{
  return strcoll ((*(struct dirent **) a)->d_name,
		  (*(struct dirent **) b)->d_name);
}

static int
get_directory (struct dirent ***list)
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
  if (dir == NULL)
    return -1;

  while ((d = readdir (dir)))
    {
      // need mor memory?
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

      if (filter_dirent (d))
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

// return a darker color
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

// show entry nr
static void
show (int nr, void *data)
{
  if (1 == nr)
    show_directory ();
  else if (2 == nr)
    marked_text (PARENT_DIRECTORY);
  else
    {
      struct dirent *d;
      int max_x;

      max_x = avt_get_max_x ();
      d = ((struct dirent **) data)[nr - 3];
      avt_say_mb (d->d_name);

      // is it a directory?
      if (is_dirent_directory (d))
	{
	  // mark as directory
	  if (avt_where_x () > max_x - 1)
	    {
	      avt_move_x (max_x - 1);
	      marked_text (LONGER);
	    }
	  marked_text (DIRECTORY);
	}
      else			// not directory
	{
	  if (avt_where_x () > max_x)
	    {
	      avt_move_x (max_x);
	      marked_text (LONGER);
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
avta_file_selection (char *filename, int filename_size, avta_filter filter)
{
  int rcode;			// return code
  int choice;
  int entries;
  char old_encoding[100];
  struct dirent **namelist;

  rcode = -1;

  if (filename == NULL or filename_size <= 0)
    return -1;

  memset (filename, 0, filename_size);
  markcolor = avt_darker (avt_get_balloon_color (), 0x22);

  // don't show the balloon
  avt_show_avatar ();

  // set maximum size
  avt_set_balloon_size (0, 0);

  custom_filter = filter;
  namelist = NULL;

  strncpy (old_encoding, avt_get_mb_encoding (), sizeof (old_encoding));
  old_encoding[sizeof (old_encoding) - 1] = '\0';

  // set the systems default encoding
  // this also catches earlier errors
  if (avt_mb_encoding (NULL) != AVT_NORMAL)
    goto quit;

  while (rcode < 0)
    {
      entries = get_directory (&namelist);
      if (entries < 0)
	goto quit;

      avt_move_xy (1, 1);
      // entry 1 is directory name, entry 2 is parent directory
      if (avt_menu (&choice, entries + 2, show, namelist))
	goto quit;

      if (choice == 1)		// path
	{
	  char dirname[AVT_LINELENGTH + 1];

	  avt_move_xy (1, 1);
	  marked ();
	  avt_clear_line ();
	  avt_ask_mb (dirname, sizeof (dirname));
	  avt_normal_text ();
	  if (*dirname)
	    chdir (dirname);
	}
      else if (choice == 2)	// parent directory
	{
	  if (HAS_DRIVE_LETTERS and is_root_dir ())	// ask for drive?
	    {
	      if (avta_ask_drive (avt_get_max_y ()) != AVT_NORMAL)
		goto quit;

	      avt_set_balloon_size (0, 0);
	    }
	  else
	    chdir ("..");
	}
      else if (strlen (namelist[choice - 3]->d_name) < (size_t) filename_size)
	{
	  strcpy (filename, namelist[choice - 3]->d_name);

	  // directory chosen?
	  if (is_directory (filename))
	    {
	      chdir (filename);
	      memset (filename, 0, filename_size);
	    }
	  else
	    rcode = 0;		// found
	}

      // free namelist
      while (entries--)
	free (namelist[entries]);
      free (namelist);
      namelist = NULL;
    }

quit:
  if (namelist)
    {
      while (entries--)
	free (namelist[entries]);
      free (namelist);
      namelist = NULL;
    }

  custom_filter = NULL;
  avt_mb_encoding (old_encoding);

  return rcode;
}
