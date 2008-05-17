/* 
 * avatarsay - show a textfile with libavatar
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
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

/* $Id: avatarsay.c,v 2.140 2008-05-17 19:54:30 akf Exp $ */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "version.h"
#include "akfavatar.h"
#include <wchar.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <getopt.h>

#ifdef __WIN32__
#  include <windows.h>
#  include <shellapi.h>
#  define NO_MANPAGES 1
#  ifdef __MINGW32__
#    define NO_PTY 1
#    define NO_LANGINFO 1
#  endif
#endif

#ifndef NO_LANGINFO
#  include <langinfo.h>
#endif

#ifndef NO_PTY
#  include <termios.h>
#  include <sys/ioctl.h>
#  include <pwd.h>
#endif

#define PRGNAME "avatarsay"
#define HOMEPAGE "http://akfoerster.de/akfavatar/"
#define BUGMAIL "bug-akfavatar@akfoerster.de"

/* terminal type */
/* 
 * this is not dependent on the system on which it runs,
 * but the terminal database should have an entry for this
 */
#define TERM "linux"
#define BWTERM "linux-m"

/* size for input buffer - not too small, please */
/* .encoding must be in first buffer */
#define INBUFSIZE 10240

/* maximum size for path */
/* should fit into stack */
#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

/* is there really no clean way to find the executable? */
#ifndef PREFIX
#  define PREFIX "/usr/local/bin"
#endif

/* device attribute (DEC) */
#define DS "\033[?1;2c"		/* claim to be a vt100 with advanced video */

#define BYTE_ORDER_MARK L'\xfeff'


static const char *version_info_en =
  PRGNAME " (AKFAvatar) " AVTVERSION "\n"
  "Copyright (c) 2007, 2008 Andreas K. Foerster\n\n"
  "License GPLv3+: GNU GPL version 3 or later "
  "<http://gnu.org/licenses/gpl.html>\n\n"
  "This is free software: you are free to change and redistribute it.\n"
  "There is NO WARRANTY, to the extent permitted by law.\n"
  "Please read the manual for instructions.";

/* avoid german umlauts here */
static const char *version_info_de =
  PRGNAME " (AKFAvatar) " AVTVERSION "\n"
  "Copyright (c) 2007, 2008 Andreas K. Foerster\n\n"
  "Lizenz GPLv3+: GNU GPL Version 3 oder neuer "
  "<http://gnu.org/licenses/gpl.html>\n\n"
  "Dies ist Freie Software: Sie duerfen es gemaess der GPL weitergeben und\n"
  "bearbeiten. Fuer AKFAvatar besteht KEINERLEI GARANTIE.\n"
  "Bitte lesen Sie auch die Anleitung.";

/* pointer to program name in argv[0] */
static const char *program_name;

/* default encoding - either system encoding or given per parameters */
/* supported in SDL: ASCII, ISO-8859-1, UTF-8, UTF-16, UTF-32 */
static char default_encoding[80];

/* if rawmode is set, then don't interpret any commands or comments */
/* rawmode can be activated with the options -r or --raw */
static avt_bool_t rawmode;

/* popup-mode? */
static avt_bool_t popup;

/* default-text delay */
static int default_delay;

/* 
 * ignore end of file conditions
 * this should be used, when the input doesn't come from a file
 */
static avt_bool_t ignore_eof;

/* start the terminal mode? */
static avt_bool_t terminal_mode;

/* execute file? Option -e */
static avt_bool_t executable;
static avt_bool_t read_error_is_eof;
static int prg_input;		/* file descriptor for program input */

/* in idle loop? */
static avt_bool_t idle;

/* whether to run in a window, or in fullscreen mode */
/* the mode can be set by -f, --fullscreen or -w, --window */
/* or with --fullfullscreen or -F */
static int window_mode;

/* is the avatar initialized? (0|1) */
/* for delaying the initialization until it is clear that we actually 
   have data to show */
static avt_bool_t initialized;

/* was the file already checked for an encoding? */
static avt_bool_t encoding_checked;

/* encoding given on command line */
static avt_bool_t given_encoding;

/* play it in an endless loop? */
/* deactivated, when input comes from stdin */
/* can be deactivated by -1, --once */
static avt_bool_t loop;

/* where to find imagefiles */
static char datadir[512];

/* for loading an avt_image */
static avt_image_t *avt_image;

/* name of the image file */
static char *avt_image_name;

/* for loading sound files */
static avt_audio_t *sound;

/* text-buffer */
static int wcbuf_pos = 0;
static int wcbuf_len = 0;

/* maximum coordinates (set by "initialize") */
static int max_x, max_y;
static int region_min_y, region_max_y;

/* insert mode (for terminal) */
static avt_bool_t insert_mode;

/* colors for terminal mode */
static int text_color;
static int text_background_color;
static avt_bool_t faint;

/* no color (but still bold, underlined, reversed) allowed */
static avt_bool_t nocolor;

/* character for DEC cursor keys (either [ or O) */
static char dec_cursor_seq[3];

/* language (of current locale) */
enum language_t
{ ENGLISH, DEUTSCH };
static enum language_t language;


static void
quit (int exitcode)
{
  if (initialized)
    {
      if (sound)
	avt_free_audio (sound);

      avt_quit ();
    }

  exit (exitcode);
}


#ifndef __WIN32__

/* 
 * "warning_msg", "notice_msg" and "error_msg" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

static void
warning_msg (const char *msg1, const char *msg2)
{
  if (msg2)
    fprintf (stderr, PRGNAME ": %s: %s\n", msg1, msg2);
  else
    fprintf (stderr, PRGNAME ": %s\n", msg1);
}

static void
notice_msg (const char *msg1, const char *msg2)
{
  warning_msg (msg1, msg2);
}

static void
error_msg (const char *msg1, const char *msg2)
{
  warning_msg (msg1, msg2);
  quit (EXIT_FAILURE);
}

static void
showversion (void)
{
  switch (language)
    {
    case DEUTSCH:
      puts (version_info_de);
      break;

    case ENGLISH:
    default:
      puts (version_info_en);
    }

  exit (EXIT_SUCCESS);
}

static void
help (void)
{
  printf ("\nUsage: %s [Options]\n", program_name);
  printf ("  or:  %s [Options] textfiles\n", program_name);
  printf ("  or:  %s [Options] --execute program [program options]\n\n",
	  program_name);
  puts
    ("A fancy text-terminal, text-viewer and scripting language for making demos.\n");
  puts ("If textfile is - then read from stdin and don't loop.\n");
  puts ("Options:");
  puts (" -h, --help              show this help");
  puts (" -v, --version           show the version");
#ifdef NO_PTY
  puts (" -t, --terminal          not supported on this system");
  puts (" -x, -e, --execute       not supported on this system");
#else
  puts (" -t, --terminal          terminal mode (run a shell in balloon)");
  puts (" -x, -e, --execute       execute program in balloon");
#endif
  puts (" -b, --nocolor           no color allowed (black and white)");
  puts (" -w, --window            try to run the program in a window"
	" (default)");
  puts (" -f, --fullscreen        try to run the program in fullscreen mode");
  puts (" -F, --fullfullscreen    like -f, but use current display-size");
  puts (" -E, --encoding=enc      input data is encoded in encoding \"enc\"");
  puts (" -l, --latin1            input data is encoded in Latin-1");
  puts (" -u, --utf-8             input data is encoded in UTF-8");
  puts (" -1, --once              run only once (don't loop)");
  puts (" -p, --popup             popup, ie. don't move the avatar in");
  puts (" -r, --raw               output raw text"
	" (don't handle any commands)");
  puts (" -n, --no-delay          don't delay output of text (textfiles)");
  puts (" -i, --ignoreeof         ignore end of file conditions "
	"(input is not a file)");
  puts ("\nEnvironment variables:");
  puts (" AVATARIMAGE             different image as avatar");
  puts (" AVATARDATADIR           data-directory");
#ifndef NO_PTY
  puts (" HOME                    home directory (terminal)");
  puts (" SHELL                   preferred shell (terminal)");
#endif
  puts ("\nHomepage:");
  puts ("  " HOMEPAGE);
  puts ("\nReport bugs to <" BUGMAIL ">");
  exit (EXIT_SUCCESS);
}

#else /* not Windows or ReactOS */

static void
warning_msg (const char *msg1, const char *msg2)
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
notice_msg (const char *msg1, const char *msg2)
{
}

