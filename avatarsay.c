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

/* $Id: avatarsay.c,v 2.56 2008-01-13 09:27:44 akf Exp $ */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "version.h"
#include "akfavatar.h"
#include <wchar.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <getopt.h>

#ifdef __WIN32__
#  include <windows.h>
#  define NO_MANPAGES 1
#  ifdef __MINGW32__
#    define NO_FIFO 1
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

#ifndef NO_FIFO
#  include <sys/stat.h>
#endif

#define PRGNAME "avatarsay"
#define HOMEPAGE "http://akfoerster.de/akfavatar/"
#define BUGMAIL "bug-akfavatar@akfoerster.de"

/* size for input buffer - not too small, please */
/* .encoding must be in first buffer */
#define INBUFSIZE 10240

/* for keybuffer (external processes) */
#define KEY_BUFFER_SIZE 255

/* maximum size for path */
/* should fit into stack */
#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

static const char *version_info_en =
  PRGNAME " (AKFAvatar) " AVTVERSION "\n"
  "Copyright (c) 2007 Andreas K. Foerster\n\n"
  "License GPLv3+: GNU GPL version 3 or later "
  "<http://gnu.org/licenses/gpl.html>\n\n"
  "This is free software: you are free to change and redistribute it.\n"
  "There is NO WARRANTY, to the extent permitted by law.\n\n"
  "Please read the manual for instructions.";

static const char *version_info_de =
  PRGNAME " (AKFAvatar) " AVTVERSION "\n"
  "Copyright (c) 2007 Andreas K. Foerster\n\n"
  "Lizenz GPLv3+: GNU GPL Version 3 oder neuer "
  "<http://gnu.org/licenses/gpl.html>\n\n"
  "Dies ist Freie Software: Sie dürfen es gemäß der GPL weitergeben und\n"
  "überarbeiten. Für AKFAavatar besteht KEINERLEI GARANTIE.\n\n"
  "Bitte lesen Sie die Anleitung für weitere Details.";

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

/* create a fifo for what to say? */
static avt_bool_t say_pipe;

/* execute file? Option -e */
static avt_bool_t executable;
static avt_bool_t read_error_is_eof;
static int prg_input;

/* in idle loop? */
static avt_bool_t idle;

/* whether to run in a window, or in fullscreen mode */
/* the mode can be set by -f, --fullscreen or -w, --window */
/* or with --fullfullscreen or -F */
static int mode;

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

/* for loading sound files */
static avt_audio_t *sound;

/* text-buffer */
static int wcbuf_pos = 0;
static int wcbuf_len = 0;

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
help (const char *prgname)
{
  printf ("\nUsage: %s [Options] textfile(s)\n\n", prgname);
  puts ("A fancy text-viewer and scripting language for making demos.\n");
  puts ("If textfile is - then read from stdin and don't loop.\n");
  puts ("Options:");
  puts (" -h, --help              show this help");
  puts (" -v, --version           show the version");
#ifdef NO_FIFO
  puts (" -e, --execute           not supported on this system");
  puts (" -s, --saypipe           not supported on this system");
#else
  puts (" -e, --execute           execute lineoriented program in balloon");
  puts (" -s, --saypipe           create named pipe for filename");
#endif
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
  puts (" -i, --ignoreeof         ignore end of file conditions "
	"(input is not a file)");
  puts ("\nEnvironment variables:");
  puts (" AVATARIMAGE             different image as avatar");
  puts (" AVATARDATADIR           data-directory");
  puts ("\nReport bugs to <" BUGMAIL ">");
  exit (EXIT_SUCCESS);
}

static void
open_homepage (void)
{
  avt_switch_mode (AVT_WINDOW);
  avt_clear ();
  avt_set_text_delay (0);

  avt_say_mb ("Homepage: " HOMEPAGE "\n");
  switch (language)
    {
    case DEUTSCH:
      avt_say (L"Versuche es in einem Webbrowser zu öffnen...");
      break;
    case ENGLISH:
    default:
      avt_say (L"Trying to open it in a webbrowser...");
    }

  if (getenv ("KDE_FULL_SESSION") != NULL)
    system ("kfmclient openURL " HOMEPAGE " &");
  else if (getenv ("GNOME_DESKTOP_SESSION_ID") != NULL)
    system ("gnome-open " HOMEPAGE " &");
  else
    system ("firefox " HOMEPAGE " &");

  avt_wait (4000);
  quit (EXIT_SUCCESS);
}

