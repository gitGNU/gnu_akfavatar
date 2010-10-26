/*
 * avtterm - terminal emulation for AKFAAvatar
 * Copyright (c) 2007, 2008, 2009, 2010 Andreas K. Foerster <info@akfoerster.de>
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

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "akfavatar.h"
#include "avttermsys.h"
#include "avtaddons.h"
#include <wchar.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* size for input buffer */
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

/* cursor active? */
static avt_bool_t cursor_active;

static int text_delay;

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

static avt_bool_t application_keypad;

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

/* handler for APC commands */
static avta_term_apc_cmd apc_cmd_handler;

static void
set_encoding (const char *encoding)
{
  vt100graphics = (strcmp (VT100, encoding) == 0);

  if (vt100graphics)
    {
      if (avt_mb_encoding ("US-ASCII"))
	avta_error ("iconv", avt_get_error ());
    }
  else if (avt_mb_encoding (encoding))
    {
      avta_warning ("iconv", avt_get_error ());

      /* try a fallback */
      avt_set_status (AVT_NORMAL);
      if (avt_mb_encoding ("US-ASCII"))
	avta_error ("iconv", avt_get_error ());
    }
}

extern void
avta_term_nocolor (avt_bool_t on)
{
  nocolor = on;
}

extern void
avta_term_slowprint (avt_bool_t on)
{
  if (on)
    text_delay = AVT_DEFAULT_TEXT_DELAY;
  else
    text_delay = 0;

  avt_set_text_delay (text_delay);
}

static void
activate_cursor (avt_bool_t on)
{
  cursor_active = AVT_MAKE_BOOL (on);
  avt_activate_cursor (cursor_active);
}

extern void
avta_term_update_size (void)
{
  if (prg_input > 0)
    {
      max_x = avt_get_max_x ();
      max_y = avt_get_max_y ();

      avta_term_size (prg_input, max_y, max_x);

      if (region_max_y > max_y)
	region_max_y = max_y;
      if (region_min_y > max_y)
	region_min_y = 1;
    }
}

/* TODO: make get_character simpler */
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

      /* update */
      if (text_delay == 0)
	avt_lock_updates (AVT_FALSE);

      /* reserve one byte for a terminator */
      nread = read (fd, &filebuf, sizeof (filebuf) - 1);

      /* waiting for data */
      if (nread == -1 && errno == EAGAIN)
	{
	  if (cursor_active)
	    avt_activate_cursor (AVT_TRUE);
	  idle = AVT_TRUE;
	  do
	    {
	      nread = read (fd, &filebuf, sizeof (filebuf) - 1);
	    }
	  while (nread == -1 && errno == EAGAIN
		 && avt_update () == AVT_NORMAL);
	  idle = AVT_FALSE;
	  if (cursor_active)
	    avt_activate_cursor (AVT_FALSE);
	}

      if (nread == -1)
	wcbuf_len = -1;
      else			/* nread != -1 */
	{
	  wcbuf_len = avt_mb_decode (&wcbuf, (char *) &filebuf, nread);
	  wcbuf_pos = 0;
	}

      if (text_delay == 0)
	avt_lock_updates (AVT_TRUE);
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

extern void
avta_term_send (const char *buf, size_t count)
{
  ssize_t r;

  do
    {
      r = write (prg_input, buf, count);
      if (r > 0)
	{
	  count -= r;
	  buf += r;
	}
    }
  while (r > 0 && count > 0);
}