static void
error_msg (const char *msg1, const char *msg2)
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
  switch (language)
    {
    case DEUTSCH:
      MessageBox (NULL, version_info_de, PRGNAME,
		  MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
      break;

    case ENGLISH:
    default:
      MessageBox (NULL, version_info_en, PRGNAME,
		  MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
    }

  exit (EXIT_SUCCESS);
}

/* For Windows users it's not useful to show the options */
/* But the text refers to the manual */
static void
help (void)
{
  showversion ();
  exit (EXIT_SUCCESS);
}

static void
not_available (void)
{
  avt_clear ();
  avt_set_text_delay (default_delay);
  avt_bell ();

  switch (language)
    {
    case DEUTSCH:
      avt_say (L"Funktion auf diesem System nicht verfügbar...");
      break;
    case ENGLISH:
    default:
      avt_say (L"function not available on this system...");
    }

  if (avt_wait_button () != 0)
    quit (EXIT_SUCCESS);
}

#endif /* not Windows or ReactOS */

static void
set_encoding (const char *encoding)
{
  if (avt_mb_encoding (encoding))
    error_msg ("iconv", avt_get_error ());
}

static void
move_in (void)
{
  if (initialized)
    {
      if (avt_move_in ())
	quit (EXIT_SUCCESS);
      if (avt_wait (2000))
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
	if (avt_wait (5000))
	  quit (EXIT_SUCCESS);
    }
}

static void
initialize (void)
{
  if (!avt_image)
    avt_image = avt_default ();

  if (avt_initialize ("AKFAvatar", "AKFAvatar", avt_image, window_mode))
    switch (language)
      {
      case DEUTSCH:
	error_msg ("kann Grafik nicht initialisieren", avt_get_error ());
	break;

      case ENGLISH:
      default:
	error_msg ("cannot initialize graphics", avt_get_error ());
      }

  if (avt_initialize_audio ())
    switch (language)
      {
      case DEUTSCH:
	error_msg ("kann Audio nicht initialisieren", avt_get_error ());
	break;

      case ENGLISH:
      default:
	error_msg ("cannot initialize audio", avt_get_error ());
      }

  avt_set_text_delay (default_delay);
  max_x = avt_get_max_x ();
  max_y = avt_get_max_y ();
  region_min_y = 1;
  region_max_y = max_y;
  initialized = AVT_TRUE;
}

static void
checkoptions (int argc, char **argv)
{
  int c;
  int option_index = 0;

#ifdef __WIN32__
  /* stderr doesn't work in windows GUI programs */
  opterr = 0;
#endif

  while (1)
    {
      static struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'v'},
	{"fullscreen", no_argument, 0, 'f'},
	{"fulfullscreen", no_argument, 0, 'F'},
	{"window", no_argument, 0, 'w'},
	{"once", no_argument, 0, '1'},
	{"raw", no_argument, 0, 'r'},
	{"ignoreeof", no_argument, 0, 'i'},
	{"saypipe", no_argument, 0, 's'},
	{"encoding", required_argument, 0, 'E'},
	{"latin1", no_argument, 0, 'l'},
	{"utf-8", no_argument, 0, 'u'},
	{"utf8", no_argument, 0, 'u'},
	{"u8", no_argument, 0, 'u'},
	{"popup", no_argument, 0, 'p'},
	{"terminal", no_argument, 0, 't'},
	{"execute", no_argument, 0, 'x'},
	{"no-delay", no_argument, 0, 'n'},
	{"nocolor", no_argument, 0, 'b'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "+hvfFw1risE:luptxenb",
		       long_options, &option_index);

      /* end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	  /* long-option has set a flag, nothing to do here */
	case 0:
	  break;

	case 'h':		/* --help */
	  help ();
	  break;

	case 'v':		/* --version */
	  showversion ();
	  break;

	case 'f':		/* --fullscreen */
	  window_mode = AVT_FULLSCREEN;
	  break;

	case 'F':		/* --fullfullscreen */
	  window_mode = AVT_FULLSCREENNOSWITCH;
	  break;

	case 'w':		/* --window */
	  window_mode = AVT_WINDOW;
	  break;

	case '1':		/* --once */
	  loop = AVT_FALSE;
	  break;

	case 'r':		/* --raw */
	  rawmode = AVT_TRUE;
	  break;

	case 'i':		/* --ignoreeof */
	  ignore_eof = AVT_TRUE;
	  break;

	case 'E':		/* --encoding */
	  strncpy (default_encoding, optarg, sizeof (default_encoding));
	  given_encoding = AVT_TRUE;
	  break;

	case 'l':		/* --latin1 */
	  strcpy (default_encoding, "ISO-8859-1");
	  given_encoding = AVT_TRUE;
	  break;

	case 'u':		/* --utf-8, --utf8, --u8 */
	  strcpy (default_encoding, "UTF-8");
	  given_encoding = AVT_TRUE;
	  break;

	case 'p':		/* --popup */
	  popup = AVT_TRUE;
	  break;

	case 't':		/* --terminal */
	  default_delay = 0;
	  terminal_mode = AVT_TRUE;
	  loop = AVT_FALSE;
	  break;

	case 'x':		/* --execute */
	case 'e':
	  executable = AVT_TRUE;
	  default_delay = 0;
	  loop = AVT_FALSE;
	  break;

	case 'n':		/* --no-delay */
	  default_delay = 0;
	  avt_set_text_delay (0);
	  break;

	case 'b':		/* --nocolor */
	  nocolor = AVT_TRUE;
	  break;

	case '?':		/* unsupported option */
	  /* getopt_long already printed an error message to stderr */
	  help ();
	  break;

	  /* declared option, but not handled here */
	  /* should never happen */
	default:
	  switch (language)
	    {
	    case DEUTSCH:
	      error_msg ("interner Fehler", "Option wird nicht unterstuetzt");
	      break;

	    case ENGLISH:
	    default:
	      error_msg ("internal error", "option not supported");
	    }			/* switch (language) */
	}			/* switch (c) */
    }				/* while (1) */

  if (terminal_mode && argc > optind)
    error_msg ("error", "no files allowed for terminal mode");

  if (executable && argc <= optind)
    error_msg ("error", "execute needs at least a program name");

  if (argc > optind
      && (strcmp (argv[optind], "moo") == 0
	  || strcmp (argv[optind], "muh") == 0))
    {
      initialize ();
      avt_set_text_delay (0);
      avt_viewport (26, 3, max_x, max_y);
      avt_say_mb (" ___________\n< AKFAvatar >\n -----------\n    "
		  "    \\   ^__^\n         \\  (oo)\\_____"
		  "__\n            (__)\\       )\\/\\\n     "
		  "           ||----w |\n                ||     ||");
      avt_wait_button ();
      quit (EXIT_SUCCESS);
    }

  /* 
   * when input file is - the program must not loop
   * the - can not be combined with filenames
   */
  if ((argc == optind + 1) && (strcmp (argv[optind], "-") == 0))
    loop = 0;
  else
    {
      int i;
      for (i = optind; i < argc; i++)
	if (strcmp (argv[i], "-") == 0)
	  error_msg ("error", "filenames and \"-\" can not be combined");
    }
}

static void
use_avatar_image (char *image_file)
{
  /* clean up old definitions */
  if (avt_image)
    free (avt_image);

  if (avt_image_name)
    free (avt_image_name);

  /* save the name */
  avt_image_name = strdup (image_file);

  avt_image = avt_import_image_file (image_file);
  if (avt_image == NULL)
    switch (language)
      {
      case DEUTSCH:
	error_msg ("Fehler beim Laden des AVATARIMAGE Bildes",
		   avt_get_error ());
	break;
      case ENGLISH:
      default:
	error_msg ("error while loading the AVATARIMAGE", avt_get_error ());
      }
}

static void
checkenvironment (void)
{
  char *e;

  e = getenv ("AVATARDATADIR");
  if (e)
    strncpy (datadir, e, sizeof (datadir));

  e = getenv ("AVATARIMAGE");
  if (e)
    use_avatar_image (e);
}

/* fills filepath with datadir and the converted content of fn */
static void
get_data_file (const wchar_t * fn, char filepath[])
{
  size_t result, filepath_len;

  strcpy (filepath, datadir);

  if (filepath[0] != '\0')
    strcat (filepath, "/");

  filepath_len = strlen (filepath);

  /* remove leading whitespace */
  while (*fn == L' ' || *fn == L'\t')
    fn++;

  result =
    wcstombs (&filepath[filepath_len], fn, PATH_MAX - filepath_len - 1);

  if (result == (size_t) (-1))
    error_msg ("wcstombs", strerror (errno));
}

static void
handle_image_command (const wchar_t * s)
{
  char filepath[PATH_MAX];

  get_data_file (s + 7, filepath);	/* remove ".image " */

  if (!initialized)
    initialize ();
  else if (avt_wait (2500))
    quit (EXIT_SUCCESS);
  if (!avt_show_image_file (filepath))
    if (avt_wait (7000))
      quit (EXIT_SUCCESS);
}

static void
handle_avatarimage_command (const wchar_t * s)
{
  char filepath[PATH_MAX];

  /* if already assigned, delete it */
  if (avt_image)
    avt_free_image (avt_image);

  get_data_file (s + 13, filepath);	/* remove ".avatarimage " */

  if (!(avt_image = avt_import_image_file (filepath)))
    warning_msg ("warning", avt_get_error ());
}

static void
handle_backgoundcolor_command (const wchar_t * s)
{
  unsigned int red, green, blue;

  if (swscanf (s, L".backgroundcolor #%2x%2x%2x", &red, &green, &blue) == 3)
    avt_set_background_color (red, green, blue);
  else
    error_msg (".backgroundcolor", NULL);
}

static void
handle_audio_command (const wchar_t * s)
{
  char filepath[PATH_MAX];

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (sound)
    {
      avt_free_audio (sound);
      sound = NULL;
    }

  get_data_file (s + 7, filepath);

  sound = avt_load_wave_file (filepath);
  if (sound == NULL)
    {
      notice_msg ("can not load audio file", avt_get_error ());
      return;
    }

  if (avt_play_audio (sound, AVT_FALSE))
    notice_msg ("can not play audio file", avt_get_error ());
}

static void
handle_back_command (const wchar_t * s)
{
  int i, value;


  if (!initialized)
    return;

  if (swscanf (s, L".back %d", &value) > 0)
    {
      for (i = 0; i < value; i++)
	avt_backspace ();
    }
  else
    avt_backspace ();
}

static void
handle_read_command (void)
{
  wchar_t line[AVT_LINELENGTH];

  if (!initialized)
    initialize ();

  if (avt_ask (line, sizeof (line)))
    quit (EXIT_SUCCESS);

  /* TODO: not fully implemented yet */
}

/* removes trailing space, newline, etc. */
static void
strip (wchar_t ** s)
{
  wchar_t *p;

  p = *s;

  /* search end of string */
  while (*(p + 1) != L'\0')
    p++;

  /* search for last non-space char */
  while (*p == L'\n' || *p == L'\r' || *p == L' ' || *p == L'\t')
    p--;

  *(p + 1) = L'\0';
}


