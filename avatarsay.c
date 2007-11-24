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

/* $Id: avatarsay.c,v 2.33 2007-11-24 20:59:05 akf Exp $ */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "version.h"
#include "akfavatar.h"
#include <limits.h>		/* for PATH_MAX */
#include <wchar.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <locale.h>
#include <getopt.h>

#ifdef __WIN32__
#  include <windows.h>
#  ifdef __MINGW32__
#    define NO_FIFO 1
#    define NO_FORK 1
#    define NO_LANGINFO 1
#  endif
#endif

#ifndef NO_LANGINFO
#  include <langinfo.h>
#endif

#ifndef NO_FIFO
#  include <sys/stat.h>
#endif

#define PRGNAME "avatarsay"

/* size for input buffer - not too small, please */
/* .encoding must be in first buffer */
#define INBUFSIZE 10240

/* maximum size for path */
/* should fit into stack */
#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

/* encoding of the input files */
/* supported in SDL: ASCII, ISO-8859-1, UTF-8, UTF-16, UTF-32 */
/* ISO-8859-1 is a sane default */
static char encoding[80];

/* if rawmode is set, then don't interpret any commands or comments */
/* rawmode can be activated with the options -r or --raw */
static int rawmode;

/* popup-mode? */
static int popup;

/* stop the program? */
static int stop;

/* 
 * ignore end of file conditions
 * this should be used, when the input doesn't come from a file
 */
static int ignore_eof;

/* create a fifo for what to say? */
static int say_pipe;

/* execute file? */
static int executable;

/* whether to run in a window, or in fullscreen mode */
/* the mode can be set by -f, --fullscreen or -w, --window */
/* or with --fullfullscreen or -F */
static int mode;

/* is the avatar initialized? (0|1) */
/* for delaying the initialization until it is clear that we actually 
   have data to show */
static int initialized;

/* was the file already checked for an encoding? */
static int encoding_checked;

/* encoding given on command line */
static int given_encoding;

/* play it in an endless loop? (0/1) */
/* deactivated, when input comes from stdin */
/* can be deactivated by -1, --once */
static int loop;

/* where to find imagefiles */
static char datadir[512];

/* for loading an avt_image */
static avt_image_t *avt_image;

/* for loading sound files */
static avt_audio_t *sound;


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
  puts (" -E, --encoding=enc      input data is encoded in encoding \"enc\"");
  puts (" -l, --latin1            input data is encoded in Latin-1");
  puts (" -u, --utf-8             input data is encoded in UTF-8");
  puts (" -1, --once              run only once (don't loop)");
  puts (" -p, --popup             popup, ie. don't move the avatar in");
  puts (" -r, --raw               output raw text"
	" (don't handle any commands)");
  puts (" -i, --ignoreeof         ignore end of file conditions "
	"(input is not a file)");
#ifdef NO_FIFO
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
    error_msg ("iconv error", avt_get_error ());
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
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "hvfFw1risE:lupe",
		       long_options, &option_index);

      /* end of the options */
      if (c == -1)
	break;

      switch (c)
	{
	/* long-option has set a flag, nothing to do here */
	case 0:
	  break;

        /* --help */
	case 'h':
	  help (argv[0]);
	  break;

        /* --version */
	case 'v':
	  showversion ();
	  break;

        /* --fullscreen */
	case 'f':
	  mode = FULLSCREEN;
	  break;

        /* --fullfullscreen */
	case 'F':
	  mode = FULLSCREENNOSWITCH;
	  break;

        /* --window */
	case 'w':
	  mode = WINDOW;
	  break;

         /* --once */
	case '1':
	  loop = 0;
	  break;

        /* --raw */
	case 'r':
	  rawmode = 1;
	  break;

        /* --ignoreeof */
	case 'i':
	  ignore_eof = 1;
	  break;

        /* --saypipe */
	case 's':
#ifdef NO_FIFO
	  error_msg ("pipes not supported on this system", NULL);
#else
	  say_pipe = 1;
	  loop = 0;
	  ignore_eof = 1;
	  /* autodetecting the encoding doesn't work with FIFOs */
	  given_encoding = 1;
#endif /* ! NO_FIFO */
	  break;

        /* --encoding */
	case 'E':
	  strncpy (encoding, optarg, sizeof (encoding));
	  given_encoding = 1;
	  break;

        /* --latin1 */
	case 'l':
	  strcpy (encoding, "ISO-8859-1");
	  given_encoding = 1;
	  break;

        /* --utf-8, --utf8, --u8 */
	case 'u':
	  strcpy (encoding, "UTF-8");
	  given_encoding = 1;
	  break;

        /* --popup */
	case 'p':
	  popup = 1;
	  break;

	case 'e':
	  executable = 1;
	  break;

        /* unsupported option */
	case '?':
	  /* getopt_long already printed an error message to stderr */
	  help (argv[0]);
	  break;

        /* declared option, but not handled here */
        /* should never happen */
	default:
	  error_msg ("internal error", "option not supported");
	}
    }

  /* no input files? -> print help */
  if (optind >= argc)
    help (argv[0]);
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
      error_msg ("error while loading the AVATARIMAGE", avt_get_error ());
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
    error_msg ("cannot initialize graphics", avt_get_error ());

  if (avt_initialize_audio ())
    notice_msg ("cannot initialize audio", avt_get_error ());

  initialized = 1;
}