static void
prg_keyhandler (int sym, int mod AVT_UNUSED, int unicode)
{
  /* TODO: support application_keypad */

  if (idle && prg_input > 0)
    {
      idle = AVT_FALSE;		/* avoid reentrance */

      switch (sym)
	{
	case 273:		/* up arrow */
	  dec_cursor_seq[2] = 'A';
	  avta_term_send (dec_cursor_seq, 3);
	  break;

	case 274:		/* down arrow */
	  dec_cursor_seq[2] = 'B';
	  avta_term_send (dec_cursor_seq, 3);
	  break;

	case 275:		/* right arrow */
	  dec_cursor_seq[2] = 'C';
	  avta_term_send (dec_cursor_seq, 3);
	  break;

	case 276:		/* left arrow */
	  dec_cursor_seq[2] = 'D';
	  avta_term_send (dec_cursor_seq, 3);
	  break;

	case 277:		/* Insert */
	  /* avta_term_send ("\033[L", 3); */
	  avta_term_send ("\033[2~", 4);	/* linux */
	  break;

	case 278:		/* Home */
	  /* avta_term_send ("\033[H", 3); */
	  avta_term_send ("\033[1~", 4);	/* linux */
	  break;

	case 279:		/* End */
	  /* avta_term_send ("\033[0w", 4); */
	  avta_term_send ("\033[4~", 4);	/* linux */
	  break;

	case 280:		/* Page up */
	  avta_term_send ("\033[5~", 4);	/* linux */
	  break;

	case 281:		/* Page down */
	  avta_term_send ("\033[6~", 4);	/* linux */
	  break;

	case 282:		/* F1 */
	  avta_term_send ("\033[[A", 4);	/* linux */
	  /* avta_term_send ("\033OP", 3); *//* DEC */
	  break;

	case 283:		/* F2 */
	  avta_term_send ("\033[[B", 4);	/* linux */
	  /* avta_term_send ("\033OQ", 3); *//* DEC */
	  break;

	case 284:		/* F3 */
	  avta_term_send ("\033[[C", 4);	/* linux */
	  /* avta_term_send ("\033OR", 3); *//* DEC */
	  break;

	case 285:		/* F4 */
	  avta_term_send ("\033[[D", 4);	/* linux */
	  /* avta_term_send ("\033OS", 3); *//* DEC */
	  break;

	case 286:		/* F5 */
	  avta_term_send ("\033[[E", 4);	/* linux */
	  /* avta_term_send ("\033Ot", 3); *//* DEC */
	  break;

	case 287:		/* F6 */
	  avta_term_send ("\033[17~", 5);	/* linux */
	  /* avta_term_send ("\033Ou", 3); *//* DEC */
	  break;

	case 288:		/* F7 */
	  avta_term_send ("\033[[18~", 5);	/* linux */
	  /* avta_term_send ("\033Ov", 3); *//* DEC */
	  break;

	case 289:		/* F8 */
	  avta_term_send ("\033[19~", 5);	/* linux */
	  /* avta_term_send ("\033Ol", 3); *//* DEC */
	  break;

	case 290:		/* F9 */
	  avta_term_send ("\033[20~", 5);	/* linux */
	  /* avta_term_send ("\033Ow", 3); *//* DEC */
	  break;

	case 291:		/* F10 */
	  avta_term_send ("\033[21~", 5);	/* linux */
	  /* avta_term_send ("\033Ox", 3); *//* DEC */
	  break;

	case 292:		/* F11 */
	  avta_term_send ("\033[23~", 5);	/* linux */
	  break;

	case 293:		/* F12 */
	  avta_term_send ("\033[24~", 5);	/* linux */
	  break;

	case 294:		/* F13 */
	  avta_term_send ("\033[25~", 5);	/* linux */
	  break;

	case 295:		/* F14 */
	  avta_term_send ("\033[26~", 5);	/* linux */
	  break;

	case 296:		/* F15 */
	  avta_term_send ("\033[27~", 5);	/* linux */
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
		  avta_term_send (mbstring, length);
		  avt_free (mbstring);
		}
	    }			/* if (unicode) */
	}			/* switch */

      idle = AVT_TRUE;
    }				/* if (idle...) */
}

/* just handling the mouse wheel */
static void
prg_wheelhandler (int button, avt_bool_t pressed,
		  int x AVT_UNUSED, int y AVT_UNUSED)
{
  if (pressed)
    {
      if (button == 4)
	{
	  dec_cursor_seq[2] = 'A';
	  avta_term_send (dec_cursor_seq, 3);
	}
      else if (button == 5)
	{
	  dec_cursor_seq[2] = 'B';
	  avta_term_send (dec_cursor_seq, 3);
	}
    }
}

