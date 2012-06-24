/*
 * filechooser - filechooser dialog for AKFAvatar
 * Copyright (c) 2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
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
#define _GNU_SOURCE

#include "akfavatar.h"
#include "avtaddons.h"
#include "avtinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


#define marked(void) avt_set_text_background_color(0xDDDDDD)

// entries or marks that are not files
#define marked_text(S) \
         marked (); avt_say (S); avt_normal_text ()

#define marked_line(S) \
         do { \
           marked(); avt_clear_line (); \
           avt_move_x (max_x/2-(sizeof(S)/sizeof(wchar_t)-1)/2); \
           avt_say(S); \
           avt_normal_text(); \
         } while(0)

// double arrow up
#define PARENT_DIRECTORY L" \x21D1 "

// House symbol
#define HOME L" \x2302 "

// three arrows up
#define BACK L"\x2191 \x2191 \x2191"

// three arrows down
#define CONTINUE L"\x2193 \x2193 \x2193"

// 3 dots
#define LONGER L"\x2026"

// slash
#define DIRECTORY L"/"

#ifdef _WIN32
#  define HAS_DRIVE_LETTERS true
#  define HAS_SCANDIR false
#  define is_root_dir(x) (x[1] == ':' && x[3] == '\0')
#else
#  define HAS_DRIVE_LETTERS false
#  define HAS_SCANDIR true
#  define is_root_dir(x) (x[1] == '\0')
#  define avta_ask_drive(max_idx) 0	// dummy
#endif

// variable for custom filter
static avta_filter custom_filter = NULL;

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
	      || ((d->d_type == DT_UNKNOWN || d->d_type == DT_LNK) \
	             && is_directory (d->d_name)))
#else
#  define is_dirent_directory(d) (is_directory (d->d_name))
#endif // _DIRENT_HAVE_D_TYPE

static void
new_page (void)
{
  avt_lock_updates (true);
  avt_clear ();
}

static void
show_directory (char *dirname)
{
  avt_lock_updates (true);
  avt_clear ();
  marked ();
  avt_say_mb (dirname);
  avt_clear_eol ();
  avt_normal_text ();
  avt_new_line ();
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
  if (d == NULL || d->d_name[0] == '.')
    return false;
  else if (is_dirent_directory (d))
    return true;
  else
    return (custom_filter == NULL || (*custom_filter) (d->d_name));
}

#else // _WIN32

static int
filter_dirent (const struct dirent *d)
{
  // don't allow "." and ".." and apply custom_filter
  if (d == NULL)
    return false;
  else if (strcmp (".", d->d_name) == 0 || strcmp ("..", d->d_name) == 0)
    return false;
  else if (is_dirent_directory (d))
    return true;
  else
    return (custom_filter == NULL || (*custom_filter) (d->d_name));
}

#endif // _WIN32

#if (HAS_SCANDIR)
#  define get_directory(list) (scandir (".", list, filter_dirent, alphasort))
#else // not HAS_SCANDIR

static int
compare_dirent (const void *a, const void *b)
{
  return strcoll ((*(struct dirent **) a)->d_name,
		  (*(struct dirent **) b)->d_name);
}

static int
get_directory (struct dirent ***list)
{
  const int max_entries = 1024;
  struct dirent **mylist;
  struct dirent *d, *n;
  int entries;
  size_t dirent_size;
  DIR *dir;

  *list = NULL;
  entries = 0;

  // TODO: potential portability problem
  dirent_size = sizeof (struct dirent);	// works for all I have

  /*
     dirent_size = offsetof (struct dirent, d_name)
     + pathconf (".", _PC_NAME_MAX) + 1;
   */

  dir = opendir (".");
  if (dir == NULL)
    return -1;

  mylist = (struct dirent **) malloc (max_entries * sizeof (struct dirent *));

  if (mylist)
    {
      while ((d = readdir (dir)) != NULL && entries < max_entries)
	{
	  if (filter_dirent (d))
	    {
	      n = (struct dirent *) malloc (dirent_size);
	      if (!n)
		break;
	      memcpy (n, d, dirent_size);
	      mylist[entries++] = n;
	    }
	}

      // sort
      qsort (mylist, entries, sizeof (struct dirent *), compare_dirent);
    }

  closedir (dir);

  if (!mylist)
    return -1;

  *list = mylist;
  return entries;
}

#endif

/*
 * filechooser
 * coose a file, starting in working directory
 * return -1 on error or 0 on success
 */
