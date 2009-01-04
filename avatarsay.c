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

/* $Id: avatarsay.c,v 2.255 2009-01-04 16:09:09 akf Exp $ */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "version.h"
#include "akfavatar.h"
#include "arch.h"
#include "avtmsg.h"
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
#  include "avtterm.h"
#  include <sys/wait.h>
#  include <pwd.h>
#else
#  define avtterm_nocolor(ignore)	/* empty */
#  define avtterm_update_size(void)	/* empty */
#endif

/* some systems don't know O_NONBLOCK */
#ifndef O_NONBLOCK
#  define O_NONBLOCK 0
#endif

#define HOMEPAGE "http://akfavatar.nongnu.org/"
#define BUGMAIL "bug-akfavatar@akfoerster.de"

/* size for input buffer - not too small, please */
/* .encoding must be in first buffer */
#define INBUFSIZE 1024

/* maximum size for path */
/* must fit into stack */
#if defined(PATH_MAX) && PATH_MAX < 4096
#  define PATH_LENGTH PATH_MAX
#else
#  define PATH_LENGTH 4096
#endif

/* is there really no clean way to find the executable? */
#ifndef PREFIX
#  define PREFIX "/usr/local/bin"
#endif

#define BYTE_ORDER_MARK L'\xfeff'

#define default_background_color(ignore) \
       avt_set_background_color (0xE0, 0xD5, 0xC5)

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

/* starting directory (strdup) */
static char *start_dir;

/* default encoding - either system encoding or given per parameters */
/* supported in SDL: ASCII, ISO-8859-1, UTF-8, UTF-16, UTF-32 */
char default_encoding[80];

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
static char *from_archive;	/* archive name or NULL */
static size_t script_bytes_left;	/* how many bytes may still be read */

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

/* for setting the title */
/* only used, when the title is set before it is initialized */
static char *supposed_title;

/* where to find imagefiles */
static char datadir[512];

/* for loading an avt_image */
static avt_image_t *avt_image;

/* name of the image file */
static char *avt_image_name;
static avt_bool_t avatar_changed;
static avt_bool_t moved_in;

/* for loading sound files */
static avt_audio_t *sound;

/* text-buffer */
static int wcbuf_pos = 0;
static int wcbuf_len = 0;

/* was the background color changed? */
static avt_bool_t background_color_changed;

/* language (of current locale) */
enum language_t
{ ENGLISH, DEUTSCH };
static enum language_t language;

static void
quit (void)
{
  if (start_dir)
    free (start_dir);

  if (initialized)
    {
      if (sound)
	avt_free_audio (sound);

      avt_quit ();
    }
}

static void
showversion (void)
{
  switch (language)
    {
    case DEUTSCH:
      info_msg (version_info_de);
      break;

    case ENGLISH:
    default:
      info_msg (version_info_en);
    }

  exit (EXIT_SUCCESS);
}

#ifndef __WIN32__

static void
help (void)
{
  printf ("\nUsage: %s [Options]\n", program_name);
  printf ("  or:  %s [Options] [textfiles | demo-files]\n", program_name);
  printf ("  or:  %s [Options] --execute <program> [program options]\n\n",
	  program_name);
  puts
    ("A fancy text-terminal, text-viewer and scripting language for making demos.\n");
  puts ("If textfile is - then read from stdin and don't loop.\n");
  puts ("Options:");
  puts (" -h, --help              show this help");
  puts (" -v, --version           show the version");
#ifdef NO_PTY
  puts (" -t, --terminal          not supported on this system");
  puts (" -x, --execute           not supported on this system");
#else
  puts (" -t, --terminal          terminal mode (run a shell in balloon)");
  puts (" -x, --execute           execute program in balloon");
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

#else /* Windows or ReactOS */

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
  avt_set_balloon_size (3, 45);
  avt_clear ();
  avt_set_text_delay (default_delay);
  avt_bell ();

  switch (language)
    {
    case DEUTSCH:
      avt_say (L"Funktion auf diesem System nicht verfügbar...\n"
	       L"Vollständige Unterstützung steht zum Beispiel\n"
	       L"unter GNU/Linux zur Verfügung.");
      break;

    case ENGLISH:
    default:
      avt_say (L"function not available on this system...\n"
	       L"A fully supported system is for example\nGNU/Linux.");
    }

  avt_wait_button ();
  avt_set_status (AVT_NORMAL);
}

#endif /* not Windows or ReactOS */

#ifdef __WIN32__

void get_user_home (char *home_dir, size_t size);

#else /* not __WIN32__ */

/* get user's home direcory */
void
get_user_home (char *home_dir, size_t size)
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

  strncpy (home_dir, home, size);
  if (size > 0)
    home_dir[size - 1] = '\0';
}

#endif /* not __WIN32__ */

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
	exit (EXIT_SUCCESS);
      if (avt_wait (2000))
	exit (EXIT_SUCCESS);
      moved_in = AVT_TRUE;
    }
}

static void
move_out (void)
{
  if (initialized)
    {
      if (avt_move_out ())
	exit (EXIT_SUCCESS);
    }

  moved_in = AVT_FALSE;
}

