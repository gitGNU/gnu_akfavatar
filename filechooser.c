/*
 * filechooser - filechooser dialog for AKFAvatar
 * Copyright (c) 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
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

#include "akfavatar.h"
#include "avtaddons.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* entries or marks that are not files */
#define MARK(S) \
         avt_set_text_background_color (0xdd, 0xdd, 0xdd); \
         avt_say (S); avt_normal_text ()

#define PARENT_DIRECTORY L" .. "

/* House symbol */
#define HOME L" \x2302 "

/* three arrows up */
#define BACK L" \x2191 \x2191 \x2191 "

/* three arrows down */
#define CONTINUE L" \x2193 \x2193 \x2193 "

/* 3 dots */
#define LONGER L"\x2026"

/* slash */
#define DIRECTORY L"/"

#ifdef __WIN32__
#  define HAS_DRIVE_LETTERS AVT_TRUE
#  define HAS_SCANDIR AVT_FALSE
#  define is_root_dir(x) (x[1] == ':' && x[3] == '\0')
extern int avta_ask_drive (int max_idx);
#else
#  define HAS_DRIVE_LETTERS AVT_FALSE
#  define HAS_SCANDIR AVT_TRUE
#  define is_root_dir(x) (x[1] == '\0')
#  define avta_ask_drive(max_idx) 0	/* dummy */
#endif

/* variable for custom filter */
static avta_filter_t custom_filter = NULL;

static avt_bool_t
is_directory (const char *name)
{
  struct stat buf;

  if (stat (name, &buf) > -1)
    return (avt_bool_t) S_ISDIR (buf.st_mode);
  else
    return AVT_FALSE;
}

#ifdef _DIRENT_HAVE_D_TYPE
#  define is_dirent_directory(d) \
	    (d->d_type == DT_DIR \
	      || ((d->d_type == DT_UNKNOWN || d->d_type == DT_LNK) \
	             && is_directory (d->d_name)))
#else
#  define is_dirent_directory(d) (is_directory (d->d_name))
#endif /* _DIRENT_HAVE_D_TYPE */

static void
new_page (char *dirname)
{
  avt_lock_updates (AVT_TRUE);
  avt_clear ();
  avt_set_text_background_color (0xdd, 0xdd, 0xdd);
  avt_say_mb (dirname);
  avt_clear_eol ();
  avt_normal_text ();
  avt_move_xy (1, 2);
}

#ifndef __WIN32__

#ifdef __USE_GNU
#  define FILTER_DIRENT_T  const struct dirent
#else
#  define FILTER_DIRENT_T  struct dirent
#endif

static int
filter_dirent (FILTER_DIRENT_T * d)
{
  /* allow nothing that starts with a dot */
  if (d == NULL || d->d_name[0] == '.')
    return AVT_FALSE;
  else if (is_dirent_directory (d))
    return AVT_TRUE;
  else
    return (custom_filter == NULL || (*custom_filter) (d->d_name));
}

#else /* __WIN32__ */

static int
filter_dirent (const struct dirent *d)
{
  /* don't allow "." and ".." and apply custom_filter */
  if (d == NULL)
    return AVT_FALSE;
  else if (strcmp (".", d->d_name) == 0 || strcmp ("..", d->d_name) == 0)
    return AVT_FALSE;
  else if (is_dirent_directory (d))
    return AVT_TRUE;
  else
    return (custom_filter == NULL || (*custom_filter) (d->d_name));
}

#endif /* __WIN32__ */

static int
compare_dirent (const void *a, const void *b)
{
  return strcoll ((*(struct dirent **) a)->d_name,
		  (*(struct dirent **) b)->d_name);
}