/* fills filepath with datadir and the converted contend of fn */
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
    error_msg ("formatting error for \".backgroundcolor\"", NULL);
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

  if (avt_play_audio (sound, 0))
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
  wchar_t line[LINELENGTH];

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
static int
iscommand (wchar_t * s)
{
  if (rawmode)
    return 0;

  /* 
   * a stripline begins with at least 3 dashes
   * or it begins with \f 
   * the rest of the line is ignored
   */
  if (wcsncmp (s, L"---", 3) == 0 || s[0] == L'\f')
    {
      if (initialized)
	if (avt_flip_page ())
	  stop = 1;
      return 1;
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
	  return 1;
	}

      if (wcsncmp (s, L".avatarimage ", 13) == 0)
	{
	  if (!initialized)
	    handle_avatarimage_command (s);

	  return 1;
	}

      /* the encoding is checked in check_encoding */
      /* so ignore it here */
      if (wcsncmp (s, L".encoding ", 10) == 0)
	return 1;

      if (wcsncmp (s, L".backgroundcolor ", 17) == 0)
	{
	  if (!initialized)
	    handle_backgoundcolor_command (s);

	  return 1;
	}

      /* default - for most languages */
      if (wcscmp (s, L".left-to-right") == 0)
	{
	  avt_text_direction (LEFT_TO_RIGHT);
	  return 1;
	}

      /* currently only hebrew/yiddish supported */
      if (wcscmp (s, L".right-to-left") == 0)
	{
	  avt_text_direction (RIGHT_TO_LEFT);
	  return 1;
	}

      /* new page - same as \f or stripline */
      if (wcscmp (s, L".flip") == 0)
	{
	  if (initialized)
	    if (avt_flip_page ())
	      stop = 1;
	  return 1;
	}

      /* clear ballon - don't wait */
      if (wcscmp (s, L".clear") == 0)
	{
	  if (initialized)
	    avt_clear ();
	  return 1;
	}

      /* longer intermezzo */
      if (wcscmp (s, L".pause") == 0)
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
      if (wcsncmp (s, L".image ", 7) == 0)
	{
	  handle_image_command (s);
	  return 1;
	}

      /* play sound */
      if (wcsncmp (s, L".audio ", 7) == 0)
	{
	  handle_audio_command (s);
	  return 1;
	}

      /* wait until sound ends */
      if (wcscmp (s, L".waitaudio") == 0)
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
      if (wcscmp (s, L".effectpause") == 0)
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
      if (wcsncmp (s, L".back ", 6) == 0)
	{
	  handle_back_command (s);
	  return 1;
	}

      if (wcscmp (s, L".read") == 0)
	{
	  handle_read_command ();
	  return 1;
	}

      if (wcscmp (s, L".end") == 0)
	{
	  if (initialized)
	    avt_move_out ();
	  stop = 1;
	  return 1;
	}

      if (wcscmp (s, L".stop") == 0)
	{
	  /* doesn't matter whether it's initialized */
	  stop = 1;
	  return 1;
	}

      /* silently ignore unknown commands */
      return 1;
    }


  if (s[0] == L'#')
    return 1;

  return 0;
}

