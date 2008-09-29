/* 
 * avtterm - terminal emulation for AKFAAvatar
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

/* $Id: avtterm.c,v 2.16 2008-09-29 11:49:07 akf Exp $ */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "avtterm.h"
#include "avtmsg.h"
#include "version.h"
#include "akfavatar.h"
#include <wchar.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pwd.h>

/* terminal type */
/* 
 * this is not dependent on the system on which it runs,
 * but the terminal database should have an entry for this
 */
#define TERM "linux"
#define BWTERM "linux-m"

/* size for input buffer - not too small, please */
/* .encoding must be in first buffer */
#define INBUFSIZE 1024

/* device attribute (DEC) */
#define DS "\033[?1;2c"		/* claim to be a vt100 with advanced video */

/* Vt100 graphics is handled internaly */
#define VT100 "VT100 graphics"

/* default encoding - either system encoding or given per parameters */
/* supported in SDL: ASCII, ISO-8859-1, UTF-8, UTF-16, UTF-32 */
static const char *default_encoding;

static int prg_input;		/* file descriptor for program input */

/* in idle loop? */
static avt_bool_t idle;

/* maximum coordinates */
static int max_x, max_y;

static int region_min_y, region_max_y;

/* insert mode */
static avt_bool_t insert_mode;

/* no color (but still bold, underlined, reversed) allowed */
static avt_bool_t nocolor;

/* colors for terminal mode */
static int text_color;
static int text_background_color;
static avt_bool_t faint;

/* use vt100 graphics? */
static avt_bool_t vt100graphics;

/* G0 and G1 charset encoding (linux-specific) */
static const char *G0, *G1;

/* character for DEC cursor keys (either [ or O) */
static char dec_cursor_seq[3];

/* text-buffer */
static wchar_t *wcbuf = NULL;
static int wcbuf_pos = 0;
static int wcbuf_len = 0;

static const wchar_t vt100trans[] = {
  0x00A0, 0x25C6, 0x2592, 0x2409, 0x240C, 0x240D,
  0x240A, 0x00B0, 0x00B1, 0x2424, 0x240B,
  0x2518, 0x2510, 0x250C, 0x2514, 0x253C,
  0x23BA, 0x23BB, 0x2500, 0x23BC, 0x23BD,
  0x251C, 0x2524, 0x2534, 0x252C, 0x2502,
  0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00B7
};

/* defined in avatarsay.c */
/* used in APC_command */
extern void avatar_command (wchar_t * s, int *stop);


static void
set_encoding (const char *encoding)
{
  vt100graphics = (strcmp (VT100, encoding) == 0);

  if (vt100graphics)
    avt_mb_encoding ("US-ASCII");
  else if (avt_mb_encoding (encoding))
    error_msg ("iconv", avt_get_error ());
}

void
avtterm_nocolor (avt_bool_t on)
{
  nocolor = on;
}

/* set terminal size */
static void
avtterm_size (int fd, int height, int width)
{
#ifdef TIOCSWINSZ
  struct winsize size;

  size.ws_row = height;
  size.ws_col = width;
  size.ws_xpixel = size.ws_ypixel = 0;
  ioctl (fd, TIOCSWINSZ, &size);
#endif
}