extern int
avta_file_selection (char *filename, int filename_size, avta_filter filter)
{
  int rcode;			// return code
  struct dirent *d;
  struct dirent **namelist;
  char dirname[4096];
  int max_x, max_idx, page_entries;
  int idx, menu_entry, page_nr;
  int entries, entry_nr;
  char *entry[100];		// entry on screen
  char old_encoding[100];
  bool old_auto_margin, old_newline_mode;

  rcode = -1;

  if (filename == NULL || filename_size <= 0)
    return -1;

  avt_set_text_delay (0);
  avt_normal_text ();

  // don't show the balloon
  avt_show_avatar ();

  // set maximum size
  avt_set_balloon_size (0, 0);

  max_x = avt_get_max_x ();
  max_idx = avt_get_max_y ();
  page_entries = max_idx - 2;	// minus back and forward entries
  custom_filter = filter;
  namelist = NULL;

  old_auto_margin = avt_get_auto_margin ();
  avt_set_auto_margin (false);
  old_newline_mode = avt_get_newline_mode ();
  avt_newline_mode (true);

  strncpy (old_encoding, avt_get_mb_encoding (), sizeof (old_encoding));
  old_encoding[sizeof (old_encoding) - 1] = '\0';

  // set the systems default encoding
  // this also catches earlier errors
  if (avt_mb_encoding (NULL) != AVT_NORMAL)
    goto quit;

start:
  // returncode: assume failure as default
  rcode = -1;
  menu_entry = -1;
  page_nr = 0;
  entries = 0;
  entry_nr = 0;
  *filename = '\0';
  idx = 0;

  if (!getcwd (dirname, sizeof (dirname)))
    dirname[0] = '\0';

  show_directory (dirname);
  idx++;			// for the directory-line

  entries = get_directory (&namelist);
  if (entries < 0)
    goto quit;

  // entry for parent directory or home
  if (!HAS_DRIVE_LETTERS && is_root_dir (dirname))
    {
      entry[idx] = "";
      marked_text (HOME);
    }
  else
    {
      entry[idx] = "..";
      marked_text (PARENT_DIRECTORY);
    }
  idx++;
  avt_new_line ();

  while (!*filename)
    {
      if (entry_nr < entries)
	d = namelist[entry_nr++];
      else
	d = NULL;

      if (idx == 1 && !d)	// no entries at all
	break;

      // end reached?
      if (!d || idx == max_idx - 1)
	{
	  if (d)		// continue entry
	    {
	      marked_line (CONTINUE);
	      idx++;
	    }

	  avt_lock_updates (false);
	  if (avt_choice (&menu_entry, 1, idx, 0, (page_nr > 0), (d != NULL)))
	    break;

	  if (page_nr == 0 && menu_entry == 1)	// path-bar
	    {
	      break;
	    }
	  else if (d && menu_entry == idx)	// continue?
	    {
	      idx = 0;
	      page_nr++;

	      new_page ();
	      entry[idx] = "";
	      marked_line (BACK);
	      idx++;
	      avt_new_line ();
	    }
	  else if (page_nr > 0 && menu_entry == 1)	// back
	    {
	      idx = 0;
	      page_nr--;
	      entry_nr = page_nr * page_entries;

	      new_page ();
	      if (page_nr > 0)
		{
		  entry_nr--;	// first page had one extra entry
		  entry[idx] = "";
		  marked_line (BACK);
		}
	      else		// first page
		{
		  show_directory (dirname);
		  idx++;

		  if (!HAS_DRIVE_LETTERS && is_root_dir (dirname))
		    {
		      entry[idx] = "";
		      marked_text (HOME);
		    }
		  else
		    {
		      entry[idx] = "..";
		      marked_text (PARENT_DIRECTORY);
		    }
		}

	      idx++;
	      avt_new_line ();
	      continue;
	    }
	  else			// file chosen
	    {
	      if (strlen (entry[menu_entry - 1]) < (size_t) filename_size)
		{
		  strcpy (filename, entry[menu_entry - 1]);
		  rcode = 0;
		}
	      break;
	    }
	}

      // copy name into entry
      entry[idx] = d->d_name;
      avt_say_mb (entry[idx]);

      // is it a directory?
      if (is_dirent_directory (d))
	{
	  // mark as directory
	  if (avt_where_x () > max_x)
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

      avt_new_line ();
      idx++;
    }

  // free namelist
  while (entries--)
    free (namelist[entries]);
  free (namelist);
  namelist = NULL;

  // path chosen
  if (page_nr == 0 && menu_entry == 1)
    {
      avt_move_xy (1, 1);
      marked ();
      avt_clear_line ();
      avt_ask_mb (dirname, sizeof (dirname));
      avt_normal_text ();
      if (*dirname)
	chdir (dirname);
      goto start;
    }

  // back-entry in root_dir
  if (page_nr == 0 && menu_entry == 2 && is_root_dir (dirname))
    {
      *filename = '\0';
      if (HAS_DRIVE_LETTERS)	// ask for drive?
	{
	  if (avta_ask_drive (max_idx + 1) == AVT_NORMAL)
	    {
	      avt_set_balloon_size (0, 0);
	      goto start;
	    }
	  else
	    goto quit;
	}
      else			// return to main menu
	goto quit;
    }

  // directory chosen?
  if (is_directory (filename))
    {
      chdir (filename);
      goto start;
    }

quit:
  custom_filter = NULL;
  avt_set_auto_margin (old_auto_margin);
  avt_newline_mode (old_newline_mode);
  avt_mb_encoding (old_encoding);
  avt_clear ();
  avt_lock_updates (false);

  return rcode;
}
