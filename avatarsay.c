/* 
 * avatarsay - show a textfile with libavatar
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
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

/* $Id: avatarsay.c,v 2.15 2007-10-23 08:00:32 akf Exp $ */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "version.h"
#include "akfavatar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NOFIFO
#  include <sys/stat.h>
#endif

#ifdef __WIN32__
#  include <windows.h>
#endif

#define PRGNAME "avatarsay"

/* encoding of the input files */
/* supported in SDL: ASCII, ISO-8859-1, UTF-8, UTF-16, UTF-32 */
/* ISO-8859-1 is a sane default */
static char encoding[80] = "ISO-8859-1";

/* if rawmode is set, then don't interpret any commands or comments */
/* rawmode can be activated with the options -r or --raw */
static int rawmode = 0;

/* popup-mode? */
static int popup = 0;

/* stop the program? */
static int stop = 0;

/* 
 * ignore end of file conditions
 * this should be used, when the input doesn't come from a file
 */
static int ignore_eof = 0;

/* create a fifo for what to say? */
static int say_pipe = 0;

/* whether to run in a window, or in fullscreen mode */
/* the mode can be set by -f, --fullscreen or -w, --window */
/* or with --fullfullscreen or -F */
static int mode = WINDOW;

/* is the avatar initialized? (0|1) */
/* for delaying the initialization until it is clear that we actually 
   have data to show */
static int initialized = 0;

/* play it in an endless loop? (0/1) */
/* deactivated, when input comes from stdin */
/* can be deactivated by -1, --once */
static int loop = 1;

/* where to find imagefiles */
static char datadir[512] = "";

/* for loading an avt_image */
static avt_image_t *avt_image = NULL;

/* for loading sound files */
static avt_audio_t *sound = NULL;


static void
quit (int exitcode)
{
  if (initialized)
    {
      if (sound)
	avt_free_audio (sound);

      avt_quit_audio ();
      avt_quit ();
    }

  exit (exitcode);
}


#ifndef __WIN32__

/* 
 * "warning", "notice" and "error" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

static void
warning (const char *msg1, const char *msg2)
{
  if (msg2)
    fprintf (stderr, PRGNAME ": %s: %s\n", msg1, msg2);
  else
    fprintf (stderr, PRGNAME ": %s\n", msg1);
}

static void
notice (const char *msg1, const char *msg2)
{
  warning (msg1, msg2);
}

static void
error (const char *msg1, const char *msg2)
{
  warning (msg1, msg2);
  quit (EXIT_FAILURE);
}

static void
showversion (void)
{
  puts (PRGNAME " (AKFAvatar) " AVTVERSION);
  puts ("Copyright (c) 2007 Andreas K. Foerster\n");
  puts ("License GPLv3+: GNU GPL version 3 or later "
	"<http://gnu.org/licenses/gpl.html>\n");
  puts ("This is free software: you are free to change and "
	"redistribute it.");
  puts ("There is NO WARRANTY, to the extent permitted by law.");

  exit (EXIT_SUCCESS);
}

static void
help (const char *prgname)
{
  printf ("\nUsage: %s [Options] textfile(s)\n\n", prgname);
  puts ("A fancy text-viewer and scripting language for making demos.\n");
  puts ("If textfile is - then read from stdin and don't loop.\n");
  puts ("Options:");
  puts (" -h, --help              show this help");
  puts (" -v, --version           show the version");
  puts (" -w, --window            try to run the program in a window"
	" (default)");
  puts (" -f, --fullscreen        try to run the program in fullscreen mode");
  puts (" -F, --fullfullscreen    like -f, but use current display-size");
  puts ("     --encoding=enc      input data is encoded in encoding \"enc\"");
  puts (" -l, --latin1            input data is encoded in Latin-1");
  puts (" -u, --utf-8             input data is encoded in UTF-8");
  puts (" -1, --once              run only once (don't loop)");
  puts ("     --popup             popup, ie. don't move the avatar in");
  puts (" -r, --raw               output raw text"
	" (don't handle any commands)");
  puts (" -i, --ignoreeof         ignore end of file conditions "
	"(input is not a file)");
#ifdef NOFIFO
  puts (" -s, --saypipe           not supported on this system");
#else
  puts (" -s, --saypipe           create named pipe for filename");
#endif
  puts ("\nEnvironment variables:");
  puts (" AVATARIMAGE             different image as avatar");
  puts (" AVATARDATADIR           data-directory");
  puts (" LC_ALL, LC_CTYPE, LANG  check for default encoding");
  puts ("\nReport bugs to <info@akfoerster.de>");
  exit (EXIT_SUCCESS);
}

#else /* Windows or ReactOS */