/* @@@ */
static wint_t
get_character (int fd)
{
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
      nread = read (fd, &filebuf, sizeof (filebuf) - 1);

      /* waiting for data */
      if (nread == -1 && errno == EAGAIN)
	{
	  idle = AVT_TRUE;
	  while (nread == -1 && errno == EAGAIN
		 && avt_update () == AVT_NORMAL)
	    nread = read (fd, &filebuf, sizeof (filebuf) - 1);
	  idle = AVT_FALSE;
	}

      if (nread == -1)
	wcbuf_len = -1;
      else			/* nread != -1 */
	{
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

static void
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
static void
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

/* avoid anything that draws the balloon here */
static void
reset_terminal (void)
{
  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;

  region_min_y = 1;
  region_max_y = max_y;
  insert_mode = AVT_FALSE;

  avt_reserve_single_keys (AVT_TRUE);
  avt_newline_mode (AVT_FALSE);
  avt_activate_cursor (AVT_TRUE);
  avt_set_scroll_mode (1);
  vt100graphics = AVT_FALSE;
  G0 = "ISO-8859-1";
  G1 = VT100;
  set_encoding (default_encoding);	/* not G0! */

  /* like vt102 */
  avt_set_origin_mode (AVT_FALSE);

  avt_reset_tab_stops ();
  ansi_graphic_code (0);
  avt_set_text_delay (0);
  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
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
      do
	ch = get_character (fd);
      while (ch == L'\0');

      /* ignore CSI [ + one character */
      if (pos == 0 && ch == L'[')
	{
	  get_character (fd);	/* ignore one char */
	  return;
	}

      /* CAN or SUB cancel the escape-sequence */
      if (ch == L'\x18' || ch == L'\x1A')
	return;

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

      /* AKFAvatar extension - set balloon size (comp. to xterm) */
    case L't':
      if (sequence[0] == '8')
	{
	  char *next;
	  int height = 0, width = 0;

	  next = &sequence[2];
	  height = strtol (next, &next, 10);
	  if (*next == ';')
	    {
	      next++;
	      width = strtol (next, &next, 10);
	    }

	  if (!height)
	    height = max_y;
	  if (!width)
	    width = max_x;

	  avt_set_balloon_size (height, width);
	  avtterm_size (prg_input, height, width);
	}
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

/* APC: Application Program Command */
static void
APC_sequence (int fd)
{
  wchar_t ch, old;
  wchar_t command[1024];
  unsigned int p;
  int ignore;

  p = 0;
  ch = old = L'\0';

  /* skip until "\a" or "\x9c" or "\033\\" is found */
  while (p < (sizeof (command) / sizeof (wchar_t))
	 && ch != L'\a' && ch != L'\x9c' && (old != L'\033' || ch != L'\\'))
    {
      old = ch;
      ch = get_character (fd);
      if (ch >= L' ' && ch != L'\x9c' && (old != L'\033' || ch != L'\\'))
	command[p++] = ch;
    }

  command[p] = L'\0';
  
  avatar_command (command, &ignore);
}

/*
 * OSC sequences aren't really supported yet,
 * but we need the code, to successfully ignore that stuff
 */
static void
OSC_sequence (int fd)
{
  wchar_t ch, old;
  int i;

  do
    ch = get_character (fd);
  while (ch == L'\0');

  if (ch == L'P')		/* set palette (Linux) */
    {
      /* ignore 7 characters */
      for (i = 0; i < 7; i++)
	get_character (fd);
      return;
    }

  /* ignore L'R' reset palette (Linux) */
  if (ch == L'R')
    return;

  /* skip until "\a" or "\x9c" or "\033\\" is found */
  do
    {
      old = ch;
      ch = get_character (fd);
    }
  while (ch != L'\a' && ch != L'\x9c' && (old != L'\033' || ch != L'\\'));
}

static void
escape_sequence (int fd, wchar_t last_character)
{
  wchar_t ch;
  static int saved_text_color, saved_text_background_color;
  static avt_bool_t saved_underline_state, saved_bold_state;
  static const char *saved_G0, *saved_G1;

  do
    ch = get_character (fd);
  while (ch == L'\0');

#ifdef DEBUG
  if (ch != L'[')
    fprintf (stderr, "ESC %lc\n", ch);
#endif

  /* ESC [ch] */
  switch (ch)
    {
    case L'\x18':		/* CAN */
    case L'\x1A':		/* SUB */
      /* cancel escape sequence */
      break;

    case L'7':			/* DECSC */
      avt_save_position ();
      saved_text_color = text_color;
      saved_text_background_color = text_background_color;
      saved_underline_state = avt_get_underlined ();
      saved_bold_state = avt_get_bold ();
      saved_G0 = G0;
      saved_G1 = G1;
      break;

    case L'8':			/* DECRC */
      avt_restore_position ();
      text_color = saved_text_color;
      set_foreground_color (text_color);
      text_background_color = saved_text_background_color;
      set_background_color (text_background_color);
      avt_underlined (saved_underline_state);
      avt_bold (saved_bold_state);
      G0 = saved_G0;
      G1 = saved_G1;
      break;

    case L'%':			/* select charset */
      {
	wchar_t ch2;
	do
	  ch2 = get_character (fd);
	while (ch2 == L'\0');
	if (ch2 == L'@')
	  set_encoding (G0);	/* TODO: unsure */
	else if (ch2 == L'G' || ch2 == L'8' /* obsolete */ )
	  set_encoding ("UTF-8");
	/* else if (ch2 == L'B')
	   set_encoding ("UTF-1"); *//* does anybody use that? */
      }
      break;

      /* Note: this is not compatible to ISO 2022! */
    case L'(':			/* set G0 */
      {
	wchar_t ch2;
	do
	  ch2 = get_character (fd);
	while (ch2 == L'\0');
	if (ch2 == L'B')
	  G0 = "ISO-8859-1";
	else if (ch2 == L'0')
	  G0 = VT100;
	else if (ch2 == L'U')
	  G0 = "IBM437";
	else if (ch2 == L'K')
	  G0 = "UTF-8";		/* "user-defined" */
      }
      break;

    case L')':			/* set G1 */
      {
	wchar_t ch2;
	do
	  ch2 = get_character (fd);
	while (ch2 == L'\0');
	if (ch2 == L'B')
	  G1 = "ISO-8859-1";
	else if (ch2 == L'0')
	  G1 = VT100;
	else if (ch2 == L'U')
	  G1 = "IBM437";
	else if (ch2 == L'K')
	  G1 = "UTF-8";		/* "user-defined" */
      }
      break;

    case L'[':			/* CSI */
      CSI_sequence (fd, last_character);
      break;

    case L'c':			/* RIS - reset device */
      reset_terminal ();
      avt_clear ();
      avt_save_position ();
      avt_viewport (1, region_min_y, max_x, region_max_y);
      saved_text_color = text_color;
      saved_text_background_color = text_background_color;
      break;

    case L'D':			/* move down or scroll up one line */
      if (avt_where_y () < region_max_y)
	avt_move_y (avt_where_y () + 1);
      else
	avt_delete_lines (region_min_y, 1);
      break;

    case L'E':
      avt_move_x (1);
      avt_new_line ();
      break;

/* for some few terminals it's the home function 
    case L'H':
      avt_move_x (1);
      avt_move_y (1);
      return;
*/

    case L'H':			/* HTS */
      avt_set_tab (avt_where_x (), AVT_TRUE);
      break;

    case L'M':			/* RI - scroll down one line */
      if (avt_where_y () > region_min_y)
	avt_move_y (avt_where_y () - 1);
      else
	avt_insert_lines (region_min_y, 1);
      break;

    case L'Z':			/* DECID */
      write (prg_input, DS, sizeof (DS) - 1);
      break;

      /* OSC: Operating System Command */
    case L']':
      OSC_sequence (fd);
      break;

      /* APC: Application Program Command */
    case L'_':
      APC_sequence (fd);
      break;

    default:
      fprintf (stderr, ESC_UNUPPORTED " %lc\n", ch);
      break;
    }
}

void
process_subprogram (int fd)
{
  avt_bool_t stop;
  wint_t ch;
  wchar_t last_character;

  last_character = L'\0';
  stop = AVT_FALSE;

  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
  dec_cursor_seq[2] = ' ';	/* to be filled later */
  avt_register_keyhandler (prg_keyhandler);
  /* TODO: doesn't work yet */
  /* avt_register_mousehandler (prg_mousehandler); */

  reset_terminal ();

  /* FIXME: \x80 - \x9f may not survive through iconv */
  /* use the escape sequences instead */
  while ((ch = get_character (fd)) != WEOF && !stop)
    {
      if (ch == L'\033')	/* Esc */
	escape_sequence (fd, last_character);
      else if (ch == L'\x9b')	/* CSI (may not work) */
	CSI_sequence (fd, last_character);
      else if (ch == L'\x9d')	/* OSC (may not work) */
	OSC_sequence (fd);
      else if (ch == L'\x9f')	/* APC (may not work) */
	APC_sequence (fd);
      else if (ch == L'\x0E')	/* SO */
	set_encoding (G1);
      else if (ch == L'\x0F')	/* SI */
	set_encoding (G0);
      else
	{
	  if (insert_mode)
	    avt_insert_spaces (1);

	  if (vt100graphics && (ch >= 95 && ch <= 126))
	    ch = vt100trans[ch - 95];

	  last_character = (wchar_t) ch;

	  stop = avt_put_character ((wchar_t) ch);
	}
    }

  avt_activate_cursor (AVT_FALSE);
  avt_reserve_single_keys (AVT_FALSE);

  /* close file descriptor */
  if (close (fd) == -1 && errno != EAGAIN)
    warning_msg ("close", strerror (errno));

  /* just to prevent zombies */
  wait (NULL);

  /* release keyhandler */
  avt_register_keyhandler (NULL);
  prg_input = -1;

  /* release wcbuf */
  if (wcbuf)
    {
      avt_free (wcbuf);
      wcbuf = NULL;
    }
}

/* execute a subprocess, visible in the balloon */
/* if fname == NULL, start a shell */
/* returns file-descriptor for output of the process */
int
execute_process (const char *system_encoding, char *const prg_argv[])
{
  pid_t childpid;
  int master, slave;
  char *terminalname;
  struct termios settings;
  char *shell = "/bin/sh";

  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;

  default_encoding = system_encoding;
  max_x = avt_get_max_x ();
  max_y = avt_get_max_y ();
  region_min_y = 1;
  region_max_y = max_y;
  insert_mode = AVT_FALSE;

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

  /* terminal settings */
  if (tcgetattr (master, &settings) < 0)
    {
      close (master);
      close (slave);
      return -1;
    }

  /* TODO: improve */
  settings.c_cc[VERASE] = 8;	/* Backspace */
  settings.c_iflag |= ICRNL;	/* input: cr -> nl */
  settings.c_lflag |= (ECHO | ECHOE | ECHOK | ICANON);

  if (tcsetattr (master, TCSANOW, &settings) < 0)
    {
      close (master);
      close (slave);
      return -1;
    }

  avtterm_size (master, max_y, max_x);

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
  fcntl (master, F_SETFL, O_NONBLOCK);
  prg_input = master;

  return master;
}