/* handle commads, including comments */
static avt_bool_t
iscommand (wchar_t * s, int *stop)
{
  if (rawmode)
    return AVT_FALSE;

  /* 
   * a stripline begins with at least 3 dashes
   * or it begins with \f 
   * the rest of the line is ignored
   */
  if (wcsncmp (s, L"---", 3) == 0 || s[0] == L'\f')
    {
      if (initialized)
	if (avt_flip_page ())
	  *stop = AVT_TRUE;
      return AVT_TRUE;
    }

  if (s[0] == L'.')
    {
      strip (&s);

      /* new datadir */
      if (wcsncmp (s, L".datadir ", 9) == 0)
	{
	  if (wcstombs ((char *) &datadir, s + 9, sizeof (datadir))
	      == (size_t) (-1))
	    warning_msg (".datadir", strerror (errno));
	  return AVT_TRUE;
	}

      if (wcsncmp (s, L".avatarimage ", 13) == 0)
	{
	  if (!initialized)
	    handle_avatarimage_command (s);

	  return AVT_TRUE;
	}

      /* the encoding is checked in check_encoding */
      /* so ignore it here */
      if (wcsncmp (s, L".encoding ", 10) == 0)
	return AVT_TRUE;

      if (wcsncmp (s, L".backgroundcolor ", 17) == 0)
	{
	  handle_backgoundcolor_command (s);
	  return AVT_TRUE;
	}

      /* default - for most languages */
      if (wcscmp (s, L".left-to-right") == 0)
	{
	  avt_text_direction (AVT_LEFT_TO_RIGHT);
	  return AVT_TRUE;
	}

      /* currently only hebrew/yiddish supported */
      if (wcscmp (s, L".right-to-left") == 0)
	{
	  avt_text_direction (AVT_RIGHT_TO_LEFT);
	  return AVT_TRUE;
	}

      /* new page - same as \f or stripline */
      if (wcscmp (s, L".flip") == 0)
	{
	  if (initialized)
	    if (avt_flip_page ())
	      *stop = AVT_TRUE;
	  return AVT_TRUE;
	}

      /* clear ballon - don't wait */
      if (wcscmp (s, L".clear") == 0)
	{
	  if (initialized)
	    avt_clear ();
	  return AVT_TRUE;
	}

      /* longer intermezzo */
      if (wcscmp (s, L".pause") == 0)
	{
	  if (!initialized)
	    initialize ();
	  else if (avt_wait (2700))
	    *stop = AVT_TRUE;

	  avt_show_avatar ();
	  if (avt_wait (4000))
	    *stop = AVT_TRUE;
	  return AVT_TRUE;
	}

      /* show image */
      if (wcsncmp (s, L".image ", 7) == 0)
	{
	  handle_image_command (s);
	  return AVT_TRUE;
	}

      /* play sound */
      if (wcsncmp (s, L".audio ", 7) == 0)
	{
	  handle_audio_command (s);
	  return AVT_TRUE;
	}

      /* wait until sound ends */
      if (wcscmp (s, L".waitaudio") == 0)
	{
	  if (initialized)
	    if (avt_wait_audio_end ())
	      *stop = AVT_TRUE;
	  return AVT_TRUE;
	}

      /* 
       * pause for effect in a sentence
       * the previous line should end with a backslash
       */
      if (wcscmp (s, L".effectpause") == 0)
	{
	  if (initialized)
	    if (avt_wait (2500))
	      *stop = AVT_TRUE;
	  return AVT_TRUE;
	}

      /* 
       * move back a number of characters
       * the previous line has to end with a backslash!
       */
      if (wcsncmp (s, L".back ", 6) == 0)
	{
	  handle_back_command (s);
	  return AVT_TRUE;
	}

      if (wcscmp (s, L".read") == 0)
	{
	  handle_read_command ();
	  return AVT_TRUE;
	}

      if (wcscmp (s, L".end") == 0)
	{
	  if (initialized)
	    avt_move_out ();
	  *stop = AVT_TRUE;
	  return AVT_TRUE;
	}

      if (wcscmp (s, L".stop") == 0)
	{
	  /* doesn't matter whether it's initialized */
	  *stop = AVT_TRUE;
	  return AVT_TRUE;
	}

      /* silently ignore unknown commands */
      return AVT_TRUE;
    }

  /* ignore lines starting with a '#' */
  if (s[0] == L'#')
    return AVT_TRUE;

  return AVT_FALSE;
}

/* 
 * check for the command .encoding 
 * or a byte order mark (BOM) U+FEFF 
 */
static void
check_encoding (const char *buf)
{
  encoding_checked = AVT_TRUE;

  {
    char *enc;
    char temp[80];

    /* buf is \0 terminated */
    /* check for command .encoding */
    enc = strstr (buf, ".encoding ");

    /*
     * if .encoding is found and it is either at the start of the buffer 
     * or the previous character is a \n then set_encoding
     * and don't check anything else anymore
     */
    if (enc != NULL && (enc == buf || *(enc - 1) == '\n'))
      {
	if (sscanf (enc, ".encoding %79s", (char *) &temp) <= 0)
	  warning_msg (".encoding", NULL);
	else
	  {
	    set_encoding (temp);
	    return;
	  }
      }
  }

  /* check for byte order marks (BOM) */

  /* UTF-8 BOM (as set by Windows notepad) */
  if (*buf == '\xEF' && *(buf + 1) == '\xBB' && *(buf + 2) == '\xBF')
    {
      set_encoding ("UTF-8");
      return;
    }

  /* check 32 Bit BOMs before 16 Bit ones, to avoid confusion! */

  /* UTF-32BE BOM */
  if (*buf == '\x00' && *(buf + 1) == '\x00'
      && *(buf + 2) == '\xFE' && *(buf + 3) == '\xFF')
    {
      set_encoding ("UTF-32BE");
      return;
    }

  /* UTF-32LE BOM */
  if (*buf == '\xFF' && *(buf + 1) == '\xFE'
      && *(buf + 2) == '\x00' && *(buf + 3) == '\x00')
    {
      set_encoding ("UTF-32LE");
      return;
    }

  /* UTF-16BE BOM */
  if (*buf == '\xFE' && *(buf + 1) == '\xFF')
    {
      set_encoding ("UTF-16BE");
      return;
    }

  /* UTF-16LE BOM */
  if (*buf == '\xFF' && *(buf + 1) == '\xFE')
    {
      set_encoding ("UTF-16LE");
      return;
    }

  /* other heuristics for Unicode */

  if (*buf == '\x00' && *(buf + 1) == '\x00')
    {
      set_encoding ("UTF-32BE");
      return;
    }

  if (*(buf + 2) == '\x00' && *(buf + 3) == '\x00')
    {
      set_encoding ("UTF-32LE");
      return;
    }

  if (*buf == '\x00')
    {
      set_encoding ("UTF-16BE");
      return;
    }

  if (*(buf + 1) == '\x00')
    {
      set_encoding ("UTF-16LE");
      return;
    }
}

/* if line ends on \, strip and \n */
void
process_line_end (wchar_t * s, ssize_t * len)
{
  /* in rawmode don't change anything */
  if (rawmode)
    return;

  if (*len < 3)
    return;

  /* strip \\\n at the end */
  if (*(s + *len - 1) == L'\n' && *(s + *len - 2) == L'\\')
    {
      *len -= 2;
      *(s + *len + 1) = L'\0';
    }

  /* strip \\\r\n at the end */
  if (*(s + *len - 1) == L'\n' && *(s + *len - 2) == L'\r'
      && *(s + *len - 3) == L'\\')
    {
      *len -= 3;
      *(s + *len + 1) = L'\0';
    }
}

/* @@@ */
static wint_t
get_character (int fd)
{
  static wchar_t *wcbuf = NULL;
  wchar_t ch;

  if (wcbuf_pos >= wcbuf_len)
    {
      static char filebuf[INBUFSIZE];
      static int filebuf_end = 0;

      if (wcbuf)
	{
	  avt_free (wcbuf);
	  wcbuf = NULL;
	}

      /* reserve one byte for a terminator */
      filebuf_end = read (fd, &filebuf, sizeof (filebuf) - 1);

      /* waiting for data */
      if (filebuf_end == -1 && errno == EAGAIN)
	{
	  idle = AVT_TRUE;
	  while (filebuf_end == -1 && errno == EAGAIN
		 && avt_update () == AVT_NORMAL)
	    filebuf_end = read (fd, &filebuf, sizeof (filebuf) - 1);
	  idle = AVT_FALSE;
	}

      if (filebuf_end == -1)
	{
	  if (read_error_is_eof)
	    wcbuf_len = -1;
	  else
	    error_msg ("error while reading from file", strerror (errno));
	}
      else			/* filebuf_end != -1 */
	{
	  if (!encoding_checked && !given_encoding)
	    {
	      filebuf[filebuf_end] = '\0';	/* terminate */
	      check_encoding (filebuf);
	    }

	  wcbuf_len = avt_mb_decode (&wcbuf, (char *) &filebuf, filebuf_end);
	  wcbuf_pos = 0;
	}
    }

  if (wcbuf_len < 0)
    ch = WEOF;
  else
    {
      ch = *(wcbuf + wcbuf_pos);
      wcbuf_pos++;
    }

  return ch;
}

static ssize_t
getwline (int fd, wchar_t * lineptr, size_t n)
{
  ssize_t nchars, maxchars;
  wint_t ch;

  /* one char reserved for terminator */
  maxchars = (n / sizeof (wchar_t)) - 1;
  nchars = 0;

  /* get first character and skip byte order marks */
  do
    {
      ch = get_character (fd);
    }
  while (ch == BYTE_ORDER_MARK);

  *lineptr = ch;
  nchars++;

  while (ch != WEOF && ch != L'\n' && nchars <= maxchars)
    {
      ch = get_character (fd);
      lineptr++;
      *lineptr = ch;
      nchars++;
    }

  /* terminate */
  if (ch != WEOF)
    *(lineptr + 1) = L'\0';
  else
    {
      *lineptr = L'\0';
      nchars--;
    }

  return nchars;
}

#ifndef NO_PTY