static void
warning (const char *msg1, const char *msg2)
{
  char msg[1024];

  strcpy (msg, msg1);

  if (msg2)
    {
      strcat (msg, ": ");
      strcat (msg, msg2);
    }

  MessageBox (NULL, msg, PRGNAME, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
}

/* ignore unimportant notices on Windows */
static void
notice (const char *msg1, const char *msg2)
{
}

static void
error (const char *msg1, const char *msg2)
{
  char msg[1024];

  strcpy (msg, msg1);

  if (msg2)
    {
      strcat (msg, ": ");
      strcat (msg, msg2);
    }

  MessageBox (NULL, msg, PRGNAME, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
  quit (EXIT_FAILURE);
}

static void
showversion (void)
{
  char msg[] = PRGNAME " (AKFAvatar) " AVTVERSION "\n"
    "Copyright \xa9 2007 Andreas K. F\xf6rster\n\n"
    "License GPLv3+: GNU GPL version 3 or later "
    "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n\n"
    "Please read the manual for instructions.";

  MessageBox (NULL, msg, PRGNAME,
	      MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);

  exit (EXIT_SUCCESS);
}

/* For Windows users it's not useful to show the options */
/* But the text refers to the manual */
static void
help (const char *prgname)
{
  showversion ();
  exit (EXIT_SUCCESS);
}

#endif /* Windows or ReactOS */

static void
set_encoding (const char *encoding)
{
  if (avt_mb_encoding (encoding))
    error ("charset encoding not supported", avt_get_error() );
}

static void
check_system_encoding (void)
{
  char *s;

  s = getenv ("LC_ALL");
  if (s == NULL)
    s = getenv ("LC_CTYPE");
  if (s == NULL)
    s = getenv ("LANG");
  if (s == NULL)
    return;			/* give up */

  if (strstr (s, "UTF") || strstr (s, "utf"))
    strcpy (encoding, "UTF-8");
  else if (strstr (s, "euro"))
    strcpy (encoding, "ISO-8859-15");
  else if (strstr (s, "KOI8-R") || strstr (s, "KOI8R"))
    strcpy (encoding, "KOI8-R");
  else if (strstr (s, "KOI8-U") || strstr (s, "KOI8U"))
    strcpy (encoding, "KOI8-U");
  else if (strstr (s, "KOI8"))
    strcpy (encoding, "KOI8");
  else				/* ISO 8859- family */
    {
      char *p = strstr (s, "8859-");
      if (p)
	{
	  strcpy (encoding, "ISO-8859-");
	  p += 5;
	  /* two digits following? */
	  if (*(p + 1) >= '0' && *(p + 1) <= '9')
	    strncat (encoding, p, 2);
	  else
	    strncat (encoding, p, 1);
	}
    }
}

static void
checkoptions (int argc, char **argv)
{
  int i;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
	help (argv[0]);

      if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
	showversion ();

      if (strcmp (argv[i], "--fullscreen") == 0
	  || strcmp (argv[i], "-f") == 0)
	{
	  mode = FULLSCREEN;
	  continue;
	}

      if (strcmp (argv[i], "--fullfullscreen") == 0
	  || strcmp (argv[i], "-F") == 0)
	{
	  mode = FULLSCREENNOSWITCH;
	  continue;
	}

      if (strcmp (argv[i], "--window") == 0 || strcmp (argv[i], "-w") == 0)
	{
	  mode = WINDOW;
	  continue;
	}

      if (strcmp (argv[i], "--once") == 0 || strcmp (argv[i], "-1") == 0)
	{
	  loop = 0;
	  continue;
	}

      if (strcmp (argv[i], "--raw") == 0 || strcmp (argv[i], "-r") == 0)
	{
	  rawmode = 1;
	  continue;
	}

      if (strcmp (argv[i], "--ignoreeof") == 0 || strcmp (argv[i], "-i") == 0)
	{
	  ignore_eof = 1;
	  continue;
	}

      if (strcmp (argv[i], "--saypipe") == 0 || strcmp (argv[i], "-s") == 0)
	{
#ifdef NOFIFO
	  error ("pipes not supported on this system", NULL);
#else
	  say_pipe = 1;
	  loop = 0;
	  ignore_eof = 1;
	  continue;
#endif /* not NOFIFO */
	}

      if (strncmp (argv[i], "--encoding=", 11) == 0)
	{
	  sscanf (argv[i], "--encoding=%79s", (char *) &encoding);
	  continue;
	}

      if (strcmp (argv[i], "--latin1") == 0 || strcmp (argv[i], "-l") == 0)
	{
	  strcpy (encoding, "ISO-8859-1");
	  continue;
	}

      if (strcmp (argv[i], "--utf-8") == 0 ||
	  strcmp (argv[i], "--utf8") == 0 ||
	  strcmp (argv[i], "-u8") == 0 || strcmp (argv[i], "-u") == 0)
	{
	  strcpy (encoding, "UTF-8");
	  continue;
	}

      if (strcmp (argv[i], "--popup") == 0)
	{
	  popup = 1;
	  continue;
	}

      /* check for unknown option */
      if (argv[i][0] == '-' && argv[i][1] != '\0')
	error ("unknown option", argv[i]);
    }				/* for */
}

static void
checkenvironment (void)
{
  char *e;

  e = getenv ("AVATARDATADIR");
  if (e)
    strncpy (datadir, e, sizeof (datadir));

  e = getenv ("AVATARIMAGE");
  if (e && !avt_image)
    if (!(avt_image = avt_import_image_file (e)))
      error ("error while loading the AVATARIMAGE", avt_get_error ());
}

static void
move_in (void)
{
  if (initialized)
    {
      if (avt_move_in ())
	quit (EXIT_SUCCESS);
      if (avt_wait (seconds (2.0)))
	quit (EXIT_SUCCESS);
    }
}

static void
move_out (void)
{
  if (initialized)
    {
      if (avt_move_out ())
	quit (EXIT_SUCCESS);

      /* if running in a loop, wait a while */
      if (loop)
	if (avt_wait (seconds (5.0)))
	  quit (EXIT_SUCCESS);
    }
}

static void
initialize (void)
{
  if (!avt_image)
    avt_image = avt_default ();

  if (avt_initialize ("AKFAvatar", "AKFAvatar", avt_image, mode))
    error ("cannot initialize graphics", avt_get_error ());

  if (avt_initialize_audio ())
    notice ("cannot initialize audio", avt_get_error ());

  initialized = 1;
}

static void
handle_image_command (const char *s)
{
  char filename[255];
  char file[512];

  if (sscanf (s, ".image %255s", (char *) &filename) > 0)
    {
      strcpy (file, datadir);
      if (file[0] != '\0')
	strcat (file, "/");
      strncat (file, filename, sizeof (file) - 1 - strlen (datadir));

      if (!initialized)
	initialize ();
      else if (avt_wait (2500))
	quit (EXIT_SUCCESS);
      if (!avt_show_image_file (file))
	if (avt_wait (7000))
	  quit (EXIT_SUCCESS);
    }
}

static void
handle_avatarimage_command (const char *s)
{
  char filename[255];
  char file[512];

  /* if already assigned, delete it */
  if (avt_image)
    avt_free_image (avt_image);

  if (sscanf (s, ".avatarimage %255s", (char *) &filename) > 0)
    {
      strcpy (file, datadir);
      if (file[0] != '\0')
	strcat (file, "/");
      strncat (file, filename, sizeof (file) - 1 - strlen (datadir));
      if (!(avt_image = avt_import_image_file (file)))
	warning ("warning", avt_get_error ());
    }
}

static void
handle_backgoundcolor_command (const char *s)
{
  unsigned int red, green, blue;

  if (sscanf (s, ".backgroundcolor #%2x%2x%2x", &red, &green, &blue) == 3)
    avt_set_background_color (red, green, blue);
  else
    error ("formatting error for \".backgroundcolor\"", NULL);
}

static void
handle_audio_command (const char *s)
{
  char filename[255];
  char file[512];

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (sscanf (s, ".audio %255s", (char *) &filename) > 0)
    {
      if (sound)
	avt_free_audio (sound);
      sound = NULL;

      strcpy (file, datadir);
      if (file[0] != '\0')
	strcat (file, "/");
      strncat (file, filename, sizeof (file) - 1 - strlen (datadir));

      sound = avt_load_wave_file (file);
      if (!sound)
	{
	  notice ("can not load audio file", avt_get_error ());
	  return;
	}

      if (avt_play_audio (sound, 0))
	notice ("can not play audio file", avt_get_error ());
    }
}

static void
handle_back_command (const char *s)
{
  int i, value;


  if (!initialized)
    return;

  if (sscanf (s, ".back %d", &value) > 0)
    {
      for (i = 0; i < value; i++)
	avt_backspace ();
    }
  else
    avt_backspace ();
}

static void
handle_read_command (const char *s)
{
  wchar_t line[LINELENGTH];

  if (!initialized)
    initialize ();

  if (avt_ask (line, sizeof (line)))
    quit (EXIT_SUCCESS);

  /* TODO: not fully implemented yet */
}

/* removes trailing space, newline, etc. */
static void
strip (char **s)
{
  char *p;

  p = *s;

  /* search end of string */
  while (*(p + 1) != '\0')
    p++;

  /* search for last non-space char */
  while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')
    p--;

  *(p + 1) = '\0';
}


/* handle commads, including comments */
static int
iscommand (char *s)
{
  if (rawmode)
    return 0;

  /* 
   * a stripline begins with at least 3 dashes
   * or it begins with \f 
   * the rest of the line is ignored
   */
  if (strncmp (s, "---", 3) == 0 || s[0] == '\f')
    {
      if (initialized)
	if (avt_flip_page ())
	  stop = 1;
      return 1;
    }

  if (s[0] == '.')
    {
      strip (&s);

      /* new datadir */
      if (strncmp (s, ".datadir ", 9) == 0)
	{
	  sscanf (s, ".datadir %255s", (char *) &datadir);
	  return 1;
	}

      if (strncmp (s, ".avatarimage ", 13) == 0)
	{
	  if (!initialized)
	    handle_avatarimage_command (s);

	  return 1;
	}

      if (strncmp (s, ".encoding ", 10) == 0)
	{
	  if (sscanf (s, ".encoding %79s", (char *) &encoding) <= 0)
	    warning ("warning", "cannot read the \".encoding\" line.");
	  else
	    set_encoding (encoding);

	  return 1;
	}

      if (strncmp (s, ".backgroundcolor ", 17) == 0)
	{
	  if (!initialized)
	    handle_backgoundcolor_command (s);

	  return 1;
	}

      /* default - for most languages */
      if (strcmp (s, ".left-to-right") == 0)
	{
	  avt_text_direction (LEFT_TO_RIGHT);
	  return 1;
	}

      /* currently only hebrew/yiddish supported */
      if (strcmp (s, ".right-to-left") == 0)
	{
	  avt_text_direction (RIGHT_TO_LEFT);
	  return 1;
	}

      /* new page - same as \f or stripline */
      if (strcmp (s, ".flip") == 0)
	{
	  if (initialized)
	    if (avt_flip_page ())
	      stop = 1;
	  return 1;
	}

      /* clear ballon - don't wait */
      if (strcmp (s, ".clear") == 0)
	{
	  if (initialized)
	    avt_clear ();
	  return 1;
	}

      /* longer intermezzo */
      if (strcmp (s, ".pause") == 0)
	{
	  if (!initialized)
	    initialize ();
	  else if (avt_wait (2700))
	    stop = 1;

	  avt_show_avatar ();
	  if (avt_wait (4000))
	    stop = 1;
	  return 1;
	}

      /* show image */
      if (strncmp (s, ".image ", 7) == 0)
	{
	  handle_image_command (s);
	  return 1;
	}

      /* play sound */
      if (strncmp (s, ".audio ", 7) == 0)
	{
	  handle_audio_command (s);
	  return 1;
	}

      /* wait until sound ends */
      if (strcmp (s, ".waitaudio") == 0)
	{
	  if (initialized)
	    if (avt_wait_audio_end ())
	      stop = 1;
	  return 1;
	}

      /* 
       * pause for effect in a sentence
       * the previous line should end with a backslash
       */
      if (strcmp (s, ".effectpause") == 0)
	{
	  if (initialized)
	    if (avt_wait (2500))
	      stop = 1;
	  return 1;
	}

      /* 
       * move back a number of characters
       * the previous line has to end with a backslash!
       */
      if (strncmp (s, ".back ", 6) == 0)
	{
	  handle_back_command (s);
	  return 1;
	}

      if (strncmp (s, ".read ", 6) == 0)
	{
	  handle_read_command (s);
	  return 1;
	}

      if (strcmp (s, ".end") == 0)
	{
	  if (initialized)
	    avt_move_out ();
	  stop = 1;
	  return 1;
	}

      if (strcmp (s, ".stop") == 0)
	{
	  /* doesn't matter whether it's initialized */
	  stop = 1;
	  return 1;
	}

      /* silently ignore unknown commands */
      return 1;
    }

  if (s[0] == '#')
    return 1;

  return 0;
}

/* if line ends on \\n, strip it */
static char *
process_line_end (char *s)
{
  char *p;

  /* in rawmode don't change anything */
  if (rawmode)
    return s;

  p = strrchr (s, '\\');
  if (p)
    if (*(p + 1) == '\n' || (*(p + 1) == '\r' && *(p + 2) == '\n'))
      *p = '\0';

  return s;
}

#ifndef __USE_GNU

/* inferior replacement for GNU specific getline */
static ssize_t
getline (char **lineptr, size_t * n, FILE * stream)
{
  /* reserve a fixed size of memory */
  if (*n == 0)
    {
      *n = 10240;
      *lineptr = (char *) malloc (*n);
    }

  if (fgets (*lineptr, *n, stream))
    return strlen (*lineptr);
  else
    return EOF;
}

#endif /* not __USE_GNU */

/* check for byte order mark (BOM) U+FEFF and remove it */
static void
check_bom (char *line, int len)
{
  /* UTF-8 BOM (as set by Windows notepad) */
  if (line[0] == '\xEF' && line[1] == '\xBB' && line[2] == '\xBF')
    {
      strcpy (encoding, "UTF-8");
      set_encoding (encoding);
      memmove (line, line + 3, len - 3);
    }

  /* UTF-16BE BOM (doesn't work) */
  if (line[0] == '\xFE' && line[1] == '\xFF')
    error ("UTF-16BE Unicode not supported", "please use UTF-8 for Unicode");

  /* UTF-16LE BOM (doesn't work) */
  if (line[0] == '\xFF' && line[1] == '\xFE')
    error ("UTF-16LE Unicode not supported", "please use UTF-8 for Unicode");

  /* UTF-32BE BOM (doesn't work) */
  if (line[0] == '\x00' && line[1] == '\x00'
      && line[2] == '\xFE' && line[3] == '\xFF')
    error ("UTF-32BE Unicode not supported", "please use UTF-8 for Unicode");

  /* UTF-32LE BOM (doesn't work) */
  if (line[0] == '\xFF' && line[1] == '\xFE'
      && line[2] == '\x00' && line[3] == '\x00')
    error ("UTF-32LE Unicode not supported", "please use UTF-8 for Unicode");
}

/* shows content of file / other input */
static int
processfile (const char *fname)
{
  FILE *text;
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;

  if (strcmp (fname, "-") == 0)
    text = stdin;
  else
    {
#ifndef NOFIFO
      if (say_pipe)
	{
	  if (mkfifo (fname, 0600))
	    error ("error creating fifo", fname);
	}
#endif /* not NOFIFO */
      text = fopen (fname, "rt");
    }


  if (text == NULL)
    error ("error opening file for reading", fname);

  if (!rawmode)
    do				/* skip empty lines at the beginning of the file */
      {
	read = getline (&line, &len, text);

	if (read != EOF)
	  check_bom (line, len);

	/* simulate empty lines when ignore_eof is set */
	if (ignore_eof && read == EOF)
	  {
	    clearerr (text);
	    strcpy (line, "\n");
	    read = 1;
	    if (avt_wait (10))
	      stop = 1;
	  }
      }
    while (read != EOF
	   && (strcmp (line, "\n") == 0
	       || strcmp (line, "\r\n") == 0 || iscommand (line)) && !stop);

  if (!initialized && !stop)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  /* show text */
  if (line && !stop)
    stop = avt_say_mb (process_line_end (line));

  while (!stop && (read != EOF || ignore_eof))
    {
      read = getline (&line, &len, text);

      if (ignore_eof)
	{
	  /* wait for input */
	  while (read == EOF)
	    {
	      clearerr (text);
	      
	      if (avt_wait (10))
		{
		  stop = 1;
		  break;
		}
	      else
		read = getline (&line, &len, text);
	    }
	}

      if (read != EOF && !stop && !iscommand (line))
	if (avt_say_mb (process_line_end (line)))
	  stop = 1;
    }

  if (line)
    free (line);

  if (text != stdin)
    fclose (text);

#ifndef NOFIFO
  if (say_pipe)
    remove (fname);
#endif /* not NOFIFO */

  if (avt_get_status () == AVATARERROR)
    {
      stop = 1;
      warning ("warning", avt_get_error ());
    }

  return stop;
}

int
main (int argc, char *argv[])
{
  int i;

  if (argc < 2)
    help (argv[0]);

  checkenvironment ();
  check_system_encoding ();
  checkoptions (argc, argv);

  set_encoding (encoding);

  do
    {
      if (initialized && !popup)
	move_in ();

      for (i = 1; i < argc; i++)
	{
	  /* ignore options here */
	  if (argv[i][0] == '-' && argv[i][1] != '\0')
	    continue;

	  if (processfile (argv[i]))
	    quit (EXIT_SUCCESS);
	  if (avt_flip_page ())
	    quit (EXIT_SUCCESS);

	  /* 
	   * ignore anything past "-" 
	   * and don't loop then (it would break things)
	   */
	  if (strcmp (argv[i], "-") == 0)
	    {
	      loop = 0;
	      break;
	    }
	}

      move_out ();
    }
  while (loop);

  quit (EXIT_SUCCESS);
  return EXIT_SUCCESS;
}