#else /* Windows or ReactOS */

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
help (const char *prgname)
{
  showversion ();
  exit (EXIT_SUCCESS);
}

static void
open_homepage (void)
{
  avt_switch_mode (AVT_WINDOW);
  avt_clear ();
  avt_set_text_delay (0);

  avt_say_mb ("Homepage: " HOMEPAGE "\n");
  switch (language)
    {
    case DEUTSCH:
      avt_say (L"Versuche es in einem Webbrowser zu öffnen...");
      break;

    case ENGLISH:
    default:
      avt_say (L"Trying to open it in a webbrowser...");
    }

  ShellExecute (NULL, "open", HOMEPAGE, NULL, NULL, SW_SHOWNORMAL);

  avt_wait (4000);
  quit (EXIT_SUCCESS);
}

#endif /* Windows or ReactOS */

static void
set_encoding (const char *encoding)
{
  if (avt_mb_encoding (encoding))
    error_msg ("iconv", avt_get_error ());
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
	{"execute", no_argument, 0, 'e'},
	{"no-delay", no_argument, 0, 'n'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "hvfFw1risE:lupen",
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
	  help (argv[0]);
	  break;

	case 'v':		/* --version */
	  showversion ();
	  break;

	case 'f':		/* --fullscreen */
	  mode = AVT_FULLSCREEN;
	  break;

	case 'F':		/* --fullfullscreen */
	  mode = AVT_FULLSCREENNOSWITCH;
	  break;

	case 'w':		/* --window */
	  mode = AVT_WINDOW;
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

	case 's':		/* --saypipe */
#ifdef NO_FIFO
	  switch (language)
	    {
	    case DEUTSCH:
	      error_msg ("Pipes werden auf diesem System nicht unterstuetzt",
			 NULL);
	      break;

	    case ENGLISH:
	    default:
	      error_msg ("pipes not supported on this system", NULL);
	    }
#else
	  say_pipe = AVT_TRUE;
	  loop = AVT_FALSE;
	  ignore_eof = AVT_TRUE;
	  /* autodetecting the encoding doesn't work with FIFOs */
	  given_encoding = AVT_TRUE;
#endif /* ! NO_FIFO */
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

	case 'e':		/* --exec */
	  executable = AVT_TRUE;
	  loop = AVT_FALSE;
	  break;

	case 'n':		/* --no-delay */
	  default_delay = 0;
	  avt_set_text_delay (0);
	  break;

	case '?':		/* unsupported option */
	  /* getopt_long already printed an error message to stderr */
	  help (argv[0]);
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

  /* some sanity checks */
  if (say_pipe && executable)
    error_msg ("error", "-s and -e can not be used together");

  if (say_pipe && argc > optind + 1)
    error_msg ("error", "only one file argument for -s allowed");
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
    if ((avt_image = avt_import_image_file (e)) == NULL)
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

  if (avt_initialize ("AKFAvatar", "AKFAvatar", avt_image, mode))
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
  initialized = AVT_TRUE;
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

/* check for byte order mark (BOM) U+FEFF and remove it */
static void
check_encoding (char *buf, int *size)
{
  encoding_checked = AVT_TRUE;

  {
    char *enc;
    char temp[80];

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
      memmove (buf, buf + 3, *size - 3);
      *size -= 3;
      return;
    }

  /* check 32 Bit BOMs before 16 Bit ones, to avoid confusion! */

  /* UTF-32BE BOM */
  if (*buf == '\x00' && *(buf + 1) == '\x00'
      && *(buf + 2) == '\xFE' && *(buf + 3) == '\xFF')
    {
      set_encoding ("UTF-32BE");
      memmove (buf, buf + 4, *size - 4);
      *size -= 4;
      return;
    }

  /* UTF-32LE BOM */
  if (*buf == '\xFF' && *(buf + 1) == '\xFE'
      && *(buf + 2) == '\x00' && *(buf + 3) == '\x00')
    {
      set_encoding ("UTF-32LE");
      memmove (buf, buf + 4, *size - 4);
      *size -= 4;
      return;
    }

  /* UTF-16BE BOM */
  if (*buf == '\xFE' && *(buf + 1) == '\xFF')
    {
      set_encoding ("UTF-16BE");
      memmove (buf, buf + 2, *size - 2);
      *size -= 2;
      return;
    }

  /* UTF-16LE BOM */
  if (*buf == '\xFF' && *(buf + 1) == '\xFE')
    {
      set_encoding ("UTF-16LE");
      memmove (buf, buf + 2, *size - 2);
      *size -= 2;
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

      filebuf_end = read (fd, &filebuf, sizeof (filebuf));

      /* waiting for data */
      if (filebuf_end == -1 && errno == EAGAIN)
	{
	  idle = AVT_TRUE;
	  while (filebuf_end == -1 && errno == EAGAIN
		 && avt_update () == AVT_NORMAL)
	    filebuf_end = read (fd, &filebuf, sizeof (filebuf));
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
	    check_encoding (filebuf, &filebuf_end);

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

  ch = get_character (fd);
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
  struct passwd *user_data;

  user_data = getpwuid (getuid ());
  if (!user_data || !user_data->pw_shell)
    return "/bin/sh";		/* default shell */
  else
    return user_data->pw_shell;
}

void
prg_keyhandler (int sym, int mod, int unicode)
{
  wchar_t ch;
  char *mbstring;
  int mblen;

  if (idle && prg_input > 0 && unicode != 0)
    {
      idle = AVT_FALSE;		/* avoid reentrance */

      ch = (wchar_t) unicode;
      mblen = avt_mb_encode (&mbstring, &ch, 1);
      if (mblen != -1)
	{
	  write (prg_input, mbstring, mblen);
	  avt_free (mbstring);
	}

      idle = AVT_TRUE;
    }				/* if (idle...) */
}

#endif /* not NO_PTY */

/* execute a subprocess, visible in the balloon */
/* if fname == NULL, start a shell */
/* returns file-descriptor for output of the process */
static int
execute_process (const char *fname)
{
#ifdef NO_PTY
  return -1;
#else /* not NO_PTY */

  pid_t childpid;
  int master, slave;
  char *terminalname;
  struct termios settings;
  struct winsize size;		/* does this have to be static? */

  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;

  /* must be initialized to get the window size */
  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

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

      /* redirect stdin */
      if (dup2 (slave, STDIN_FILENO) == -1)
	error_msg ("dup2", strerror (errno));

      /* redirect stdout */
      if (dup2 (slave, STDOUT_FILENO) == -1)
	error_msg ("dup2", strerror (errno));

      /* redirect sterr */
      if (dup2 (slave, STDERR_FILENO) == -1)
	error_msg ("dup2", strerror (errno));

      close (slave);

      /* It's a very very dumb terminal */
      putenv ("TERM=dumb");

      /* probably the only pager that actually works here */
      putenv ("PAGER=more");

      /* programs can identify avatarsay with this */
      putenv ("AVATARSAY=" AVTVERSION);

      if (fname == NULL)
	{			/* execute shell */
	  char *shell = get_user_shell ();
	  execl (shell, shell, (char *) NULL);
	}
      else			/* execute the command */
	execl ("/bin/sh", "/bin/sh", "-c", fname, (char *) NULL);

      /* in case of an error, we can not do much */
      /* stdout and stderr are broken by now */
      _exit (EXIT_FAILURE);
    }
  else				/* parent process */
    {
      /* parent closes slave */
      close (slave);
    }

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

  /* set window size */
  /* not portable? */
  size.ws_row = avt_get_max_y ();
  size.ws_col = avt_get_max_x ();
  size.ws_xpixel = size.ws_ypixel = 0;
  ioctl (master, TIOCSWINSZ, &size);

  fcntl (master, F_SETFL, O_NONBLOCK);

  prg_input = master;
  avt_register_keyhandler (prg_keyhandler);
  read_error_is_eof = AVT_TRUE;

  /* return master */
  return master;
#endif /* not NO_PTY */
}

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

  /* 
   * skip empty lines and handle commands at the beginning 
   * before initializing the graphics
   */
  nread = getwline (fd, line, line_size);
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


static int
process_subprogram (int fd)
{
  avt_bool_t stop;
  wint_t ch;

  /* initialize the graphics */
  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  stop = AVT_FALSE;
  ch = get_character (fd);
  while (ch != WEOF && !stop)
    {
      wchar_t c = ch;

      stop = avt_put_character (c);
      ch = get_character (fd);
    }

  /* close file descriptor */
  if (close (fd) == -1 && errno != EAGAIN)
    warning_msg ("close", strerror (errno));

  /* release keyhandler */
  avt_register_keyhandler (NULL);
  prg_input = -1;

  return 0;
}

static void
not_available (void)
{
  avt_clear ();
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

static void
not_yet_implemented (void)
{
  avt_clear ();
  avt_bell ();
  avt_say_mb (strerror (ENOSYS));

  if (avt_wait_button () != 0)
    quit (EXIT_SUCCESS);
}

static void
ask_file (avt_bool_t execute)
{
#ifdef NO_PTY
  if (execute)
    {
      not_available ();
      return;
    }
#endif

  avt_clear ();
  avt_set_text_delay (0);

  /* show directory and prompt */
  {
    char dirname[255];

    if (getcwd (dirname, sizeof (dirname)) != NULL)
      avt_say_mb (dirname);
    avt_say_mb ("> ");
  }

  {
    char filename[255];

    if (avt_ask_mb (filename, sizeof (filename)) != 0)
      quit (EXIT_SUCCESS);

    avt_clear ();
    avt_set_text_delay (default_delay);
    if (filename[0] != '\0')
      {
	int fd, status;

	if (execute)
	  {
	    fd = execute_process (filename);
	    if (fd > -1)
	      process_subprogram (fd);
	  }
	else			/* not execute */
	  {
	    fd = openfile (filename);
	    if (fd > -1)
	      process_file (fd);
	  }

	/* ignore file errors */
	status = avt_get_status ();
	if (status == AVT_ERROR)
	  quit (EXIT_FAILURE);	/* warning already printed */

	if (status == AVT_NORMAL)
	  if (avt_wait_button ())
	    quit (EXIT_SUCCESS);

	/* reset quit-request */
	avt_set_status (AVT_NORMAL);
      }
  }
}

#ifdef NO_MANPAGES

static void
ask_manpage (void)
{
  not_available ();
}

#else /* not NO_MANPAGES */

static void
ask_manpage (void)
{
  char manpage[AVT_LINELENGTH];

  avt_clear ();
  avt_set_text_delay (0);

  avt_say (L"Manpage> ");

  if (avt_ask_mb (manpage, sizeof (manpage)) != 0)
    quit (EXIT_SUCCESS);

  avt_clear ();
  avt_set_text_delay (default_delay);
  if (manpage[0] != '\0')
    {
      char command[255];
      int fd, status;

      /* -T is not supported on FreeBSD */
      strcpy (command, "man -t ");
      strcat (command, manpage);

      /* GROFF assumed! */
      putenv ("GROFF_TYPESETTER=latin1");
      putenv ("MANWIDTH=80");

      /* temporary settings */
      set_encoding ("ISO-8859-1");

      /* ignore file errors */
      fd = execute_process (command);
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

#endif /* not NO_MANPAGES */


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

  set_encoding (default_encoding);
  avt_set_text_delay (default_delay);

  if (avt_wait_button () != 0)
    quit (EXIT_SUCCESS);
}

static void
menu (void)
{
  wchar_t ch;

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
      avt_set_text_delay (0);
      avt_say (L"AKFAvatar\n");
      avt_say (L"=========\n\n");

      switch (language)
	{
	case DEUTSCH:
	  avt_say (L"1) ein Demo oder eine Text-Datei anzeigen\n");
	  avt_say (L"2) eine Hilfeseite (Manpage) anzeigen\n");
	  avt_say (L"3) Shell starten\n");
	  avt_say (L"4) Homepage des Projektes aufrufen\n");
	  avt_say (L"5) Programm-Infos\n");
	  avt_say (L"0) beenden\n");
	  break;

	case ENGLISH:
	default:
	  avt_say (L"1) show a demo or textfile\n");
	  avt_say (L"2) show a manpage\n");
	  avt_say (L"3) run a shell\n");
	  avt_say (L"4) website\n");
	  avt_say (L"5) show info about the program\n");
	  avt_say (L"0) exit\n");
	}

      avt_set_text_delay (default_delay);

      if (avt_get_key (&ch))
	quit (EXIT_SUCCESS);

      switch (ch)
	{
	case L'1':		/* show a demo or textfile */
	  ask_file (AVT_FALSE);
	  break;

	case L'2':		/* show a manpage */
	  ask_manpage ();
	  break;

	case L'3':		/* run a shell */
	  {
	    int fd;

	    avt_clear ();
	    avt_set_text_delay (0);
	    fd = execute_process (NULL);
	    if (fd > -1)
	      process_subprogram (fd);
	  }
	  break;

	case L'4':		/* website */
	  open_homepage ();
	  break;

	case L'5':		/* show info about the program */
	  about_avatarsay ();
	  break;

	case L'0':		/* exit */
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
  if (strncmp (locale_info, "German", 6) == 0)
    language = DEUTSCH;
  else if (strncmp (locale_info, "german", 6) == 0)
    language = DEUTSCH;
  else if (strncmp (locale_info, "English", 7) == 0)
    language = ENGLISH;
  else if (strncmp (locale_info, "english", 7) == 0)
    language = ENGLISH;
  else if (strncmp (locale_info, "de", 2) == 0)
    language = DEUTSCH;
  else if (strncmp (locale_info, "en", 2) == 0)
    language = ENGLISH;
}

int
main (int argc, char *argv[])
{
  mode = AVT_AUTOMODE;
  loop = AVT_TRUE;
  default_delay = AVT_DEFAULT_TEXT_DELAY;
  prg_input = -1;
  strcpy (default_encoding, "ISO-8859-1");

  init_language_info ();
  checkenvironment ();

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

  checkoptions (argc, argv);

  /* no input files? -> menu */
  if (optind >= argc)
    menu ();

  /* handle files given as parameters */
  do
    {
      int i;

      if (initialized && !popup)
	move_in ();

      for (i = optind; i < argc; i++)
	{
	  int status = 0;
	  int fd = -1;

	  set_encoding (default_encoding);

#ifndef NO_FIFO
	  if (say_pipe)
	    {
	      if (mkfifo (argv[i], 0660) < 0)
		error_msg ("mkfifo", strerror (errno));
	    }
#endif /* not NO_FIFO */

	  if (executable)
	    {
	      fd = execute_process (argv[i]);
	      if (fd > -1)
		status = process_subprogram (fd);
	    }
	  else			/* not executable */
	    {
	      fd = openfile (argv[i]);
	      if (fd > -1)
		status = process_file (fd);
	    }

	  if (say_pipe)
	    if (remove (argv[i]) == -1)
	      warning_msg ("remove", strerror (errno));

	  if (status <= -1)
	    error_msg ("error opening file", argv[i]);
	  else if (status == 1)
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

      if (initialized)
	move_out ();
    }
  while (loop);

  quit (EXIT_SUCCESS);

  /* never executed, but kept in the code */
  puts ("$Id: avatarsay.c,v 2.56 2008-01-13 09:27:44 akf Exp $");

  return EXIT_SUCCESS;
}