static char *
get_user_shell (void)
{
  char *shell;

  shell = getenv ("SHELL");

  /* when the variable is not set, dig deeper */
  if (shell == NULL || *shell == '\0')
    {
      struct passwd *user_data;

      user_data = getpwuid (getuid ());
      if (user_data != NULL && user_data->pw_shell != NULL
	  && *user_data->pw_shell != '\0')
	shell = user_data->pw_shell;
      else
	shell = "/bin/sh";	/* default shell */
    }

  return shell;
}

/* get user's home direcory */
static char *
get_user_home (void)
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

  return home;
}

void
prg_keyhandler (int sym, int mod AVT_UNUSED, int unicode)
{
  if (idle && prg_input > 0)
    {
      idle = AVT_FALSE;		/* avoid reentrance */

      switch (sym)
	{
	case 273:		/* up arrow */
	  dec_cursor_seq[2] = 'A';
	  write (prg_input, dec_cursor_seq, 3);
	  break;

	case 274:		/* down arrow */
	  dec_cursor_seq[2] = 'B';
	  write (prg_input, dec_cursor_seq, 3);
	  break;

	case 275:		/* right arrow */
	  dec_cursor_seq[2] = 'C';
	  write (prg_input, dec_cursor_seq, 3);
	  break;

	case 276:		/* left arrow */
	  dec_cursor_seq[2] = 'D';
	  write (prg_input, dec_cursor_seq, 3);
	  break;

	case 277:		/* Insert */
	  /* write (prg_input, "\033[L", 3); */
	  write (prg_input, "\033[2~", 4);	/* linux */
	  break;

	case 278:		/* Home */
	  /* write (prg_input, "\033[H", 3); */
	  write (prg_input, "\033[1~", 4);	/* linux */
	  break;

	case 279:		/* End */
	  /* write (prg_input, "\033[0w", 4); */
	  write (prg_input, "\033[4~", 4);	/* linux */
	  break;

	case 280:		/* Page up */
	  write (prg_input, "\033[5~", 4);	/* linux */
	  break;

	case 281:		/* Page down */
	  write (prg_input, "\033[6~", 4);	/* linux */
	  break;

	case 282:		/* F1 */
	  write (prg_input, "\033[[A", 4);	/* linux */
	  /* write (prg_input, "\033OP", 3); *//* DEC */
	  break;

	case 283:		/* F2 */
	  write (prg_input, "\033[[B", 4);	/* linux */
	  /* write (prg_input, "\033OQ", 3); *//* DEC */
	  break;

	case 284:		/* F3 */
	  write (prg_input, "\033[[C", 4);	/* linux */
	  /* write (prg_input, "\033OR", 3); *//* DEC */
	  break;

	case 285:		/* F4 */
	  write (prg_input, "\033[[D", 4);	/* linux */
	  /* write (prg_input, "\033OS", 3); *//* DEC */
	  break;

	case 286:		/* F5 */
	  write (prg_input, "\033[[E", 4);	/* linux */
	  /* write (prg_input, "\033Ot", 3); *//* DEC */
	  break;

	case 287:		/* F6 */
	  write (prg_input, "\033[17~", 5);	/* linux */
	  /* write (prg_input, "\033Ou", 3); *//* DEC */
	  break;

	case 288:		/* F7 */
	  write (prg_input, "\033[[18~", 5);	/* linux */
	  /* write (prg_input, "\033Ov", 3); *//* DEC */
	  break;

	case 289:		/* F8 */
	  write (prg_input, "\033[19~", 5);	/* linux */
	  /* write (prg_input, "\033Ol", 3); *//* DEC */
	  break;

	case 290:		/* F9 */
	  write (prg_input, "\033[20~", 5);	/* linux */
	  /* write (prg_input, "\033Ow", 3); *//* DEC */
	  break;

	case 291:		/* F10 */
	  write (prg_input, "\033[21~", 5);	/* linux */
	  /* write (prg_input, "\033Ox", 3); *//* DEC */
	  break;

	case 292:		/* F11 */
	  write (prg_input, "\033[23~", 5);	/* linux */
	  break;

	case 293:		/* F12 */
	  write (prg_input, "\033[24~", 5);	/* linux */
	  break;

	case 294:		/* F13 */
	  write (prg_input, "\033[25~", 5);	/* linux */
	  break;

	case 295:		/* F14 */
	  write (prg_input, "\033[26~", 5);	/* linux */
	  break;

	case 296:		/* F15 */
	  write (prg_input, "\033[27~", 5);	/* linux */
	  break;

	default:
	  if (unicode)
	    {
	      wchar_t ch;
	      char *mbstring;
	      int length;

	      ch = (wchar_t) unicode;
	      length = avt_mb_encode (&mbstring, &ch, 1);
	      if (length != -1)
		{
		  write (prg_input, mbstring, length);
		  avt_free (mbstring);
		}
	    }			/* if (unicode) */
	}			/* switch */

      idle = AVT_TRUE;
    }				/* if (idle...) */
}

/* TODO: doesn't work yet */
void
prg_mousehandler (int button, avt_bool_t pressed, int x, int y)
{
  char code[7];

  /* X10 method */
  if (pressed)
    {
      snprintf (code, sizeof (code), "\033[M%c%c%c",
		(char) (040 + button), (char) (040 + x), (char) (040 + y));
      write (prg_input, &code, sizeof (code) - 1);
    }
}

#endif /* not NO_PTY */

#ifdef __WIN32__

/* get user's home direcory */
static char *
get_user_home (void)
{
  char *home;

  home = getenv ("HOME");
  if (home == NULL)
    home = getenv ("HOMEPATH");
  if (home == NULL)
    home = "C:\\";

  return home;
}

#endif

/* opens the file, returns file descriptor or -1 on error */
static int
openfile (const char *fname)
{
  int fd = -1;

  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;

  if (strcmp (fname, "-") == 0)
    fd = STDIN_FILENO;		/* stdin */
  else				/* regular file */
#ifdef O_NONBLOCK
    fd = open (fname, O_RDONLY | O_NONBLOCK);
#else
    fd = open (fname, O_RDONLY);
#endif

  read_error_is_eof = AVT_FALSE;

  return fd;
}

/* shows content of file / other input */
/* returns -1:file cannot be processed, 0:normal, 1:stop requested */
static int
process_file (int fd)
{
  wchar_t *line = NULL;
  size_t line_size = 0;
  ssize_t nread = 0;
  avt_bool_t stop = AVT_FALSE;

  line_size = 1024 * sizeof (wchar_t);
  line = (wchar_t *) malloc (line_size);

  encoding_checked = AVT_FALSE;

  /* get first line */
  nread = getwline (fd, line, line_size);

  /* if first line is "@echo off", skip to exit */
  if (!rawmode && wcsncmp (line, L"@echo off", 9) == 0)
    {
      while (nread != 0 && wcsncmp (line, L"exit", 4) != 0)
	nread = getwline (fd, line, line_size);

      /* get next line */
      if (nread != 0)
	nread = getwline (fd, line, line_size);
    }

  /* 
   * skip empty lines and handle commands at the beginning 
   * before initializing the graphics
   */
  if (!rawmode)
    while (nread != 0 && !stop
	   && (wcscmp (line, L"\n") == 0
	       || wcscmp (line, L"\r\n") == 0 || iscommand (line, &stop)))
      {
	nread = getwline (fd, line, line_size);

	/* simulate empty lines when ignore_eof is set */
	if (ignore_eof && nread == 0)
	  {
	    wcscpy (line, L"\n");
	    nread = 1;
	    if (avt_update ())
	      stop = AVT_TRUE;
	  }
      }

  /* initialize the graphics */
  if (!initialized && !stop)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  /* show text */
  if (line && !stop)
    {
      process_line_end (line, &nread);
      stop = avt_say_len (line, nread);
    }

  while (!stop && (nread != 0 || ignore_eof))
    {
      nread = getwline (fd, line, line_size);

      if (ignore_eof)
	{
	  /* wait for input */
	  while (nread == 0)
	    {
	      if (avt_update ())
		{
		  stop = AVT_TRUE;
		  break;
		}
	      else
		nread = getwline (fd, line, line_size);
	    }
	}

      if (nread != 0 && !iscommand (line, &stop) && !stop)
	{
	  process_line_end (line, &nread);
	  if (avt_say_len (line, nread))
	    stop = AVT_TRUE;
	}
    }

  if (line)
    {
      free (line);
      line_size = 0;
    }

  if (close (fd) == -1 && errno != EAGAIN)
    warning_msg ("close", strerror (errno));

  if (avt_get_status () == AVT_ERROR)
    {
      stop = AVT_TRUE;
      warning_msg ("AKFAvatar", avt_get_error ());
    }

  avt_text_direction (AVT_LEFT_TO_RIGHT);
  return (int) stop;
}

extern int get_file (char *filename);

static void
ask_file (void)
{
  char filename[256];

  chdir (datadir);
  get_file (filename);

  /* ignore quit-requests */
  /* (used to get out of the file dialog) */
  if (avt_get_status () == AVT_QUIT)
    avt_set_status (AVT_NORMAL);

  if (filename[0] != '\0')
    {
      int fd, status;

      avt_set_text_delay (default_delay);

      fd = openfile (filename);
      if (fd > -1)
	process_file (fd);

      /* ignore file errors */
      status = avt_get_status ();
      if (status == AVT_ERROR)
	quit (EXIT_FAILURE);	/* warning already printed */

      if (status == AVT_NORMAL)
	if (avt_wait_button ())
	  quit (EXIT_SUCCESS);

      /* reset quit-request and encoding */
      avt_set_status (AVT_NORMAL);
      set_encoding (default_encoding);
    }
}

#ifdef NO_PTY

static void
run_shell (void)
{
  not_available ();
}

static void
run_info (void)
{
  not_available ();
}

static int
execute_process (char *const prg_argv[])
{
  not_available ();
  return -1;
}