/* check for byte order mark (BOM) U+FEFF and remove it */
static void
check_encoding (char *buf, int *size)
{
  char *enc;

  encoding_checked = 1;

  /* check for command .encoding */
  enc = strstr (buf, ".encoding ");

  /*
   * if .encoding is found and it is either at the start of the buffer 
   * or the previous character is a \n then set_encoding
   * and don't check anything else anymore
   */
  if (enc != NULL && (enc == buf || *(enc - 1) == '\n'))
    {
      if (sscanf (enc, ".encoding %79s", (char *) &encoding) <= 0)
	warning_msg ("warning", "cannot read the \".encoding\" line.");
      else
	{
	  set_encoding (encoding);
	  return;
	}
    }


  /* check for byte order marks (BOM) */

  /* UTF-8 BOM (as set by Windows notepad) */
  if (*buf == '\xEF' && *(buf + 1) == '\xBB' && *(buf + 2) == '\xBF')
    {
      strcpy (encoding, "UTF-8");
      set_encoding (encoding);
      memmove (buf, buf + 3, *size - 3);
      *size -= 3;
      return;
    }

  /* check 32 Bit BOMs before 16 Bit ones, to avoid confusion! */

  /* UTF-32BE BOM */
  if (*buf == '\x00' && *(buf + 1) == '\x00'
      && *(buf + 2) == '\xFE' && *(buf + 3) == '\xFF')
    {
      strcpy (encoding, "UTF-32BE");
      set_encoding (encoding);
      memmove (buf, buf + 4, *size - 4);
      *size -= 4;
      return;
    }

  /* UTF-32LE BOM */
  if (*buf == '\xFF' && *(buf + 1) == '\xFE'
      && *(buf + 2) == '\x00' && *(buf + 3) == '\x00')
    {
      strcpy (encoding, "UTF-32LE");
      set_encoding (encoding);
      memmove (buf, buf + 4, *size - 4);
      *size -= 4;
      return;
    }

  /* UTF-16BE BOM */
  if (*buf == '\xFE' && *(buf + 1) == '\xFF')
    {
      strcpy (encoding, "UTF-16BE");
      set_encoding (encoding);
      memmove (buf, buf + 2, *size - 2);
      *size -= 2;
      return;
    }

  /* UTF-16LE BOM */
  if (*buf == '\xFF' && *(buf + 1) == '\xFE')
    {
      strcpy (encoding, "UTF-16LE");
      set_encoding (encoding);
      memmove (buf, buf + 2, *size - 2);
      *size -= 2;
      return;
    }

  /* other heuristics for Unicode */

  if (*buf == '\x00' && *(buf + 1) == '\x00')
    {
      strcpy (encoding, "UTF-32BE");
      set_encoding (encoding);
      return;
    }

  if (*(buf + 2) == '\x00' && *(buf + 3) == '\x00')
    {
      strcpy (encoding, "UTF-32LE");
      set_encoding (encoding);
      return;
    }

  if (*buf == '\x00')
    {
      strcpy (encoding, "UTF-16BE");
      set_encoding (encoding);
      return;
    }

  if (*(buf + 1) == '\x00')
    {
      strcpy (encoding, "UTF-16LE");
      set_encoding (encoding);
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
  static int wcbuf_pos = 0;
  static int wcbuf_len = 0;
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

      /* no data in FIFO */
      while (filebuf_end == -1 && errno == EAGAIN && !stop)
	{
	  if (avt_update ())
	    stop = 1;
	  filebuf_end = read (fd, &filebuf, sizeof (filebuf));
	}

      if (filebuf_end == -1)
	error_msg ("error while reading from file", strerror (errno));

      if (!encoding_checked && !given_encoding)
	check_encoding (filebuf, &filebuf_end);

      wcbuf_len = avt_mb_decode (&wcbuf, (char *) &filebuf, filebuf_end);
      wcbuf_pos = 0;
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

static int
execute_process (const char *fname)
{
#ifndef NO_FORK
  pid_t childpid;
  int fdpair[2];

  if (pipe (fdpair) == -1)
    error_msg ("error creating pipe", strerror (errno));

  childpid = fork ();

  if (childpid == -1)
    error_msg ("error while forking", strerror (errno));

  /* is it the child process? */
  if (childpid == 0)
    {
      /* child closes input part of pipe */
      close (fdpair[0]);

      /* redirect stdout to pipe */
      if (dup2 (fdpair[1], STDOUT_FILENO) == -1)
	error_msg ("error with dup2", strerror (errno));

      /* redirect sterr to pipe */
      if (dup2 (fdpair[1], STDERR_FILENO) == -1)
	error_msg ("error with dup2", strerror (errno));

      close (fdpair[1]);

      /* It's a very very dumb terminal */
      putenv ("TERM=dumb");

      /* execute the command */
      execlp (fname, fname, NULL);

      /* in case of an error, we can not do much */
      /* stdout and stderr are broken by now */
      quit (EXIT_FAILURE);
    }

  /* parent closes output part of pipe */
  close (fdpair[1]);

  /* use input part of pipe */
  return fdpair[0];
#endif /* ! NO_FORK */
}

/* opens the file */
static int
openfile (const char *fname)
{
  int fd = -1;

  if (strcmp (fname, "-") == 0)
    fd = STDIN_FILENO;		/* stdin */
  else
#ifndef NO_FIFO
  if (say_pipe)
    {
      if (mkfifo (fname, 0600) == -1)
	error_msg ("mkfifo", fname);
      fd = open (fname, O_RDONLY | O_NONBLOCK);
    }
  else
#endif /* ! NO_FIFO */
#ifndef NO_FORK
  if (executable)
    fd = execute_process (fname);
  else
#endif /* ! NO_FORK */
    fd = open (fname, O_RDONLY);	/* regular file */

  /* error */
  if (fd < 0)
    error_msg ("error opening file for reading", strerror (errno));

  return fd;
}

/* shows content of file / other input */
static int
processfile (const char *fname)
{
  wchar_t *line = NULL;
  size_t line_size = 0;
  ssize_t nread = 0;
  int fd;

  fd = openfile (fname);

  line_size = 1024 * sizeof (wchar_t);
  line = (wchar_t *) malloc (line_size);

  encoding_checked = 0;

  if (!rawmode)
    do				/* skip empty lines at the beginning */
      {
	nread = getwline (fd, line, line_size);

	/* simulate empty lines when ignore_eof is set */
	if (ignore_eof && nread == 0)
	  {
	    wcscpy (line, L"\n");
	    nread = 1;
	    if (avt_update ())
	      stop = 1;
	  }
      }
    while (nread != 0
	   && (wcscmp (line, L"\n") == 0
	       || wcscmp (line, L"\r\n") == 0 || iscommand (line)) && !stop);

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
		  stop = 1;
		  break;
		}
	      else
		nread = getwline (fd, line, line_size);
	    }
	}

      if (nread != 0 && !stop && !iscommand (line))
	{
	  process_line_end (line, &nread);
	  if (avt_say_len (line, nread))
	    stop = 1;
	}
    }

  if (line)
    {
      free (line);
      line_size = 0;
    }

  if (close (fd) == -1 && errno != EAGAIN)
    error_msg ("error closing the file", strerror (errno));

#ifndef NO_FIFO
  if (say_pipe)
    if (remove (fname) == -1)
      warning_msg ("problem removing FIFO", strerror (errno));
#endif /* ! NO_FIFO */

  if (avt_get_status () == AVATARERROR)
    {
      stop = 1;
      warning_msg ("warning", avt_get_error ());
    }

  return stop;
}

int
main (int argc, char *argv[])
{
  mode = AUTOMODE;
  loop = 1;
  strcpy (encoding, "ISO-8859-1");

  setlocale (LC_ALL, "");

  checkenvironment ();

  /* get system encoding */
#ifndef NO_LANGINFO
  strncpy (encoding, nl_langinfo (CODESET), sizeof (encoding));
#endif

  checkoptions (argc, argv);

  set_encoding (encoding);

  do
    {
      int i;

      if (initialized && !popup)
	move_in ();

      for (i = optind; i < argc; i++)
	{
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

      if (initialized)
	move_out ();
    }
  while (loop);

  quit (EXIT_SUCCESS);
  return EXIT_SUCCESS;
}