/* TODO: prg_mousehandler doesn't work yet */
static void
prg_mousehandler (int button, avt_bool_t pressed, int x, int y)
{
  char code[7];

  /* X10 method */
  if (pressed)
    {
      snprintf (code, sizeof (code), "\033[M%c%c%c",
		(char) (040 + button), (char) (040 + x), (char) (040 + y));
      avta_term_send (&code[0], sizeof (code) - 1);
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
    case 15:			/* ballooncolor */
      avt_set_text_background_ballooncolor ();
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
  activate_cursor (AVT_TRUE);
  avt_set_scroll_mode (1);
  vt100graphics = AVT_FALSE;
  G0 = "ISO-8859-1";
  G1 = VT100;
  set_encoding (default_encoding);	/* not G0! */

  /* like vt102 */
  avt_set_origin_mode (AVT_FALSE);

  avt_reset_tab_stops ();
  ansi_graphic_code (0);
  avta_term_slowprint (AVT_FALSE);
  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
}

#define ESC_UNUPPORTED "unsupported escape sequence"
#define CSI_UNUPPORTED "unsupported CSI sequence"


/* Esc [ ... */
/* CSI */
static void
CSI_sequence (int fd, avt_char last_character)
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
	avt_put_char (last_character);
      else
	{
	  int count = strtol (sequence, NULL, 10);
	  int i;
	  for (i = 0; i < count; i++)
	    avt_put_char (last_character);
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
	avta_term_send (DS, sizeof (DS) - 1);
      else if (sequence[0] == '?')
	{			/* I have no real infos about that :-( */
	  if (sequence[1] == '1' && sequence[2] == 'c')
	    activate_cursor (AVT_FALSE);
	  else if (sequence[1] == '2' && sequence[2] == 'c')
	    activate_cursor (AVT_TRUE);
	  else if (sequence[1] == '0' && sequence[2] == 'c')
	    activate_cursor (AVT_TRUE);	/* normal? */
	  else if (sequence[1] == '8' && sequence[2] == 'c')
	    activate_cursor (AVT_TRUE);	/* very visible */
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
      else			/* TODO: TBC 1-5 are not distinguished here */
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
	      /* TODO: mouse doesn't work yet */
	      /* avt_register_mousehandler (prg_mousehandler); */
	      break;
	    case 25:
	      activate_cursor (AVT_TRUE);
	      break;
	    case 56:		/* AKFAvatar extension */
	      avta_term_slowprint (AVT_TRUE);
	      break;
	    case 66:
	      application_keypad = AVT_TRUE;
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
	      /* TODO: mouse doesn't work yet */
	      /* avt_register_mousehandler (NULL); */
	      break;
	    case 25:
	      activate_cursor (AVT_FALSE);
	      break;
	    case 56:		/* AKFAvatar extension */
	      avta_term_slowprint (AVT_FALSE);
	      break;
	    case 66:
	      application_keypad = AVT_FALSE;
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
	avta_term_send ("\033[0n", 4);	/* device okay */
      /* "\033[3n" for failure */
      else if (sequence[0] == '6' && sequence[1] == 'n')
	{
	  /* report cursor position */
	  char s[80];
	  snprintf (s, sizeof (s), "\033[%d;%dR",
		    avt_where_x (), avt_where_y ());
	  avta_term_send (s, strlen (s));
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

	  avt_set_balloon_size (height, width);
	  max_x = avt_get_max_x ();
	  max_y = avt_get_max_y ();
	  avta_term_size (prg_input, max_y, max_x);
	  if (region_max_y > max_y)
	    region_max_y = max_y;
	  if (region_min_y > max_y)
	    region_min_y = 1;
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
      avta_warning (CSI_UNUPPORTED, sequence);
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

  if (apc_cmd_handler)
    (void) (*apc_cmd_handler) (command);
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
escape_sequence (int fd, avt_char last_character)
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
	  set_encoding (G0);	/* unsure */
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
      avta_term_send (DS, sizeof (DS) - 1);
      break;

      /* OSC: Operating System Command */
    case L']':
      OSC_sequence (fd);
      break;

      /* APC: Application Program Command */
    case L'_':
      APC_sequence (fd);
      break;

    case L'>':			/* DECPNM */
      application_keypad = AVT_FALSE;
      break;

    case L'=':			/* DECPAM */
      application_keypad = AVT_TRUE;
      break;

    default:
      fprintf (stderr, ESC_UNUPPORTED " %lc\n", ch);
      break;
    }
}

extern void
avta_term_register_apc (avta_term_apc_cmd command)
{
  apc_cmd_handler = command;
}

extern void
avta_term_run (int fd)
{
  avt_bool_t stop;
  wint_t ch;
  avt_char last_character;

  /* check, if fd is valid */
  if (fd < 0)
    return;

  last_character = 0x0000;
  stop = AVT_FALSE;

  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
  dec_cursor_seq[2] = ' ';	/* to be filled later */
  avt_register_keyhandler (prg_keyhandler);
  avt_register_mousehandler (prg_wheelhandler);
  avt_set_mouse_visible (AVT_FALSE);
  /* TODO: mouse clicks don't work yet */
  /* avt_register_mousehandler (prg_mousehandler); */

  reset_terminal ();
  avt_lock_updates (AVT_TRUE);

  while ((ch = get_character (fd)) != WEOF && !stop)
    {
      if (ch == L'\033')	/* Esc */
	escape_sequence (fd, last_character);
      else if (ch == L'\x9b')	/* CSI */
	CSI_sequence (fd, last_character);
      else if (ch == L'\x9d')	/* OSC */
	OSC_sequence (fd);
      else if (ch == L'\x9f')	/* APC */
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

	  last_character = (avt_char) ch;

	  stop = avt_put_char ((avt_char) ch);
	}
    }

  avta_closeterm (fd);

  activate_cursor (AVT_FALSE);
  avt_reserve_single_keys (AVT_FALSE);

  /* release handlers */
  avt_register_mousehandler (NULL);
  avt_register_keyhandler (NULL);
  prg_input = -1;

  /* release wcbuf */
  if (wcbuf)
    {
      avt_free (wcbuf);
      wcbuf = NULL;
    }
}

extern int
avta_term_start (const char *system_encoding, const char *working_dir,
		 char *const prg_argv[])
{
  /* clear text-buffer */
  wcbuf_pos = wcbuf_len = 0;
  default_encoding = system_encoding;

  max_x = avt_get_max_x ();
  max_y = avt_get_max_y ();
  region_min_y = 1;
  region_max_y = max_y;
  insert_mode = AVT_FALSE;

  /* check if AKFAvatar was initialized */
  if (max_x < 0 || max_y < 0)
    return -1;

  return
    avta_term_initialize (&prg_input, max_x, max_y, nocolor,
			  working_dir, prg_argv);
}