#else /* not NO_PTY */

/* 
 * returns 2 values of a string like "1;2"
 */
static void
get_2_values (const char *sequence, int *n1, int *n2)
{
  char *tail;

  *n1 = strtol (sequence, &tail, 10);
  *n2 = 0;
  if (*tail == ';')
    *n2 = strtol (tail + 1, &tail, 10);
}

static void
set_foreground_color (int color)
{
  if (color != 0)
    faint = AVT_FALSE;

  switch (color)
    {
    case 0:
      if (!faint)
	avt_set_text_color (0x00, 0x00, 0x00);
      else
	avt_set_text_color (0x55, 0x55, 0x55);
      break;
    case 1:			/* red */
      avt_set_text_color (0x88, 0x00, 0x00);
      break;
    case 2:			/* green */
      avt_set_text_color (0x00, 0x88, 0x00);
      break;
    case 3:			/* brown */
      avt_set_text_color (0x88, 0x44, 0x22);
      break;
    case 4:			/* blue */
      avt_set_text_color (0x00, 0x00, 0x88);
      break;
    case 5:			/* magenta */
      avt_set_text_color (0x88, 0x00, 0x88);
      break;
    case 6:			/* cyan */
      avt_set_text_color (0x00, 0x88, 0x88);
      break;
    case 7:			/* lightgray */
      avt_set_text_color (0x88, 0x88, 0x88);
      break;
    case 8:			/* darkgray */
      avt_set_text_color (0x55, 0x55, 0x55);
      break;
    case 9:			/* lightred */
      avt_set_text_color (0xFF, 0x00, 0x00);
      break;
    case 10:			/* lightgreen */
      avt_set_text_color (0x00, 0xFF, 0x00);
      break;
    case 11:			/* yellow */
      avt_set_text_color (0xE0, 0xE0, 0x00);
      break;
    case 12:			/* lightblue */
      avt_set_text_color (0x00, 0x00, 0xFF);
      break;
    case 13:			/* lightmagenta */
      avt_set_text_color (0xFF, 0x00, 0xFF);
      break;
    case 14:			/* lightcyan */
      avt_set_text_color (0x00, 0xFF, 0xFF);
      break;
    case 15:			/* white */
      avt_set_text_color (0xFF, 0xFF, 0xFF);
    }
}

static void
set_background_color (int color)
{
  switch (color)
    {
    case 0:			/* black */
      avt_set_text_background_color (0x00, 0x00, 0x00);
      break;
    case 1:			/* red */
      avt_set_text_background_color (0x88, 0x00, 0x00);
      break;
    case 2:			/* green */
      avt_set_text_background_color (0x00, 0x88, 0x00);
      break;
    case 3:			/* brown */
      avt_set_text_background_color (0x88, 0x44, 0x22);
      break;
    case 4:			/* blue */
      avt_set_text_background_color (0x00, 0x00, 0x88);
      break;
    case 5:			/* magenta */
      avt_set_text_background_color (0x88, 0x00, 0x88);
      break;
    case 6:			/* cyan */
      avt_set_text_background_color (0x00, 0x88, 0x88);
      break;
    case 7:			/* lightgray */
      avt_set_text_background_color (0x88, 0x88, 0x88);
      break;
    case 8:			/* darkgray */
      avt_set_text_background_color (0x55, 0x55, 0x55);
      break;
    case 9:			/* lightred */
      avt_set_text_background_color (0xFF, 0x00, 0x00);
      break;
    case 10:			/* lightgreen */
      avt_set_text_background_color (0x00, 0xFF, 0x00);
      break;
    case 11:			/* yellow */
      avt_set_text_background_color (0xFF, 0xFF, 0x00);
      break;
    case 12:			/* lightblue */
      avt_set_text_background_color (0x00, 0x00, 0xFF);
      break;
    case 13:			/* lighmagenta */
      avt_set_text_background_color (0xFF, 0x00, 0xFF);
      break;
    case 14:			/* lightcyan */
      avt_set_text_background_color (0x00, 0xFF, 0xFF);
      break;
    case 15:			/* white */
      avt_set_text_background_color (0xFF, 0xFF, 0xFF);
    }
}

static void
ansi_graphic_code (int mode)
{
  switch (mode)
    {
    case 0:			/* normal */
      faint = AVT_FALSE;
      text_color = 0;
      text_background_color = 0xF;
      avt_normal_text ();
      set_foreground_color (text_color);
      set_background_color (text_background_color);
      break;

    case 1:			/* bold */
      faint = AVT_FALSE;
      avt_bold (AVT_TRUE);
      /* bold is sometimes assumed to light colors */
      if (text_color > 0 && text_color < 7)
	{
	  text_color += 8;
	  set_foreground_color (text_color);
	}
      break;

    case 2:			/* faint */
      avt_bold (AVT_FALSE);
      faint = AVT_TRUE;
      if (text_color == 0)
	set_foreground_color (text_color);
      break;

    case 4:			/* underlined */
    case 21:			/* double underlined (ambiguous) */
      avt_underlined (AVT_TRUE);
      break;

    case 5:			/* blink */
      break;

    case 22:			/* normal intensity */
      avt_bold (AVT_FALSE);
      faint = AVT_FALSE;
      break;

    case 24:			/* not underlined */
      avt_underlined (AVT_FALSE);
      break;

    case 25:			/* blink off */
      break;

    case 7:			/* inverse */
      avt_inverse (AVT_TRUE);
      break;

    case 27:			/* not inverse */
      avt_inverse (AVT_FALSE);
      break;

    case 8:			/* hidden */
    case 9:
      set_foreground_color (text_background_color);
      break;

    case 28:			/* not hidden */
      set_foreground_color (text_color);
      break;

    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
      if (!nocolor)
	{
	  text_color = (mode - 30);
	  /* bold is sometimes assumed to be in light color */
	  if (text_color > 0 && text_color < 7 && avt_get_bold ())
	    text_color += 8;
	  set_foreground_color (text_color);
	}
      break;

    case 38:			/* foreground normal, underlined */
      text_color = 0;
      set_foreground_color (text_color);
      avt_underlined (AVT_TRUE);
      break;

    case 39:			/* foreground normal */
      text_color = 0;
      set_foreground_color (text_color);
      avt_underlined (AVT_FALSE);
      break;

    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
      if (!nocolor)
	{
	  text_background_color = (mode - 40);
	  set_background_color (text_background_color);
	}
      break;

    case 49:			/* background normal */
      text_background_color = 0xF;
      set_background_color (text_background_color);
      break;

    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
      if (!nocolor)
	{
	  text_color = (mode - 90 + 8);
	  set_foreground_color (text_color);
	}
      break;

    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
      if (!nocolor)
	{
	  text_background_color = (mode - 100 + 8);
	  set_background_color (text_background_color);
	}
      break;
    }
}

#define ESC_UNUPPORTED "unsupported escape sequence"
#define CSI_UNUPPORTED "unsupported CSI sequence"