static void
initialize (void)
{
  if (!avt_image)
    avt_image = avt_default ();

  /* 
   * if supposed_title is NULL then "AKFAvatar" is used
   * attention: the icontitle is set to the title, when NULL
   */
  if (avt_initialize (supposed_title, "AKFAvatar", avt_image, window_mode))
    switch (language)
      {
      case DEUTSCH:
	error_msg ("kann Grafik nicht initialisieren", avt_get_error ());
	break;

      case ENGLISH:
      default:
	error_msg ("cannot initialize graphics", avt_get_error ());
      }

  /* we don't need it anymore */
  if (supposed_title)
    {
      free (supposed_title);
      supposed_title = NULL;
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

  background_color_changed = AVT_FALSE;
  avatar_changed = AVT_FALSE;
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

      c = getopt_long (argc, argv, "+hvfFw1risE:luptxnb",
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
	  executable = AVT_TRUE;
	  default_delay = 0;
	  loop = AVT_FALSE;
	  break;

	case 'n':		/* --no-delay */
	  default_delay = 0;
	  avt_set_text_delay (0);
	  break;

	case 'b':		/* --nocolor */
	  avtterm_nocolor (AVT_TRUE);
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
      avt_set_scroll_mode (-1);
      avt_set_text_delay (0);
      avt_set_balloon_size (8, 28);
      avt_say_mb (" ___________\n< AKFAvatar >\n -----------\n    "
		  "    \\   ^__^\n         \\  (oo)\\_____"
		  "__\n            (__)\\       )\\/\\\n     "
		  "           ||----w |\n                ||     ||");
      avt_wait_button ();
      exit (EXIT_SUCCESS);
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

  avatar_changed = AVT_FALSE;
}

/* restore avatar from avt_image name */
static void
restore_avatar_image (void)
{
  avt_image_t *img;

  if (avt_image_name)
    img = avt_import_image_file (avt_image_name);
  else
    img = avt_default ();

  if (avt_change_avatar_image (img))
    error_msg (avt_image_name, "cannot load file");

  avatar_changed = AVT_FALSE;
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

/* fills filepath with the converted content of fn */
/* (datadir no longer supported here!) */
static void
get_data_file (const wchar_t * fn, char filepath[])
{
  size_t result;

  /* remove leading whitespace */
  while (*fn == L' ' || *fn == L'\t')
    fn++;

  result = wcstombs (filepath, fn, PATH_LENGTH - 1);

  if (result == (size_t) (-1))
    error_msg ("wcstombs", strerror (errno));
}

/* read in a textfile */
static char *
read_text_file (const char *f)
{
  int fd;
  char *buf;
  size_t size, capacity;
  ssize_t nread;

  buf = NULL;
  size = capacity = 0;
  nread = 0;

  fd = open (f, O_RDONLY);

  if (fd > -1)
    {
      do
	{
	  /* do we need more capacity? */
	  if (size >= capacity)
	    {
	      char *nbuf;

	      capacity += BUFSIZ;
	      nbuf = (char *) realloc (buf, capacity + 4);

	      if (nbuf)
		buf = nbuf;
	      else
		break;
	    }

	  nread = read (fd, buf + size, capacity - size);
	  if (nread > 0)
	    size += nread;
	}
      while (nread > 0);

      close (fd);

      if (buf)
	{
	  char *nbuf;

	  /* I terminate with 4 zeros, in case UTF-32 is used */
	  memset (buf + size, '\0', 4);
	  nbuf = (char *) realloc (buf, size + 4);
	  if (nbuf)
	    buf = nbuf;
	}
    }

  return buf;
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

/* 
 * check for the command [encoding ...]
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
    /* check for command [encoding ...] */
    enc = strstr (buf, "[encoding ");

    /*
     * if .encoding is found and it is either at the start of the buffer 
     * or the previous character is a \n then set_encoding
     * and don't check anything else anymore
     */
    if (enc != NULL && (enc == buf || *(enc - 1) == '\n'))
      {
	if (sscanf (enc, "[ encoding %79s ]", (char *) &temp) <= 0)
	  warning_msg ("[encoding]", NULL);
	else
	  {
	    char *closing_bracket;

	    closing_bracket = strrchr (temp, ']');
	    if (closing_bracket != NULL)
	      *closing_bracket = '\0';
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

/* @@@ */
static wint_t
get_character (int fd)
{
  static wchar_t *wcbuf = NULL;
  wchar_t ch;

  if (wcbuf_pos >= wcbuf_len)
    {
      char filebuf[INBUFSIZE];
      ssize_t nread;

      if (wcbuf)
	{
	  avt_free (wcbuf);
	  wcbuf = NULL;
	}

      /* reserve one byte for a terminator */
      if (from_archive && script_bytes_left < sizeof (filebuf) - 1)
	nread = read (fd, &filebuf, script_bytes_left);
      else
	nread = read (fd, &filebuf, sizeof (filebuf) - 1);

      if (nread > 0)
	script_bytes_left -= nread;

      /* waiting for data */
      /* should never happen on an archive */
      if (nread == -1 && errno == EAGAIN)
	{
	  while (nread == -1 && errno == EAGAIN
		 && avt_update () == AVT_NORMAL)
	    nread = read (fd, &filebuf, sizeof (filebuf) - 1);
	}

      if (nread == -1)
	{
	  if (read_error_is_eof)
	    wcbuf_len = -1;
	  else
	    error_msg ("error while reading from file", strerror (errno));
	}
      else			/* nread != -1 */
	{
	  if (!encoding_checked && !given_encoding)
	    {
	      filebuf[nread] = '\0';	/* terminate */
	      check_encoding (filebuf);
	    }

	  wcbuf_len = avt_mb_decode (&wcbuf, (char *) &filebuf, nread);
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

/* report failure to get member from archive */
static void
archive_failure (const char *name)
{
  if (strlen (name) > 15)
    error_msg (name, "names of archive members may not be "
	       "longer then 15 characters");
  else
    error_msg (name, "not found in archive");
}

static void
handle_title_command (const wchar_t * s)
{
  char newtitle[255];

  /* remove leading whitespace */
  while (*s == L' ' || *s == L'\t')
    s++;

  /* FIXME: The title in SDL is always UTF-8 encoded,
     independent from the systems encoding */
  if (wcstombs (newtitle, s, sizeof (newtitle)) == (size_t) (-1))
    error_msg ("wcstombs", strerror (errno));

  if (newtitle[0])
    {
      char title[300];

      strcpy (title, "AKFAvatar: ");
      strcat (title, newtitle);

      if (initialized)
	avt_set_title (title, NULL);
      else
	supposed_title = strdup (title);
    }
  else				/* empty */
    {
      if (initialized)
	avt_set_title ("AKFAvatar", "AKFAvatar");
    }
}

/* errors are silently ignored */
static void
handle_image_command (const wchar_t * s, int *stop)
{
  char filepath[PATH_LENGTH];

  get_data_file (s, filepath);

  if (!initialized)
    initialize ();
  else if (avt_wait (AVT_DEFAULT_FLIP_PAGE_DELAY))
    {
      *stop = 1;
      return;
    }

  if (from_archive)
    {
      void *img;
      size_t size = 0;

      if (arch_get_data (from_archive, filepath, &img, &size))
	{
	  if (!avt_show_image_data (img, size))
	    avt_wait (7000);
	  free (img);
	  if (avt_get_status ())
	    *stop = 1;
	}
      else
	archive_failure (filepath);
    }
  else				/* not from_archive */
    {
      if (!avt_show_image_file (filepath))
	if (avt_wait (7000))
	  *stop = 1;
    }
}

static void
handle_credits_command (const wchar_t * s, int *stop)
{
  char filepath[PATH_LENGTH];

  get_data_file (s, filepath);

  if (!initialized)
    initialize ();
  else
    {
      if (avt_wait (AVT_DEFAULT_FLIP_PAGE_DELAY))
	{
	  *stop = 1;
	  return;
	}
      move_out ();
    }

  if (from_archive)
    {
      void *text;
      size_t size = 0;

      if (arch_get_data (from_archive, filepath, &text, &size))
	{
	  if (avt_credits_mb ((const char *) text, AVT_TRUE))
	    *stop = 1;
	  free (text);
	}
      else
	archive_failure (filepath);
    }
  else				/* not from_archive */
    {
      char *text;

      text = read_text_file (filepath);
      if (avt_credits_mb (text, AVT_TRUE))
	*stop = 1;
      free (text);
    }
}

static void
change_avatar_image (avt_image_t * newavatar)
{
  if (initialized)
    {
      avt_change_avatar_image (newavatar);
      avatar_changed = AVT_TRUE;
      avtterm_update_size ();
    }
  else				/* save for initialize @@@ ? */
    {
      if (avt_image)
	free (avt_image);
      avt_image = newavatar;
    }
}

static void
handle_avatarimage_command (const wchar_t * s)
{
  char filepath[PATH_LENGTH];
  void *img;
  avt_image_t *newavatar = NULL;
  size_t size = 0;

  /* no name? - restore default image */
  if (s[0] == L'\0')
    {
      restore_avatar_image ();
      return;
    }

  get_data_file (s, filepath);

  if (from_archive)
    {
      if (arch_get_data (from_archive, filepath, &img, &size))
	{
	  if (!(newavatar = avt_import_image_data (img, size)))
	    warning_msg ("warning", avt_get_error ());
	  free (img);
	}
      else
	archive_failure (filepath);
    }
  else				/* not from_archive */
    {
      if (!(newavatar = avt_import_image_file (filepath)))
	warning_msg ("warning", avt_get_error ());
    }

  if (newavatar)
    change_avatar_image (newavatar);
}

static void
handle_backgoundcolor_command (const wchar_t * s)
{
  unsigned int red, green, blue;

  if (swscanf (s, L"#%2x%2x%2x", &red, &green, &blue) == 3)
    {
      avt_set_background_color (red, green, blue);
      background_color_changed = AVT_TRUE;
    }
  else
    error_msg ("[backgroundcolor]", NULL);
}

static void
handle_loadaudio_command (const wchar_t * s)
{
  char filepath[PATH_LENGTH];
  size_t size = 0;
  void *buf = NULL;

  if (sound)
    {
      avt_stop_audio ();
      avt_free_audio (sound);
      sound = NULL;
    }

  get_data_file (s, filepath);

  if (from_archive)
    {
      if (arch_get_data (from_archive, filepath, &buf, &size))
	{
	  sound = avt_load_wave_data (buf, size);
	  free (buf);
	}
      else
	archive_failure (filepath);
    }
  else				/* not from_archive */
    sound = avt_load_wave_file (filepath);

  if (sound == NULL)
    {
      notice_msg ("can not load audio data", avt_get_error ());
      return;
    }
}

static void
handle_playaudio_command (avt_bool_t do_loop)
{
  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (avt_play_audio (sound, do_loop))
    notice_msg ("can not play audio data", avt_get_error ());
}

static void
handle_audio_command (const wchar_t * s, avt_bool_t do_loop)
{
  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  handle_loadaudio_command (s);
  handle_playaudio_command (do_loop);
}

static void
handle_back_command (const wchar_t * s)
{
  int i, value;

  if (!initialized)
    return;

  if (swscanf (s, L"%i", &value) > 0)
    {
      for (i = 0; i < value; i++)
	avt_backspace ();
    }
  else
    avt_backspace ();
}

static void
handle_height_command (const wchar_t * s)
{
  int value;

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (swscanf (s, L"%i", &value) > 0)
    avt_set_balloon_height (value);
  else
    avt_set_balloon_height (0);	/* maximum */

  avtterm_update_size ();
}

static void
handle_width_command (const wchar_t * s)
{
  int value;

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (swscanf (s, L"%i", &value) > 0)
    avt_set_balloon_width (value);
  else
    avt_set_balloon_width (0);	/* maximum */

  avtterm_update_size ();
}

static void
handle_size_command (const wchar_t * s)
{
  int width, height;

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  if (swscanf (s, L"%i , %i", &height, &width) == 2)
    avt_set_balloon_size (height, width);
  else
    avt_set_balloon_size (0, 0);	/* maximum */

  avtterm_update_size ();
}

static void
handle_read_command (void)
{
  wchar_t line[AVT_LINELENGTH];

  if (!initialized)
    initialize ();

  avt_ask (line, sizeof (line));

  /* TODO: not fully implemented yet */
}

/* handle command */
/* also called from avtterm.c */
extern void
avatar_command (wchar_t * s, int *stop)
{
  /* new datadir */
  if (wcsncmp (s, L"datadir ", 8) == 0)
    {
      char directory[PATH_LENGTH];
      get_data_file (s + 8, directory);
      if (chdir (directory))
	warning_msg ("chdir", strerror (errno));
      return;
    }

  if (wcscmp (s, L"avatarimage none") == 0)
    {
      change_avatar_image (NULL);
      return;
    }

  if (wcsncmp (s, L"avatarimage", 11) == 0)
    {
      handle_avatarimage_command (s + 11);
      return;
    }

  /* the encoding is checked in check_encoding() */
  /* so ignore it here */
  if (wcsncmp (s, L"encoding ", 9) == 0)
    return;

  if (wcsncmp (s, L"title", 5) == 0)
    {
      handle_title_command (s + 5);
      return;
    }

  if (wcsncmp (s, L"backgroundcolor ", 16) == 0)
    {
      handle_backgoundcolor_command (s + 16);
      return;
    }

  /* default - for most languages */
  if (wcscmp (s, L"left-to-right") == 0)
    {
      avt_text_direction (AVT_LEFT_TO_RIGHT);
      return;
    }

  /* currently only hebrew/yiddish supported */
  if (wcscmp (s, L"right-to-left") == 0)
    {
      avt_text_direction (AVT_RIGHT_TO_LEFT);
      return;
    }

  /* slow printing on */
  if (wcscmp (s, L"slow on") == 0)
    {
      avt_set_text_delay (AVT_DEFAULT_TEXT_DELAY);
      return;
    }

  /* slow printing off */
  if (wcscmp (s, L"slow off") == 0)
    {
      avt_set_text_delay (0);
      return;
    }

  /* switch scrolling off */
  if (wcscmp (s, L"scrolling off") == 0)
    {
      avt_set_scroll_mode (-1);
      return;
    }

  /* switch scrolling on */
  if (wcscmp (s, L"scrolling on") == 0)
    {
      avt_set_scroll_mode (1);
      return;
    }

  /* change balloon size */
  /* no parameters set the maximum size */
  if (wcsncmp (s, L"size", 4) == 0)
    {
      handle_size_command (s + 4);
      return;
    }

  /* change balloonheight */
  if (wcsncmp (s, L"height", 6) == 0)
    {
      handle_height_command (s + 6);
      return;
    }

  /* change balloonwidth */
  if (wcsncmp (s, L"width", 5) == 0)
    {
      handle_width_command (s + 5);
      return;
    }

  /* new page - same as \f or stripline */
  if (wcscmp (s, L"flip") == 0)
    {
      if (initialized)
	if (avt_flip_page ())
	  *stop = 1;
      return;
    }

  /* clear ballon - don't wait */
  if (wcscmp (s, L"clear") == 0)
    {
      if (initialized)
	avt_clear ();
      return;
    }

  if (wcscmp (s, L"empty") == 0)
    {
      if (initialized)
	avt_clear_screen ();

      moved_in = AVT_FALSE;
      return;
    }

  /* move avatar out */
  if (wcscmp (s, L"move out") == 0)
    {
      if (initialized)
	avt_move_out ();
      return;
    }

  /* move avatar in */
  if (wcscmp (s, L"move in") == 0)
    {
      if (initialized && !moved_in)
	avt_move_in ();
      return;
    }

  /* wait a while or a given time */
  if (wcsncmp (s, L"wait", 4) == 0)
    {
      int value;

      if (swscanf (s + 4, L"%i", &value) > 0)
	avt_wait (value);
      else
	avt_wait (AVT_DEFAULT_FLIP_PAGE_DELAY);
    }

  /* longer intermezzo */
  if (wcscmp (s, L"pause") == 0)
    {
      if (!initialized)
	initialize ();
      else if (avt_wait (AVT_DEFAULT_FLIP_PAGE_DELAY))
	*stop = 1;

      avt_show_avatar ();
      if (avt_wait (4000))
	*stop = 1;
      return;
    }

  /* show image */
  if (wcsncmp (s, L"image ", 6) == 0)
    {
      handle_image_command (s + 6, stop);
      return;
    }

  /* load and play sound */
  if (wcsncmp (s, L"audio ", 6) == 0)
    {
      handle_audio_command (s + 6, AVT_FALSE);
      return;
    }

  /* load and play sound in a loop */
  if (wcsncmp (s, L"audioloop ", 10) == 0)
    {
      handle_audio_command (s + 10, AVT_TRUE);
      return;
    }

  /* load sound */
  if (wcsncmp (s, L"loadaudio ", 10) == 0)
    {
      handle_loadaudio_command (s + 10);
      return;
    }

  /* play sound, loaded by loadaudio */
  if (wcscmp (s, L"playaudio") == 0)
    {
      handle_playaudio_command (AVT_FALSE);
      return;
    }

  /* play sound in a loop, loaded by loadaudio */
  if (wcscmp (s, L"playaudioloop") == 0)
    {
      handle_playaudio_command (AVT_TRUE);
      return;
    }

  /* stop sound */
  if (wcscmp (s, L"stopaudio") == 0)
    {
      avt_stop_audio ();
      return;
    }

  /* wait until sound ends */
  if (wcscmp (s, L"waitaudio") == 0)
    {
      if (initialized)
	if (avt_wait_audio_end ())
	  *stop = 1;
      return;
    }

  /* 
   * pause for effect in a sentence
   * the previous line should end with a backslash
   */
  if (wcscmp (s, L"effectpause") == 0)
    {
      if (initialized)
	if (avt_wait (AVT_DEFAULT_FLIP_PAGE_DELAY))
	  *stop = 1;
      return;
    }

  /* 
   * move back a number of characters
   * the previous line has to end with a backslash!
   */
  if (wcsncmp (s, L"back ", 5) == 0)
    {
      handle_back_command (s + 5);
      return;
    }

  if (wcscmp (s, L"read") == 0)
    {
      handle_read_command ();
      return;
    }

  /* show final credits */
  if (wcsncmp (s, L"credits ", 8) == 0)
    {
      handle_credits_command (s + 8, stop);
      return;
    }

  if (wcscmp (s, L"end") == 0)
    {
      if (initialized)
	avt_move_out ();
      moved_in = AVT_FALSE;
      *stop = 2;
      return;
    }

  if (wcscmp (s, L"stop") == 0)
    {
      /* doesn't matter whether it's initialized */
      *stop = 2;
      return;
    }

  /* silently ignore unknown commands */
}

/* handle commads, including comments */
static avt_bool_t
iscommand (wchar_t * s, int *stop)
{
  wchar_t *p;

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
	  *stop = 1;

      return AVT_TRUE;
    }

  /* ignore lines starting with a '#' */
  if (s[0] == L'#')
    return AVT_TRUE;

  if (s[0] == L'[')
    {
      s++;			/* skip the opening bracket */

      /* 
       * find the closing bracket 
       * and turn it into the end of the string
       * (ie. anything after the closing bracket is ignored)
       */
      p = wcschr (s, L']');
      if (p != NULL)
	*p = L'\0';

      avatar_command (s, stop);
      return AVT_TRUE;
    }

  return AVT_FALSE;
}


/* read a line from AKFAvatar-multi archive file */
/* return width of title */
static int
read_multi_entry (const wchar_t * line, char archive_name[], wchar_t title[])
{
  const wchar_t *s;
  wchar_t *t;
  char *c;
  int count;

  /* get archive name */
  c = archive_name;
  s = line;
  count = 0;
  while (*s && *s != ' ' && *s != '\t' && count < 15)
    {
      *c = (char) *s;
      c++;
      s++;
      count++;
    }
  *c = '\0';

  /* skip spaces */
  while (*s == ' ' || *s == '\t')
    s++;

  /* get title */
  t = title;
  count = 0;
  while (*s && *s != '\r' && *s != '\n' && count < AVT_LINELENGTH)
    {
      *t = *s;
      t++;
      s++;
      count++;
    }
  *t = '\0';

  return count;
}

// @@@
static size_t
multi_menu (int fd)
{
  wchar_t ch;
  int menu_start;
  wchar_t line[256];
  wchar_t title[AVT_LINELENGTH + 1];
  ssize_t nread;
  int entry, width, i;
  char archive_member[10][16];
  wchar_t entry_title[10][AVT_LINELENGTH + 1];
  int stop;

  stop = 0;

  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;

  /* read the first line */
  nread = getwline (fd, line, sizeof (line));

  while (nread != 0 && !stop && (wcscmp (line, L"\n") == 0
				 || wcscmp (line, L"\r\n") == 0
				 || iscommand (line, &stop)))
    nread = getwline (fd, line, sizeof (line));

  wcscpy (title, line);

  entry = 0;
  width = 0;
  while ((nread = getwline (fd, line, sizeof (line))) > 0)
    {
      i = read_multi_entry (line, archive_member[entry], entry_title[entry]);
      if (i > 0)
	{
	  if (i > width)
	    width = i;
	  entry++;
	}
    }

  /* temporary script read */
  script_bytes_left = 0;

  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  avt_set_balloon_size (entry + 2, width + 3);

  avt_normal_text ();
  avt_set_text_delay (0);
  avt_set_origin_mode (AVT_FALSE);
  avt_newline_mode (AVT_TRUE);
  avt_set_scroll_mode (-1);

  avt_bold (AVT_TRUE);
  avt_say (title);
  avt_bold (AVT_FALSE);
  avt_new_line ();

  menu_start = avt_where_y ();
  for (i = 0; i < entry; i++)
    {
      avt_put_character ((wchar_t) i + L'1');
      avt_say (L") ");
      avt_say (entry_title[i]);
      avt_new_line ();
    }

  /* TODO: don't just exit */
  if (avt_menu (&ch, menu_start, menu_start + entry - 1, L'1',
		AVT_FALSE, AVT_FALSE))
    exit (EXIT_SUCCESS);

  /* back to normal... */
  avt_clear_screen ();
  moved_in = AVT_FALSE;
  avt_set_balloon_size (0, 0);
  avt_set_text_delay (default_delay);

  return arch_find_member (fd, archive_member[ch - L'1']);
}

/* opens the file, returns file descriptor or -1 on error */
static int
open_script (const char *fname)
{
  int fd = -1;
  char member_name[17];

  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;
  script_bytes_left = 0;
  from_archive = NULL;

  if (strcmp (fname, "-") == 0)
    fd = STDIN_FILENO;		/* stdin */
  else				/* regular file */
    {
      fd = arch_open (fname);

      /* if it's not an archive, open it as file */
      if (fd < 0)
	fd = open (fname, O_RDONLY | O_NONBLOCK);
      else			/* an archive */
	{
	  script_bytes_left = arch_first_member (fd, member_name);

	  if (script_bytes_left > 0)
	    {
	      if (strcmp (member_name, "AKFAvatar-demo") == 0)
		from_archive = strdup (fname);
	      else if (strcmp (member_name, "AKFAvatar-multi") == 0)
		{
		  from_archive = strdup (fname);
		  script_bytes_left = multi_menu (fd);
		}
	      else		/* no known archive content */
		{
		  close (fd);
		  fd = -1;
		}
	    }
	  else			/* script_bytes_left <= 0 */
	    {
	      close (fd);
	      fd = -1;
	    }
	}
    }

  read_error_is_eof = AVT_FALSE;

  return fd;
}

static int
say_line (const wchar_t * line, ssize_t nread)
{
  ssize_t i;
  int status = AVT_NORMAL;
  avt_bool_t underlined = AVT_FALSE;

  for (i = 0; i < nread; i++, line++)
    {
      if (*(line + 1) == L'\b')
	{
	  if (*line == L'_')
	    {
	      avt_underlined (AVT_TRUE);
	      status = avt_put_character (*(line + 2));
	      avt_underlined (underlined);
	      i += 2;
	      line += 2;
	    }
	  else if (*line == *(line + 2))
	    {
	      avt_bold (AVT_TRUE);
	      status = avt_put_character (*line);
	      avt_bold (AVT_FALSE);
	      i += 2;
	      line += 2;
	    }
	  else
	    {
	      i++;
	      line++;
	    }
	}
      else if (*line == L'_' && !rawmode)
	{
	  underlined = ~underlined;
	  avt_underlined (underlined);
	}
      else if (*line == L'\\' && *(line + 1) == L'\n')
	{
	  i++;
	  line++;
	}
      else if (*line == L'\\' && *(line + 1) == L'\r' && *(line + 2) == L'\n')
	{
	  i += 2;
	  line += 2;
	}
      else			/* not L'_' */
	{
	  status = avt_put_character (*line);
	  if (status)
	    break;
	}
    }

  if (underlined)
    avt_underlined (AVT_FALSE);

  return status;
}

/* shows content of file / other input */
/* returns 0:normal, 1:stop requested */
static int
process_script (int fd)
{
  wchar_t *line = NULL;
  size_t line_size = 0;
  ssize_t nread = 0;
  /* 1 = stop requested; 2 = end or stop command */
  int stop = 0;

  line_size = 1024 * sizeof (wchar_t);
  line = (wchar_t *) malloc (line_size);

  encoding_checked = AVT_FALSE;
  avt_set_scroll_mode (1);

  /* get first line */
  nread = getwline (fd, line, line_size);

  /* 
   * skip empty lines and handle commands at the beginning 
   * before initializing the graphics
   * a stripline starts the text
   */
  if (!rawmode)
    while (nread != 0 && wcsncmp (line, L"---", 3) != 0
	   && (wcscmp (line, L"\n") == 0 || wcscmp (line, L"\r\n") == 0
	       || iscommand (line, &stop)) && !stop)
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

  /* initialize the graphics */
  if (!initialized && !stop)
    initialize ();


  if (!moved_in && !popup)
    move_in ();

  /* show text */
  if (line && !stop && wcsncmp (line, L"---", 3) != 0)
    {
      if (say_line (line, nread))
	stop = 1;
    }

  while (!stop && (nread != 0 || ignore_eof))
    {
      nread = getwline (fd, line, line_size);

      if (ignore_eof && nread == 0)
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

      if (nread != 0 && !iscommand (line, &stop) && !stop)
	{
	  if (say_line (line, nread))
	    stop = 1;
	}
    }

  if (line)
    {
      free (line);
      line_size = 0;
    }

  if (close (fd) == -1 && errno != EAGAIN)
    warning_msg ("close", strerror (errno));

  if (from_archive)
    {
      free (from_archive);
      from_archive = NULL;
    }

  if (avt_get_status () == AVT_ERROR)
    {
      stop = 1;
      warning_msg ("AKFAvatar", avt_get_error ());
    }

  /* end or stop command not of interrest outside here */
  if (stop >= 2)
    stop = 0;

  avt_text_direction (AVT_LEFT_TO_RIGHT);
  return stop;
}

extern int get_file (char *filename);

static void
ask_file (void)
{
  char filename[256];

  get_file (filename);

  /* ignore quit-requests */
  /* (used to get out of the file dialog) */
  if (avt_get_status () == AVT_QUIT)
    avt_set_status (AVT_NORMAL);

  if (filename[0] != '\0')
    {
      int fd, status;

      avt_set_text_delay (default_delay);
      moved_in = AVT_FALSE;

      fd = open_script (filename);
      if (fd > -1)
	process_script (fd);

      /* ignore file errors */
      status = avt_get_status ();

      if (sound)
	{
	  avt_stop_audio ();
	  avt_free_audio (sound);
	  sound = NULL;
	}

      if (status == AVT_ERROR)
	exit (EXIT_FAILURE);	/* warning already printed */
      else if (status == AVT_NORMAL)
	if (avt_wait_button ())
	  exit (EXIT_SUCCESS);

      /* reset quit-request and encoding */
      avt_set_status (AVT_NORMAL);
      set_encoding (default_encoding);

      if (background_color_changed)
	{
	  default_background_color ();
	  background_color_changed = AVT_FALSE;
	}
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
avtterm_start (const char *enc AVT_UNUSED, const char *wd AVT_UNUSED,
	       char *const p[]AVT_UNUSED)
{
  not_available ();
  return -1;
}

#else /* not NO_PTY */

static int
start_shell (void)
{
  char home[PATH_LENGTH];

  get_user_home (home, sizeof (home));
  return avtterm_start (default_encoding, home, NULL);
}

static void
run_shell (void)
{
  int fd;

  /* must be initialized to get the window size */
  if (!initialized)
    {
      initialize ();

      if (!popup)
	move_in ();
    }

  avt_set_balloon_size (0, 0);
  avt_set_text_delay (0);
  avt_text_direction (AVT_LEFT_TO_RIGHT);

  fd = start_shell ();
  if (fd > -1)
    avtterm_run (fd);
}

static void
run_info (void)
{
  int fd;
  char *args[] = { "info", "akfavatar-en", NULL };

  avt_set_balloon_size (0, 0);
  avt_clear ();
  avt_set_text_delay (0);
  avt_text_direction (AVT_LEFT_TO_RIGHT);

  if (start_dir)
    if (chdir (start_dir))
      warning_msg ("chdir", strerror (errno));

  if (language == DEUTSCH)
    {
      if (access ("./doc/akfavatar-de.info", R_OK) == 0)
	args[1] = "--file=./doc/akfavatar-de.info";
      else
	args[1] = "akfavatar-de";
    }
  else				/* not DEUTSCH */
    {
      if (access ("./doc/akfavatar-en.info", R_OK) == 0)
	args[1] = "--file=./doc/akfavatar-en.info";
      else
	args[1] = "akfavatar-en";
    }

  fd = avtterm_start (default_encoding, NULL, args);
  if (fd > -1)
    avtterm_run (fd);
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

  avt_set_balloon_size (1, 40);
  avt_set_text_delay (0);

  avt_say (L"Manpage> ");

  if (avt_ask_mb (manpage, sizeof (manpage)) != 0)
    return;

  if (manpage[0] != '\0')
    {
      int fd, status;

      avt_set_balloon_size (0, 0);
      avt_clear ();
      avt_set_text_delay (default_delay);

      /* GROFF assumed! */
      putenv ("GROFF_TYPESETTER=latin1");
      putenv ("MANWIDTH=80");

      /* temporary settings */
      set_encoding ("ISO-8859-1");

      /* clear buffer */
      wcbuf_pos = wcbuf_len = 0;

      /* ignore file errors */
      read_error_is_eof = AVT_TRUE;
      fd = avtterm_start (default_encoding, NULL, argv);

      if (fd > -1)
	process_script (fd);

      /* just to prevent zombies */
      wait (NULL);

      status = avt_get_status ();
      if (status == AVT_ERROR)
	exit (EXIT_FAILURE);	/* warning already printed */

      if (status == AVT_NORMAL)
	avt_wait_button ();

      /* reset quit-request */
      avt_set_status (AVT_NORMAL);
      set_encoding (default_encoding);
    }
}

#endif /* not NO_PTY */
#endif /* not NO_MANPAGES */

#ifdef __WIN32__

extern void edit_file (const char *name);

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

  fd = avtterm_start (default_encoding, NULL, args);
  if (fd > -1)
    avtterm_run (fd);
}

#endif /* not Windows or ReactOS */

static void
create_file (const char *filename)
{
  FILE *f;

  f = fopen (filename, "wx");

  if (f)
    {
      if (default_encoding[0] != '\0')
	fprintf (f, "[encoding %s]\n", default_encoding);

      if (avt_image_name)
	fprintf (f, "[avatarimage %s]\n", avt_image_name);

      fputs ("\n# Text:\n", f);

      fclose (f);

#ifndef __WIN32__
      /* make file executable */
      chmod (filename, S_IRUSR | S_IWUSR | S_IXUSR
	     | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
    }
}

/* warn if someone tries to edit an archive file */
static avt_bool_t
dont_edit_archive (const char *filename)
{
  int fd;

  fd = arch_open (filename);
  if (fd < 0)
    return AVT_FALSE;

  close (fd);

  avt_set_balloon_size (1, 35);
  avt_clear ();
  avt_set_text_delay (default_delay);

  switch (language)
    {
    case DEUTSCH:
      avt_say (L"Kann keine Archiv-Datei bearbeiten.");
      break;

    case ENGLISH:
    default:
      avt_say (L"Can not edit an archive file.");
    }

  avt_wait_button ();

  return AVT_TRUE;
}

static void
ask_edit_file (void)
{
  char filename[256];

  avt_set_balloon_size (1, 0);
  avt_clear ();
  avt_set_text_delay (0);

  if (chdir (datadir))
    warning_msg ("chdir", strerror (errno));

  /* show directory and prompt (don't trust "datadir") */
  avt_say_mb (datadir);
  avt_say_mb ("> ");

  if (avt_ask_mb (filename, sizeof (filename)) != 0)
    return;

  if (filename[0] != '\0')
    {
      if (access (filename, F_OK) != 0)
	create_file (filename);
      else if (dont_edit_archive (filename))
	return;

      avt_set_balloon_size (0, 0);
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
  avt_set_balloon_size (0, 0);
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

  avt_wait_button ();
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
      avt_normal_text ();
      avt_set_balloon_size (10, 41);

      if (background_color_changed)
	default_background_color ();
      else
	avt_clear ();

      avt_set_text_delay (0);
      avt_set_origin_mode (AVT_FALSE);
      avt_newline_mode (AVT_TRUE);

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
	  avt_say (L"6) Vollbild-Anzeige umschalten\n");
	  avt_say (L"7) über avatarsay\n");
	  avt_say (L"8) beenden");	/* no newline */
	  break;

	case ENGLISH:
	default:
	  SAY_SHELL (L"1) terminal-mode\n");
	  avt_say (L"2) show a demo or textfile\n");
	  avt_say (L"3) create or edit a demo\n");
	  SAY_MANPAGE (L"4) show a manpage\n");
	  SAY_SHELL (L"5) documentation (info)\n");
	  avt_say (L"6) toggle fullscreen mode\n");
	  avt_say (L"7) about avatarsay\n");
	  avt_say (L"8) exit");	/* no newline */
	}

      menu_end = avt_where_y ();
      avt_set_text_delay (default_delay);

      if (avt_menu (&ch, menu_start, menu_end, L'1', AVT_FALSE, AVT_FALSE))
	exit (EXIT_SUCCESS);

      switch (ch)
	{
	case L'1':		/* terminal-mode */
	  avt_show_avatar ();	/* no balloon, while starting up */
	  run_shell ();
	  avt_set_status (AVT_NORMAL);
	  break;

	case L'2':		/* show a demo or textfile */
	  ask_file ();
	  break;

	case L'3':		/* create or edit a demo */
	  ask_edit_file ();
	  avt_set_status (AVT_NORMAL);
	  break;

	case L'4':		/* show a manpage */
	  ask_manpage ();
	  avt_set_status (AVT_NORMAL);
	  break;

	case L'5':		/* documentation */
	  run_info ();
	  avt_set_status (AVT_NORMAL);
	  break;

	case L'6':		/* toggle fullscreen */
	  avt_toggle_fullscreen ();
	  break;

	case L'7':		/* about avatarsay */
	  about_avatarsay ();
	  avt_set_status (AVT_NORMAL);
	  break;

	case L'8':		/* exit */
	  if (!popup)
	    move_out ();
	  exit (EXIT_SUCCESS);

	default:
	  avt_bell ();
	  break;
	}

      if (sound)
	{
	  avt_stop_audio ();
	  avt_free_audio (sound);
	  sound = NULL;
	}

      avt_set_title ("AKFAvatar", "AKFAvatar");

      if (avatar_changed)
	{
	  restore_avatar_image ();
	  if (!popup)
	    move_in ();
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
run_script (char *f)
{
  int status = -1;
  int fd;

  set_encoding (default_encoding);

  fd = open_script (f);

  /* if it can't be opened and there is no slash, try with datadir */
  if (fd == -1 && strchr (f, '/') == NULL)
    {
      char p[PATH_LENGTH];
      strcpy (p, datadir);
      strcat (p, "/");
      strcat (p, f);
      fd = open_script (p);
    }

  if (fd > -1)
    status = process_script (fd);

  if (status < 0)
    error_msg ("error opening file", f);
  else if (status == 1)		/* halt requested */
    exit (EXIT_SUCCESS);
  else if (status > 1)		/* problem with libakfavatar */
    exit (EXIT_FAILURE);

  avt_set_scroll_mode (1);

  if (avt_flip_page ())
    exit (EXIT_SUCCESS);
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

static void
initialize_datadir (void)
{
  char home[PATH_LENGTH];

  get_user_home (home, sizeof (home));
  strncpy (datadir, home, sizeof (datadir));
  datadir[sizeof (datadir) - 1] = '\0';
}

static void
initialize_start_dir (void)
{
  char buf[4096];

  if (getcwd (buf, sizeof (buf)))
    start_dir = strdup (buf);
  else
    start_dir = NULL;
}

int
main (int argc, char *argv[])
{
  /* initialize variables */
  supposed_title = NULL;
  window_mode = AVT_AUTOMODE;
  loop = AVT_TRUE;
  default_delay = AVT_DEFAULT_TEXT_DELAY;
  avtterm_nocolor (AVT_FALSE);

  atexit (quit);
  initialize_program_name (argv[0]);
  initialize_start_dir ();
  initialize_datadir ();

  /* this is just a default setting */
  strcpy (default_encoding, "ISO-8859-1");


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

  default_background_color ();

  check_config_file ("/etc/avatarsay");
  checkenvironment ();
  checkoptions (argc, argv);

  if (terminal_mode)
    {
      run_shell ();
      exit (EXIT_SUCCESS);
    }
#ifndef NO_PTY
  else if (executable)
    {
      int fd;

      /* must be initialized to get the window size */
      if (!initialized)
	{
	  initialize ();

	  if (!popup)
	    move_in ();
	}

      fd = avtterm_start (default_encoding, NULL, &argv[optind]);
      if (fd > -1)
	avtterm_run (fd);

      if (avt_get_status () == AVT_NORMAL)
	if (avt_wait_button ())
	  exit (EXIT_SUCCESS);

      if (initialized && !popup)
	move_out ();
      exit (EXIT_SUCCESS);
    }
#endif /* not NO_PTY */
  else if (optind >= argc)	/* no input files? -> menu */
    menu ();

  /* handle files given as parameters */
  do
    {
      int i;

      for (i = optind; i < argc; i++)
	run_script (argv[i]);

      if (initialized && !popup)
	{
	  move_out ();

	  /* if running in a loop, wait a while */
	  if (loop)
	    if (avt_wait (5000))
	      exit (EXIT_SUCCESS);
	}
    }
  while (loop);

  exit (EXIT_SUCCESS);

  /* never executed, but kept in the code */
  puts ("$Id: avatarsay.c,v 2.255 2009-01-04 16:09:09 akf Exp $");

  return EXIT_SUCCESS;
}
