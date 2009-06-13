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
#include "avtmsg.h"
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

/* how many pages? */
#define MAXPAGES 500

#ifdef __WIN32__
#  define HAS_DRIVE_LETTERS AVT_TRUE
#  define HAS_SCANDIR AVT_FALSE
#  define is_root_dir(x) (x[1] == ':' && x[3] == '\0')
extern int ask_drive (int max_idx);
#else
#  define HAS_DRIVE_LETTERS AVT_FALSE
#  define HAS_SCANDIR AVT_TRUE
#  define is_root_dir(x) (x[1] == '\0')
#  define ask_drive(max_idx) 0	/* dummy */
#endif

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
new_page (char *dirname)
{
  avt_clear ();
  avt_set_text_background_color (0xdd, 0xdd, 0xdd);
  avt_say_mb (dirname);
  avt_clear_eol ();
  avt_normal_text ();
  avt_move_xy (1, 2);
}

#if (HAS_SCANDIR == AVT_TRUE)

static int
get_directory (struct dirent ***list)
{
  return scandir (".", list, NULL, alphasort);
}

#else /* not HAS_SCANDIR */

static int
compare_dirent (const void *a, const void *b)
{
  return strcmp ((*(struct dirent **) a)->d_name,
		 (*(struct dirent **) b)->d_name);
}

static int
get_directory (struct dirent ***list)
{
  const int max_entries = 1024;
  struct dirent **mylist;
  struct dirent *d, *n;
  int entries;
  DIR *dir;

  *list = NULL;
  entries = 0;

  dir = opendir (".");
  if (dir == NULL)
    return -1;

  mylist = (struct dirent **) malloc (max_entries * sizeof (struct dirent *));

  while ((d = readdir (dir)) != NULL && entries < max_entries)
    {
      n = (struct dirent *) malloc (sizeof (struct dirent));
      memcpy (n, d, sizeof (struct dirent));
      mylist[entries++] = n;
    }

  if (closedir (dir) < 0)
    warning_msg ("closedir", "error");

  /* sort */
  qsort (mylist, entries, sizeof (struct dirent *), compare_dirent);

  *list = mylist;
  return entries;
}

#endif

/* 
 * filechooser
 * lists files in working directory
 * return -1 on error or 0 on success
 */
int
get_file (char *filename)
{
  int rcode;
  struct dirent *d;
  int max_x, max_idx;
  int idx;
  int filenr;
  char dirname[4096];
  char entry[100][256];
  int page_nr;
  off_t pages[MAXPAGES];
  struct dirent **namelist;
  int entries;
  int entry_nr;

  avt_set_text_delay (0);
  avt_normal_text ();

  /* don't show the balloon */
  avt_show_avatar ();
  /* set maximum size */
  avt_set_balloon_size (0, 0);

  max_x = avt_get_max_x ();
  max_idx = avt_get_max_y () - 1;

  if (HAS_DRIVE_LETTERS)
    if (ask_drive (max_idx + 1))
      return -1;

  /* set maximum size */
  avt_set_balloon_size (0, 0);

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
    warning_msg ("getcwd", strerror (errno));

  avt_auto_margin (AVT_FALSE);
  new_page (dirname);

  entries = get_directory (&namelist);
  if (entries < 0)
    return rcode;

  pages[page_nr] = entry_nr;

  /* entry for parent directory or home */
  if (!HAS_DRIVE_LETTERS && is_root_dir (dirname))
    {
      strcpy (entry[idx], "");
      MARK (HOME);
    }
  else
    {
      strcpy (entry[idx], "..");
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

      if (!d || (d && d->d_name[0] != '.'))
	{
	  /* end reached? */
	  if (!d || idx == max_idx - 1)
	    {
	      if (d)		/* continue entry */
		{
		  MARK (CONTINUE);
		  idx++;
		}

	      if (avt_choice (&filenr, 2, idx, 0, (page_nr > 0), (d != NULL)))
		break;


	      if (d && filenr == idx)	/* continue? */
		{
		  idx = 0;
		  page_nr++;
		  if (page_nr > MAXPAGES - 1)
		    page_nr = MAXPAGES - 1;

		  new_page (dirname);
		  entry[idx][0] = '\0';
		  MARK (BACK);
		  idx++;
		  avt_new_line ();
		}
	      else if (filenr == 1 && page_nr > 0)	/* back */
		{
		  page_nr--;
		  entry_nr = pages[page_nr];

		  idx = 0;

		  new_page (dirname);
		  if (page_nr > 0)
		    {
		      entry[idx][0] = '\0';
		      MARK (BACK);
		    }
		  else		/* first page */
		    {
		      if (!HAS_DRIVE_LETTERS && is_root_dir (dirname))
			{
			  strcpy (entry[idx], "");
			  MARK (HOME);
			}
		      else
			{
			  strcpy (entry[idx], "..");
			  MARK (PARENT_DIRECTORY);
			}
		    }

		  idx++;
		  avt_new_line ();
		  continue;
		}
	      else		/* file chosen */
		{
		  strcpy (filename, entry[filenr - 1]);
		  rcode = 0;
		  break;
		}
	    }

	  /* copy name into entry */
	  strncpy (entry[idx], d->d_name, sizeof (entry[idx]));
	  avt_say_mb (entry[idx]);

	  /* is it a directory? */
#ifdef _DIRENT_HAVE_D_TYPE
	  /* faster */
	  if (d->d_type == DT_DIR
	      || (d->d_type == DT_LNK && is_directory (d->d_name)))
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

	  avt_new_line ();

	  idx++;
	  if (idx == max_idx - 1 && page_nr < MAXPAGES - 1)
	    pages[page_nr + 1] = entry_nr;
	}
    }

  /* free namelist */
  {
    int i;
    for (i = 0; i < entries; i++)
      free (namelist[i]);
    free (namelist);
    namelist = NULL;
  }

  /* back-entry in root_dir */
  if (filenr == 1 && is_root_dir (dirname))
    {
      *filename = '\0';
      if (HAS_DRIVE_LETTERS)	/* ask for drive? */
	{
	  if (ask_drive (max_idx + 1) == AVT_NORMAL)
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
	warning_msg (filename, "cannot chdir");
      goto start;
    }

quit:
  avt_auto_margin (AVT_TRUE);
  avt_clear ();

  return rcode;
}