/* Esc [ ... */
/* CSI */
static void
CSI_sequence (int fd, wchar_t last_character)
{
  wchar_t ch;
  char sequence[80];
  unsigned int pos = 0;

  do
    {
      ch = get_character (fd);
      sequence[pos] = (char) ch;
      pos++;
    }
  while (pos < sizeof (sequence) && ch < L'@');
  sequence[pos] = '\0';
  pos++;

#ifdef DEBUG
  fprintf (stderr, "CSI %s\n", sequence);
#endif

  /* ch has last character in the sequence */
  switch (ch)
    {
    case L'@':			/* ICH */
      if (sequence[0] == '@')
	avt_insert_spaces (1);
      else
	avt_insert_spaces (strtol (sequence, NULL, 10));
      break;

    case L'A':			/* CUU */
      if (sequence[0] == 'A')
	avt_move_y (avt_where_y () - 1);
      else
	avt_move_y (avt_where_y () - strtol (sequence, NULL, 10));
      break;

    case L'a':			/* HPR */
      if (sequence[0] == 'a')
	avt_move_x (avt_where_x () + 1);
      else
	avt_move_x (avt_where_x () + strtol (sequence, NULL, 10));
      break;

    case L'B':			/* CUD */
      if (sequence[0] == 'B')
	avt_move_y (avt_where_y () + 1);
      else
	avt_move_y (avt_where_y () + strtol (sequence, NULL, 10));
      break;

    case L'b':			/* REP */
      if (sequence[0] == 'b')
	avt_put_character (last_character);
      else
	{
	  int count = strtol (sequence, NULL, 10);
	  int i;
	  for (i = 0; i < count; i++)
	    avt_put_character (last_character);
	}
      break;

    case L'C':			/* CUF */
      if (sequence[0] == 'C')
	avt_move_x (avt_where_x () + 1);
      else
	avt_move_x (avt_where_x () + strtol (sequence, NULL, 10));
      break;

    case L'c':			/* DA */
      if (sequence[0] == 'c')
	write (prg_input, DS, sizeof (DS) - 1);
      else if (sequence[0] == '?')
	{			/* I have no real infos about that :-( */
	  if (sequence[1] == '1' && sequence[2] == 'c')
	    avt_activate_cursor (AVT_FALSE);
	  else if (sequence[1] == '2' && sequence[2] == 'c')
	    avt_activate_cursor (AVT_TRUE);
	  else if (sequence[1] == '0' && sequence[2] == 'c')
	    avt_activate_cursor (AVT_TRUE);	/* normal? */
	  else if (sequence[1] == '8' && sequence[2] == 'c')
	    avt_activate_cursor (AVT_TRUE);	/* very visible */
	}
      break;

    case L'D':			/* CUB */
      if (sequence[0] == 'D')
	avt_move_x (avt_where_x () - 1);
      else
	avt_move_x (avt_where_x () - strtol (sequence, NULL, 10));
      break;

    case L'd':			/* VPA */
      if (sequence[0] == 'd')
	avt_move_y (1);
      else
	avt_move_y (strtol (sequence, NULL, 10));
      break;

    case L'E':			/* CNL */
      avt_move_x (1);
      if (sequence[0] == 'E')
	avt_move_y (avt_where_y () + 1);
      else
	avt_move_y (avt_where_y () + strtol (sequence, NULL, 10));
      break;

    case L'e':			/* VPR */
      if (sequence[0] == 'e')
	avt_move_y (avt_where_y () + 1);
      else
	avt_move_y (avt_where_y () + strtol (sequence, NULL, 10));
      break;

    case L'F':			/* CPL */
      avt_move_x (1);
      if (sequence[0] == 'F')
	avt_move_y (avt_where_y () - 1);
      else
	avt_move_y (avt_where_y () - strtol (sequence, NULL, 10));
      break;

      /* L'f', HVP: see H */

    case L'g':			/* TBC */
      if (sequence[0] == 'g' || sequence[0] == '0')
	avt_set_tab (avt_where_x (), AVT_FALSE);
      else			/* TODO: 1-5 are not distinguished here */
	avt_clear_tab_stops ();
      break;

    case L'G':			/* CHA */
      if (sequence[0] == 'G')
	avt_move_x (1);
      else
	avt_move_x (strtol (sequence, NULL, 10));
      break;

    case L'H':			/* CUP */
    case L'f':			/* HVP */
      if (sequence[0] == 'H' || sequence[0] == 'f')
	avt_move_xy (1, 1);
      else
	{
	  int n, m;
	  get_2_values (sequence, &n, &m);
	  if (n <= 0)
	    n = 1;
	  if (m <= 0)
	    m = 1;
	  avt_move_xy (m, n);
	}
      break;

    case L'h':			/* DECSET */
      if (sequence[0] == '?')
	{
	  int val = strtol (&sequence[1], NULL, 10);
	  switch (val)
	    {
	    case 1:
	      dec_cursor_seq[1] = 'O';
	      break;
	    case 5:		/* hack: non-standard! */
	      /* by definition it is reverse video */
	      avt_flash ();
	      break;
	    case 6:
	      avt_set_origin_mode (AVT_TRUE);
	      break;
	    case 9:		/* X10 mouse */
	      /* TODO: doesn't work yet */
	      /* avt_register_mousehandler (prg_mousehandler); */
	      break;
	    case 25:
	      avt_activate_cursor (AVT_TRUE);
	      break;
	    case 56:		/* AKFAvatar extension */
	      /* text delay, slow-print */
	      avt_set_text_delay (AVT_DEFAULT_TEXT_DELAY);
	      break;
	    }
	}
      else
	{
	  int val = strtol (sequence, NULL, 10);
	  switch (val)
	    {
	    case 4:
	      insert_mode = AVT_TRUE;
	      break;
	    case 20:
	      avt_newline_mode (AVT_TRUE);
	      break;
	    }
	}
      break;

    case L'l':			/* DECRST */
      if (sequence[0] == '?')
	{
	  int val = strtol (&sequence[1], NULL, 10);
	  switch (val)
	    {
	    case 1:
	      dec_cursor_seq[1] = '[';
	      break;
	    case 5:
	      /* ignored, see 'h' above */
	      break;
	    case 6:
	      avt_set_origin_mode (AVT_FALSE);
	      break;
	    case 9:		/* X10 mouse */
	      /* TODO: doesn't work yet */
	      /* avt_register_mousehandler (NULL); */
	      break;
	    case 25:
	      avt_activate_cursor (AVT_FALSE);
	      break;
	    case 56:		/* AKFAvatar extension */
	      /* no text delay */
	      avt_set_text_delay (0);
	      break;
	    }
	}
      else
	{
	  int val = strtol (sequence, NULL, 10);
	  switch (val)
	    {
	    case 4:
	      insert_mode = AVT_FALSE;
	      break;
	    case 20:
	      avt_newline_mode (AVT_FALSE);
	      break;
	    }
	}
      break;

    case L'J':			/* ED */
      if (sequence[0] == '0' || sequence[0] == 'J')
	avt_clear_down ();
      else if (sequence[0] == '1')
	avt_clear_up ();
      else if (sequence[0] == '2')
	avt_clear ();
      break;

    case L'K':			/* EL */
      if (sequence[0] == '0' || sequence[0] == 'K')
	avt_clear_eol ();
      else if (sequence[0] == '1')
	avt_clear_bol ();
      else if (sequence[0] == '2')
	avt_clear_line ();
      break;

    case L'm':			/* SGR */
      if (sequence[0] == 'm')
	ansi_graphic_code (0);
      else
	{
	  char *next;

	  next = &sequence[0];
	  while (*next >= '0' && *next <= '9')
	    {
	      ansi_graphic_code (strtol (next, &next, 10));
	      if (*next == ';')
		next++;
	    }
	}
      break;

    case L'L':			/* IL */
      if (sequence[0] == 'L')
	avt_insert_lines (avt_where_y (), 1);
      else
	avt_insert_lines (avt_where_y (), strtol (sequence, NULL, 10));
      break;

    case L'M':			/* DL */
      if (sequence[0] == 'M')
	avt_delete_lines (avt_where_y (), 1);
      else
	avt_delete_lines (avt_where_y (), strtol (sequence, NULL, 10));
      break;

    case L'n':			/* DSR */
      if (sequence[0] == '5' && sequence[1] == 'n')
	write (prg_input, "\033[0n", 4);	/* device okay */
      /* "\033[3n" for failure */
      else if (sequence[0] == '6' && sequence[1] == 'n')
	{
	  /* report cursor position */
	  char s[80];
	  snprintf (s, sizeof (s), "\033[%d;%dR",
		    avt_where_x (), avt_where_y ());
	  write (prg_input, s, strlen (s));
	}
      /* other values are unknown */
      break;

    case L'P':			/* DCH */
      if (sequence[0] == 'P')
	avt_delete_characters (1);
      else
	avt_delete_characters (strtol (sequence, NULL, 10));
      break;

    case L'r':			/* CSR */
      if (sequence[0] == 'r')
	{
	  region_min_y = 1;
	  region_max_y = max_y;
	  avt_viewport (1, region_min_y, max_x, region_max_y);
	}
      else
	{
	  int min, max;

	  get_2_values (sequence, &min, &max);
	  if (min <= 0)
	    min = 1;
	  if (max <= 0)
	    max = 1;

	  avt_viewport (1, min, max_x - 1 + 1, max - min + 1);

	  if (avt_get_origin_mode ())
	    {
	      region_min_y = 1;
	      region_max_y = max - min + 1;
	    }
	  else			/* origin_mode not set */
	    {
	      region_min_y = min;
	      region_max_y = max;
	    }
	}
      break;

    case L's':			/* SCP */
      avt_save_position ();
      break;

    case L'u':			/* RCP */
      avt_restore_position ();
      break;

    case L'X':			/* ECH */
      if (sequence[0] == 'X')
	avt_erase_characters (1);
      else
	avt_erase_characters (strtol (sequence, NULL, 10));
      break;

    case L'Z':			/* CBT */
      if (sequence[0] == 'Z')
	avt_last_tab ();
      else
	{
	  int i;
	  int count = strtol (sequence, NULL, 10);
	  for (i = 0; i < count; i++)
	    avt_last_tab ();
	}
      break;

    case L'`':			/* HPA */
      if (sequence[0] == '`')
	avt_move_x (1);
      else
	avt_move_x (strtol (sequence, NULL, 10));
      break;

#ifdef DEBUG
    default:
      warning_msg (CSI_UNUPPORTED, sequence);
#endif
    }
}

static void
escape_sequence (int fd, wchar_t last_character)
{
  wchar_t ch;
  static int saved_text_color, saved_text_background_color;
  static avt_bool_t saved_underline_state, saved_bold_state;

  ch = get_character (fd);

#ifdef DEBUG
  if (ch != L'[')
    fprintf (stderr, "ESC %lc\n", ch);
#endif

  /* ESC [ch] */
  switch (ch)
    {
    case L'7':			/* DECSC */
      avt_save_position ();
      saved_text_color = text_color;
      saved_text_background_color = text_background_color;
      saved_underline_state = avt_get_underlined ();
      saved_bold_state = avt_get_bold ();
      break;

    case L'8':			/* DECRC */
      avt_restore_position ();
      text_color = saved_text_color;
      set_foreground_color (text_color);
      text_background_color = saved_text_background_color;
      set_background_color (text_background_color);
      avt_underlined (saved_underline_state);
      avt_bold (saved_bold_state);
      break;

    case L'c':			/* RIS - reset device */
      region_min_y = 1;
      region_max_y = max_y;
      avt_viewport (1, region_min_y, max_x, region_max_y);
      avt_newline_mode (AVT_FALSE);
      avt_set_origin_mode (AVT_FALSE);
      avt_reset_tab_stops ();
      text_color = saved_text_color = 0;
      text_background_color = saved_text_background_color = 0xF;
      ansi_graphic_code (0);
      avt_set_text_delay (0);
      insert_mode = AVT_FALSE;
      avt_clear ();
      avt_save_position ();
      return;

    case L'D':			/* move down or scroll up one line */
      if (avt_where_y () < region_max_y)
	avt_move_y (avt_where_y () + 1);
      else
	avt_delete_lines (region_min_y, 1);
      return;

    case L'E':
      avt_move_x (1);
      avt_new_line ();
      return;

/* for some few terminals it's the home function 
    case L'H':
      avt_move_x (1);
      avt_move_y (1);
      return;
*/

    case L'H':			/* HTS */
      avt_set_tab (avt_where_x (), AVT_TRUE);
      return;

    case L'M':			/* RI - scroll down one line */
      if (avt_where_y () > region_min_y)
	avt_move_y (avt_where_y () - 1);
      else
	avt_insert_lines (region_min_y, 1);
      return;

    case L'Z':			/* DECID */
      write (prg_input, DS, sizeof (DS) - 1);
      return;

    case L']':			/* Linux specific: (re)set palette */
      /* not supported, but fetch the right number of chars to be ignored */
      {
	wchar_t ch2 = get_character (fd);
	/* ignore L'R' (reset palette) */
	if (ch2 == L'P')	/* set palette */
	  {
	    /* ignore 7 characters */
	    get_character (fd);
	    get_character (fd);
	    get_character (fd);
	    get_character (fd);
	    get_character (fd);
	    get_character (fd);
	    get_character (fd);
	  }
      }
      return;

    default:
      if (ch == L'[')
	CSI_sequence (fd, last_character);
      else
	{
	  fprintf (stderr, ESC_UNUPPORTED " %lc\n", ch);
	  return;
	}
      break;
    }
}