#if (HAS_SCANDIR)
#  define get_directory(list) (scandir (".", list, filter_dirent, compare_dirent))
#else /* not HAS_SCANDIR */

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

  /* TODO: potential portability problem */
  dirent_size = sizeof (struct dirent);	/* works for all I have */

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

      /* sort */
      qsort (mylist, entries, sizeof (struct dirent *), compare_dirent);
    }

  if (closedir (dir) < 0)
    avta_warning ("closedir", strerror (errno));

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
avta_file_selection (char *filename, int filename_size, avta_filter_t filter)
{
  int rcode;			/* return code */
  struct dirent *d;
  struct dirent **namelist;
  char dirname[4096];
  int max_x, max_idx, page_entries;
  int idx, filenr, page_nr;
  int entries, entry_nr;
  char *entry[100];		/* entry on screen */

  if (filename == NULL || filename_size <= 0)
    return -1;

  avt_set_text_delay (0);
  avt_normal_text ();

  /* don't show the balloon */
  avt_show_avatar ();

  /* set maximum size */
  avt_set_balloon_size (0, 0);

  max_x = avt_get_max_x ();
  max_idx = avt_get_max_y () - 1;	/* minus top-line */
  page_entries = max_idx - 2;	/* minus back and forward entries */
  custom_filter = filter;

  if (HAS_DRIVE_LETTERS)
    {
      if (avta_ask_drive (max_idx + 1))
	return -1;

      /* set maximum size again */
      avt_set_balloon_size (0, 0);
    }

  namelist = NULL;

start:
  /* returncode: assume failure as default */
  rcode = -1;
  filenr = -1;
  page_nr = 0;
  entries = 0;
  entry_nr = 0;
  *filename = '\0';
  idx = 0;

  if (!getcwd (dirname, sizeof (dirname)))
    avta_warning ("getcwd", strerror (errno));

  avt_auto_margin (AVT_FALSE);
  new_page (dirname);

  entries = get_directory (&namelist);
  if (entries < 0)
    return rcode;

  /* entry for parent directory or home */
  if (!HAS_DRIVE_LETTERS && is_root_dir (dirname))
    {
      entry[idx] = "";
      MARK (HOME);
    }
  else
    {
      entry[idx] = "..";
      MARK (PARENT_DIRECTORY);
    }
  idx++;
  avt_new_line ();

  while (!*filename)
    {
      if (entry_nr < entries)
	d = namelist[entry_nr++];
      else
	d = NULL;

      if (!idx && !d)		/* no entries at all */
	break;

      /* end reached? */
      if (!d || idx == max_idx - 1)
	{
	  if (d)		/* continue entry */
	    {
	      MARK (CONTINUE);
	      idx++;
	    }

	  avt_lock_updates (AVT_FALSE);
	  if (avt_choice (&filenr, 2, idx, 0, (page_nr > 0), (d != NULL)))
	    break;

	  if (d && filenr == idx)	/* continue? */
	    {
	      idx = 0;
	      page_nr++;

	      new_page (dirname);
	      entry[idx] = "";
	      MARK (BACK);
	      idx++;
	      avt_new_line ();
	    }
	  else if (filenr == 1 && page_nr > 0)	/* back */
	    {
	      idx = 0;
	      page_nr--;
	      entry_nr = page_nr * page_entries;

	      new_page (dirname);
	      if (page_nr > 0)
		{
		  entry[idx] = "";
		  MARK (BACK);
		}
	      else		/* first page */
		{
		  if (!HAS_DRIVE_LETTERS && is_root_dir (dirname))
		    {
		      entry[idx] = "";
		      MARK (HOME);
		    }
		  else
		    {
		      entry[idx] = "..";
		      MARK (PARENT_DIRECTORY);
		    }
		}

	      idx++;
	      avt_new_line ();
	      continue;
	    }
	  else			/* file chosen */
	    {
	      if (strlen (entry[filenr - 1]) < (size_t) filename_size)
		{
		  strcpy (filename, entry[filenr - 1]);
		  rcode = 0;
		}
	      break;
	    }
	}

      /* copy name into entry */
      entry[idx] = d->d_name;
      avt_say_mb (entry[idx]);

      /* is it a directory? */
      if (is_dirent_directory (d))
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

      avt_new_line ();
      idx++;
    }

  /* free namelist */
  while (entries--)
    free (namelist[entries]);
  free (namelist);
  namelist = NULL;

  /* back-entry in root_dir */
  if (filenr == 1 && is_root_dir (dirname))
    {
      *filename = '\0';
      if (HAS_DRIVE_LETTERS)	/* ask for drive? */
	{
	  if (avta_ask_drive (max_idx + 1) == AVT_NORMAL)
	    {
	      avt_set_balloon_size (0, 0);
	      goto start;
	    }
	  else
	    goto quit;
	}
      else			/* return to main menu */
	goto quit;
    }

  /* directory chosen? */
  if (is_directory (filename))
    {
      if (chdir (filename))
	avta_warning (filename, "cannot chdir");
      goto start;
    }

quit:
  custom_filter = NULL;
  avt_auto_margin (AVT_TRUE);
  avt_clear ();
  avt_lock_updates (AVT_FALSE);

  return rcode;
}