static void
process_subprogram (int fd)
{
  avt_bool_t stop;
  wint_t ch;
  wchar_t last_character;

  text_color = 0;
  last_character = L'\0';
  text_background_color = 0xF;
  stop = AVT_FALSE;
  avt_reserve_single_keys (AVT_TRUE);
  avt_newline_mode (AVT_FALSE);
  avt_activate_cursor (AVT_TRUE);
  set_encoding (default_encoding);

  /* like vt102 */
  avt_set_origin_mode (AVT_FALSE);

  while ((ch = get_character (fd)) != WEOF && !stop)
    {
      if (ch == L'\033')	/* Esc */
	escape_sequence (fd, last_character);
      else if (ch == L'\x9b')	/* CSI */
	CSI_sequence (fd, last_character);
      else
	{
	  last_character = (wchar_t) ch;

	  if (insert_mode)
	    avt_insert_spaces (1);

	  stop = avt_put_character ((wchar_t) ch);
	}
    }

  avt_activate_cursor (AVT_FALSE);
  avt_reserve_single_keys (AVT_FALSE);

  /* close file descriptor */
  if (close (fd) == -1 && errno != EAGAIN)
    warning_msg ("close", strerror (errno));

  /* release keyhandler */
  avt_register_keyhandler (NULL);
  prg_input = -1;
}

/* execute a subprocess, visible in the balloon */
/* if fname == NULL, start a shell */
/* returns file-descriptor for output of the process */
static int
execute_process (char *const prg_argv[])
{
  pid_t childpid;
  int master, slave;
  char *terminalname;
  struct termios settings;
  struct winsize size;		/* does this have to be static? */
  char *shell = "/bin/sh";

  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;

  /* must be initialized to get the window size */
  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (prg_argv == NULL)
    shell = get_user_shell ();

  /* as specified in POSIX.1-2001 */
  master = posix_openpt (O_RDWR);

  /* some older systems: */
  /* master = open("/dev/ptmx", O_RDWR); */

  if (master < 0)
    return -1;

  if (grantpt (master) < 0 || unlockpt (master) < 0)
    {
      close (master);
      return -1;
    }

  terminalname = ptsname (master);

  if (terminalname == NULL)
    {
      close (master);
      return -1;
    }

  slave = open (terminalname, O_RDWR);

  if (slave < 0)
    {
      close (master);
      return -1;
    }

  /*-------------------------------------------------------- */
  childpid = fork ();

  if (childpid == -1)
    {
      close (master);
      close (slave);
      return -1;
    }

  /* is it the child process? */
  if (childpid == 0)
    {
      /* child closes master */
      close (master);

      /* create a new session */
      setsid ();

      /* redirect stdin */
      if (dup2 (slave, STDIN_FILENO) == -1)
	_exit (EXIT_FAILURE);

      /* redirect stdout */
      if (dup2 (slave, STDOUT_FILENO) == -1)
	_exit (EXIT_FAILURE);

      /* redirect sterr */
      if (dup2 (slave, STDERR_FILENO) == -1)
	_exit (EXIT_FAILURE);

      close (slave);

      /* set the controling terminal */
#ifdef TIOCSCTTY
      ioctl (STDIN_FILENO, TIOCSCTTY, 0);
#endif

      if (nocolor)
	putenv ("TERM=" BWTERM);
      else
	putenv ("TERM=" TERM);

      /* programs can identify avatarsay with this */
      putenv ("AKFAVTTERM=" AVTVERSION);

      if (prg_argv == NULL)	/* execute shell */
	execl (shell, shell, (char *) NULL);
      else			/* execute the command */
	execvp (prg_argv[0], prg_argv);

      /* in case of an error, we can not do much */
      /* stdout and stderr are broken by now */
      _exit (EXIT_FAILURE);
    }

  /* parent process */
  close (slave);

  /* terminal settings */
  if (tcgetattr (master, &settings) < 0)
    {
      close (master);
      return -1;
    }

  /* TODO: improve */
  settings.c_cc[VERASE] = 8;	/* Backspace */
  settings.c_iflag |= ICRNL;	/* input: cr -> nl */
  settings.c_lflag |= (ECHO | ECHOE | ECHOK | ICANON);

  if (tcsetattr (master, TCSANOW, &settings) < 0)
    {
      close (master);
      return -1;
    }

#ifdef TIOCSWINSZ
  /* set window size */
  /* not portable? */
  size.ws_row = max_y;
  size.ws_col = max_x;
  size.ws_xpixel = size.ws_ypixel = 0;
  ioctl (master, TIOCSWINSZ, &size);
#endif

  fcntl (master, F_SETFL, O_NONBLOCK);

  prg_input = master;
  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
  dec_cursor_seq[2] = ' ';	/* to be filled later */
  avt_register_keyhandler (prg_keyhandler);
  /* TODO: doesn't work yet */
  /* avt_register_mousehandler (prg_mousehandler); */
  read_error_is_eof = AVT_TRUE;

  return master;
}

static void
run_shell (void)
{
  int fd;

  avt_clear ();
  avt_set_text_delay (0);
  avt_text_direction (AVT_LEFT_TO_RIGHT);
  chdir (get_user_home ());

  fd = execute_process (NULL);
  if (fd > -1)
    process_subprogram (fd);
}

static void
run_info (void)
{
  int fd;
  char *args[] = { "info", "akfavatar-en", NULL };

  avt_clear ();
  avt_set_text_delay (0);
  avt_text_direction (AVT_LEFT_TO_RIGHT);

  if (language == DEUTSCH)
    args[1] = "akfavatar-de";

  fd = execute_process (args);
  if (fd > -1)
    process_subprogram (fd);
}

#endif /* not NO_PTY */


#ifdef NO_MANPAGES

static void
ask_manpage (void)
{
  not_available ();
}

#else /* not NO_MANPAGES */

#ifndef NO_PTY
static void
ask_manpage (void)
{
  char manpage[AVT_LINELENGTH] = "man";
  char *argv[] = { "man", "-t", (char *) &manpage, NULL };

  avt_clear ();
  avt_set_text_delay (0);

  avt_say (L"Manpage> ");

  if (avt_ask_mb (manpage, sizeof (manpage)) != 0)
    quit (EXIT_SUCCESS);

  avt_clear ();
  avt_set_text_delay (default_delay);
  if (manpage[0] != '\0')
    {
      int fd, status;

      /* GROFF assumed! */
      putenv ("GROFF_TYPESETTER=latin1");
      putenv ("MANWIDTH=80");

      /* temporary settings */
      set_encoding ("ISO-8859-1");

      /* ignore file errors */
      fd = execute_process (argv);

      /* unset keyhandler, set by execute_process */
      avt_register_keyhandler (NULL);
      prg_input = -1;

      if (fd > -1)
	process_file (fd);

      status = avt_get_status ();
      if (status == AVT_ERROR)
	quit (EXIT_FAILURE);	/* warning already printed */

      if (status == AVT_NORMAL)
	if (avt_wait_button () != 0)
	  quit (EXIT_SUCCESS);

      /* reset quit-request */
      avt_set_status (AVT_NORMAL);
      set_encoding (default_encoding);
    }
}

#endif /* not NO_PTY */
#endif /* not NO_MANPAGES */

#ifdef __WIN32__

static void
edit_file (const char *name)
{
  ShellExecute (NULL, "open", "notepad.exe", name, NULL /* dir */ ,
		SW_SHOWMAXIMIZED);

  /*
   * program returns immediately,
   * so leave some time to see the message
   */
  avt_wait (AVT_SECONDS (5));
}

#else /* not Windows or ReactOS */

static void
edit_file (const char *name)
{
  char *editor;
  char *args[3];
  int fd;

  editor = getenv ("VISUAL");
  if (!editor)
    editor = getenv ("EDITOR");
  if (!editor)
    editor = "vi";

  args[0] = editor;
  args[1] = (char *) name;
  args[2] = (char *) NULL;

  fd = execute_process (args);
  if (fd > -1)
    process_subprogram (fd);
}

static char *
get_program_path (void)
{
  char *p;

  p = (char *) malloc (strlen (PREFIX) + 1 + strlen (program_name) + 1);
  strcpy (p, PREFIX);
  strcat (p, "/");
  strcat (p, program_name);

  return p;
}

#endif /* not Windows or ReactOS */

static void
create_file (const char *filename)
{
  FILE *f;

  f = fopen (filename, "wx");

  if (f)
    {
#ifndef __WIN32__
      /* include #! line */
      {
	char *p = get_program_path ();
	fprintf (f, "#! %s\n\n", p);
	free (p);
      }
#endif

      if (default_encoding[0] != '\0')
	fprintf (f, ".encoding %s\n", default_encoding);

      if (datadir[0] != '\0')
	fprintf (f, ".datadir %s\n", datadir);

      if (avt_image_name)
	fprintf (f, ".avatarimage %s\n", avt_image_name);

      fputs ("\n# Text:\n", f);

      fclose (f);

#ifndef __WIN32__
      /* make file executable */
      chmod (filename, S_IRUSR | S_IWUSR | S_IXUSR
	     | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
    }
}

static void
ask_edit_file (void)
{
  char filename[256];

  avt_clear ();
  avt_set_text_delay (0);

  chdir (datadir);

  /* show directory and prompt (don't trust "datadir") */
  avt_say_mb (datadir);
  avt_say_mb ("> ");

  if (avt_ask_mb (filename, sizeof (filename)) != 0)
    quit (EXIT_SUCCESS);

  if (filename[0] != '\0')
    {
      if (access (filename, F_OK) != 0)
	create_file (filename);

      avt_clear ();
      switch (language)
	{
	case DEUTSCH:
	  avt_say (L"öffne Datei im editor...");
	  break;

	case ENGLISH:
	default:
	  avt_say (L"opening file in an editor...");
	}
      edit_file (filename);
    }
}

static void
about_avatarsay (void)
{
  avt_clear ();
  set_encoding ("UTF-8");
  avt_set_text_delay (0);

  switch (language)
    {
    case DEUTSCH:
      avt_say_mb (version_info_de);
      break;

    case ENGLISH:
    default:
      avt_say_mb (version_info_en);
    }

  avt_say_mb ("\n\nHomepage:  " HOMEPAGE);

  set_encoding (default_encoding);
  avt_set_text_delay (default_delay);

  if (avt_wait_button () != 0)
    quit (EXIT_SUCCESS);
}

#ifdef NO_MANPAGES
#  define SAY_MANPAGE(x) \
     avt_set_text_color (0x88, 0x88, 0x88); \
     avt_say(x); \
     avt_set_text_color (0x00, 0x00, 0x00)
#else
#  define SAY_MANPAGE(x) avt_say(x)
#endif

#ifdef NO_PTY
#  define SAY_SHELL(x) \
     avt_set_text_color (0x88, 0x88, 0x88); \
     avt_say(x); \
     avt_set_text_color (0x00, 0x00, 0x00)
#else
#  define SAY_SHELL(x) avt_say(x)
#endif

static void
menu (void)
{
  wchar_t ch;
  int menu_start, menu_end;

  /* avoid pause after moving out */
  loop = AVT_FALSE;

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  set_encoding (default_encoding);

  while (1)
    {
      avt_clear ();
      avt_normal_text ();
      avt_set_text_delay (0);
      avt_set_origin_mode (AVT_FALSE);
      avt_newline_mode (AVT_TRUE);
      avt_viewport (19, 1, 42, avt_get_max_y ());

      avt_underlined (AVT_TRUE);
      avt_bold (AVT_TRUE);
      avt_say (L"AKFAvatar");
      avt_bold (AVT_FALSE);
      avt_underlined (AVT_FALSE);
      avt_new_line ();
      avt_new_line ();

      menu_start = avt_where_y ();

      switch (language)
	{
	case DEUTSCH:
	  SAY_SHELL (L"1) Terminal-Modus\n");
	  avt_say (L"2) ein Demo oder eine Text-Datei anzeigen\n");
	  avt_say (L"3) ein Demo erstellen oder bearbeiten\n");
	  SAY_MANPAGE (L"4) eine Hilfeseite (Manpage) anzeigen\n");
	  SAY_SHELL (L"5) Anleitung (info)\n");
	  avt_say (L"6) über avatarsay\n");
	  avt_say (L"7) beenden\n");
	  break;

	case ENGLISH:
	default:
	  SAY_SHELL (L"1) terminal-mode\n");
	  avt_say (L"2) show a demo or textfile\n");
	  avt_say (L"3) create or edit a demo\n");
	  SAY_MANPAGE (L"4) show a manpage\n");
	  SAY_SHELL (L"5) documentation (info)\n");
	  avt_say (L"6) about avatarsay\n");
	  avt_say (L"7) exit\n");
	}

      menu_end = avt_where_y () - 1;
      avt_set_text_delay (default_delay);

      if (avt_get_menu (&ch, menu_start, menu_end, L'1'))
	quit (EXIT_SUCCESS);

      avt_viewport (1, 1, avt_get_max_x (), avt_get_max_y ());

      switch (ch)
	{
	case L'1':		/* terminal-mode */
	  run_shell ();
	  break;

	case L'2':		/* show a demo or textfile */
	  ask_file ();
	  break;

	case L'3':		/* create or edit a demo */
	  ask_edit_file ();
	  break;

	case L'4':		/* show a manpage */
	  ask_manpage ();
	  break;

	case L'5':		/* documentation */
	  run_info ();
	  break;

	case L'6':		/* about avatarsay */
	  about_avatarsay ();
	  break;

	case L'7':		/* exit */
	  if (!popup)
	    move_out ();
	  quit (EXIT_SUCCESS);

	default:
	  avt_bell ();
	  break;
	}
    }
}

static void
init_language_info (void)
{
  char *locale_info;

  /* initialize system's default locale */
  locale_info = setlocale (LC_ALL, "");

  /* we are interested only in the laguage here */
#ifdef LC_MESSAGES
  locale_info = setlocale (LC_MESSAGES, NULL);
#endif

  /* default lagnuage */
  language = ENGLISH;

  /* long names for Windows (and possibly others) */
  /* longer names first to avoid coflicts */
  if (strncasecmp (locale_info, "German", 6) == 0)
    language = DEUTSCH;
  else if (strncasecmp (locale_info, "English", 7) == 0)
    language = ENGLISH;
  else if (strncmp (locale_info, "de", 2) == 0)
    language = DEUTSCH;
  else if (strncmp (locale_info, "en", 2) == 0)
    language = ENGLISH;
}

/* strip trailing newline, if any */
static void
strip_newline (char *s)
{
  while (*s)
    {
      if (*s == '\n')
	*s = '\0';
      s++;
    }
}

static void
check_config_file (const char *f)
{
  FILE *cnf;
  char buf[255];
  char *s;

  cnf = fopen (f, "r");
  if (cnf)
    {
      s = fgets (buf, sizeof (buf), cnf);
      while (s)
	{
	  strip_newline (s);

	  if (strncasecmp (s, "AVATARDATADIR=", 14) == 0)
	    strncpy (datadir, s + 14, sizeof (datadir));

	  if (strncasecmp (s, "AVATARIMAGE=", 12) == 0)
	    use_avatar_image (s + 12);

	  if (strncasecmp (s, "BACKGROUNDCOLOR=", 16) == 0)
	    {
	      unsigned int red, green, blue;

	      if (sscanf (s + 16, "#%2x%2x%2x", &red, &green, &blue) == 3)
		avt_set_background_color (red, green, blue);
	      else
		error_msg (f, "bad background color");
	    }

	  s = fgets (buf, sizeof (buf), cnf);
	}
      fclose (cnf);
    }
}

static void
show_file (char *f)
{
  int status = -1;
  int fd;

  set_encoding (default_encoding);

  fd = openfile (f);

  /* if it can't be opened and there is no slash, try with datadir */
  if (fd == -1 && strchr (f, '/') == NULL)
    {
      char p[PATH_MAX];
      strcpy (p, datadir);
      strcat (p, "/");
      strcat (p, f);
      fd = openfile (p);
    }

  if (fd > -1)
    status = process_file (fd);

  if (status < 0)
    error_msg ("error opening file", f);
  else if (status == 1)		/* halt requested */
    quit (EXIT_SUCCESS);
  else if (status > 1)		/* problem with libakfavatar */
    quit (EXIT_FAILURE);

  if (avt_flip_page ())
    quit (EXIT_SUCCESS);
}

static void
initialize_program_name (const char *argv0)
{
#ifdef __WIN32__
  program_name = strrchr (argv0, '\\');
#else
  program_name = strrchr (argv0, '/');
#endif
  if (program_name == NULL)	/* no slash */
    program_name = argv0;
  else				/* skip slash */
    program_name++;
}

int
main (int argc, char *argv[])
{
  /* initialize variables */
  window_mode = AVT_AUTOMODE;
  loop = AVT_TRUE;
  default_delay = AVT_DEFAULT_TEXT_DELAY;
  prg_input = -1;
  initialize_program_name (argv[0]);

  /* this is just a default setting */
  strcpy (default_encoding, "ISO-8859-1");

  /* set datadir to home */
  strncpy (datadir, get_user_home (), sizeof (datadir));
  datadir[sizeof (datadir) - 1] = '\0';

  init_language_info ();

  /* get system encoding */
#ifndef NO_LANGINFO
  strncpy (default_encoding, nl_langinfo (CODESET),
	   sizeof (default_encoding));

/* get canonical encoding name for SDL's iconv */
#  ifndef FORCE_ICONV
  if (strcmp ("ISO_8859-1", default_encoding) == 0 ||
      strcmp ("ISO8859-1", default_encoding) == 0)
    strcpy (default_encoding, "ISO-8859-1");
#  endif /* not FORCE_ICONV */
#endif /* not NO_LANGINFO */

  check_config_file ("/etc/avatarsay");
  checkenvironment ();
  checkoptions (argc, argv);

  if (terminal_mode)
    {
      run_shell ();
      quit (EXIT_SUCCESS);
    }
#ifndef NO_PTY
  else if (executable)
    {
      int fd;
      fd = execute_process (&argv[optind]);
      if (fd > -1)
	process_subprogram (fd);

      if (avt_wait_button ())
	quit (EXIT_SUCCESS);

      if (initialized && !popup)
	move_out ();
      quit (EXIT_SUCCESS);
    }
#endif /* not NO_PTY */
  else if (optind >= argc)	/* no input files? -> menu */
    menu ();

  /* handle files given as parameters */
  do
    {
      int i;

      if (initialized && !popup)
	move_in ();

      for (i = optind; i < argc; i++)
	show_file (argv[i]);

      if (initialized && !popup)
	move_out ();
    }
  while (loop);

  quit (EXIT_SUCCESS);

  /* never executed, but kept in the code */
  puts ("$Id: avatarsay.c,v 2.140 2008-05-17 19:54:30 akf Exp $");

  return EXIT_SUCCESS;
}
