/*
 * avtterm - terminal emulation for AKFAAvatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
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
#include "avtinternals.h"
#include "avttermsys.h"
#include "avtaddons.h"
#include <wchar.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iso646.h>
#include <stdint.h>
#include <limits.h>

// size for input buffer
#define INBUFSIZE 1024

#define ESC  "\033"
#define CSI  ESC "["

// device attribute (DEC)
// claim to be a vt100 with advanced video
#define DS  CSI "?1;2c"

#define TERM_LINUX 1

#ifdef TERM_LINUX
#define KEY_HOME      CSI "1~"
#define KEY_INSERT    CSI "2~"
#define KEY_END       CSI "4~"
#define KEY_PAGEUP    CSI "5~"
#define KEY_PAGEDOWN  CSI "6~"
#define KEY_F1        CSI "[A"
#define KEY_F2        CSI "[B"
#define KEY_F3        CSI "[C"
#define KEY_F4        CSI "[D"
#define KEY_F5        CSI "[E"
#define KEY_F6        CSI "17~"
#define KEY_F7        CSI "18~"
#define KEY_F8        CSI "19~"
#define KEY_F9        CSI "20~"
#define KEY_F10       CSI "21~"
#define KEY_F11       CSI "23~"
#define KEY_F12       CSI "24~"
#define KEY_F13       CSI "25~"
#define KEY_F14       CSI "26~"
#define KEY_F15       CSI "27~"
#else // DEC compatible (unsure)
#define KEY_HOME      CSI "H"
#define KEY_INSERT    CSI "L"
#define KEY_END       CSI "0w"
#define KEY_PAGEUP    CSI "5~"
#define KEY_PAGEDOWN  CSI "6~"
#define KEY_F1        ESC "OP"
#define KEY_F2        ESC "OQ"
#define KEY_F3        ESC "OR"
#define KEY_F4        ESC "OS"
#define KEY_F5        ESC "Ot"
#define KEY_F6        ESC "Ou"
#define KEY_F7        ESC "Ov"
#define KEY_F8        ESC "Ol"
#define KEY_F9        ESC "Ow"
#define KEY_F10       ESC "Ox"
#define KEY_F11       CSI "23~"
#define KEY_F12       CSI "24~"
#define KEY_F13       CSI "25~"
#define KEY_F14       CSI "26~"
#define KEY_F15       CSI "27~"
#endif

#define AVT_EOF (0xFFFFFFFFu)

const struct avt_charenc *convert;

// default encoding - either system encoding or given per parameters
static const struct avt_charenc *default_encoding;

static int prg_input;		// file descriptor for program input

// maximum coordinates
static int max_x, max_y;
static int region_min_y, region_max_y;

// insert mode
static bool insert_mode;

// cursor active?
static bool cursor_active;

static int text_delay;

// no color (but still bold, underlined, reversed) allowed
static bool nocolor;

// colors for terminal mode
static int text_color;
static int text_background_color;
static bool faint;

// G0 and G1 charset encoding (linux-specific)
static const struct avt_charenc *G0, *G1;

// sequence for DEC cursor keys (either Esc[A or EscOA)
static char dec_cursor_seq[3];

// mode for mouse: 0: no mouse, 1: X10, 2: X11
static int mouse_mode;

static bool application_keypad;

static const uint_least16_t vt100trans[] = {
  0x00A0, 0x25C6, 0x2592, 0x2409, 0x240C, 0x240D,
  0x240A, 0x00B0, 0x00B1, 0x2424, 0x240B,
  0x2518, 0x2510, 0x250C, 0x2514, 0x253C,
  0x23BA, 0x23BB, 0x2500, 0x23BC, 0x23BD,
  0x251C, 0x2524, 0x2534, 0x252C, 0x2502,
  0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00B7
};

static size_t
vt100_decode (const struct avt_charenc *self, avt_char * ch, const char *s)
{
  (void) self;

  if (*s < 95)
    *ch = (avt_char) * s;
  else if (*s <= 126)
    *ch = vt100trans[*s - 95];
  else
    *ch = BROKEN_WCHAR;

  return 1;
}

static const struct avt_charenc vt100_converter = {
  .data = (void *) &vt100trans,	// not used
  .decode = vt100_decode,
  .encode = NULL
};

// handler for APC commands
static avta_term_apc_cmd apc_cmd_handler;

static inline void
set_encoding (const struct avt_charenc *encoding)
{
  convert = encoding;
  avt_char_encoding (encoding);
}

extern void
avta_term_nocolor (bool on)
{
  nocolor = on;
}

extern void
avta_term_slowprint (bool on)
{
  if (on)
    text_delay = AVT_DEFAULT_TEXT_DELAY;
  else
    text_delay = 0;

  avt_set_text_delay (text_delay);
}

static void
activate_cursor (bool on)
{
  cursor_active = on;
  avt_activate_cursor (cursor_active);
}

extern void
avta_term_update_size (void)
{
  if (prg_input > 0)
    {
      int new_max_x = avt_get_max_x ();
      int new_max_y = avt_get_max_y ();

      if (new_max_x != max_x or new_max_y != max_y)
	{
	  max_x = new_max_x;
	  max_y = new_max_y;

	  avta_term_size (prg_input, max_y, max_x);

	  if (region_max_y > max_y)
	    region_max_y = max_y;
	  if (region_min_y > max_y)
	    region_min_y = 1;
	}
    }
}

/*
static void
prg_mousehandler (int button, bool pressed, int x, int y)
{
  if (button <= 3 and ((mouse_mode == 1 and pressed) or mouse_mode == 2))
    {
      char code[7];
      int b;

      if (mouse_mode == 2 and not pressed)
	b = 3;			// button released
      else
	b = button - 1;

      snprintf (code, sizeof (code), CSI "M%c%c%c",
		(char) (040 + b), (char) (040 + x), (char) (040 + y));
      avta_term_send (&code[0], sizeof (code) - 1);
    }
}
*/

extern void
avta_term_send (const char *buf, size_t count)
{
  if (prg_input > 0)
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
      while (r > 0 and count > 0);
    }
}

#define send_cursor_seq(c)  \
  do { dec_cursor_seq[2]=c; avta_term_send(dec_cursor_seq, 3); } while(0)

static void
process_key (avt_char key)
{
  // TODO: support application_keypad

  if (prg_input <= 0)
    return;

  switch (key)
    {
    case AVT_KEY_UP:
      send_cursor_seq ('A');
      break;

    case AVT_KEY_DOWN:
      send_cursor_seq ('B');
      break;

    case AVT_KEY_RIGHT:
      send_cursor_seq ('C');
      break;

    case AVT_KEY_LEFT:
      send_cursor_seq ('D');
      break;

    case AVT_KEY_INSERT:
      avta_term_send_literal (KEY_INSERT);
      break;

    case AVT_KEY_HOME:
      avta_term_send_literal (KEY_HOME);
      break;

    case AVT_KEY_END:
      avta_term_send_literal (KEY_END);
      break;

    case AVT_KEY_PAGEUP:
      avta_term_send_literal (KEY_PAGEUP);
      break;

    case AVT_KEY_PAGEDOWN:
      avta_term_send_literal (KEY_PAGEDOWN);
      break;

    case AVT_KEY_F1:
      avta_term_send_literal (KEY_F1);
      break;

    case AVT_KEY_F2:
      avta_term_send_literal (KEY_F1);
      break;

    case AVT_KEY_F3:
      avta_term_send_literal (KEY_F3);
      break;

    case AVT_KEY_F4:
      avta_term_send_literal (KEY_F4);
      break;

    case AVT_KEY_F5:
      avta_term_send_literal (KEY_F5);
      break;

    case AVT_KEY_F6:
      avta_term_send_literal (KEY_F6);
      break;

    case AVT_KEY_F7:
      avta_term_send_literal (KEY_F7);
      break;

    case AVT_KEY_F8:
      avta_term_send_literal (KEY_F8);
      break;

    case AVT_KEY_F9:
      avta_term_send_literal (KEY_F9);
      break;

    case AVT_KEY_F10:
      avta_term_send_literal (KEY_F10);
      break;

    case AVT_KEY_F11:
      avta_term_send_literal (KEY_F11);
      break;

    case AVT_KEY_F12:
      avta_term_send_literal (KEY_F12);
      break;

    case AVT_KEY_F13:
      avta_term_send_literal (KEY_F13);
      break;

    case AVT_KEY_F14:
      avta_term_send_literal (KEY_F14);
      break;

    case AVT_KEY_F15:
      avta_term_send_literal (KEY_F15);
      break;

    default:
      if (key)
	{
	  char mbstring[16];
	  size_t length;

	  length =
	    convert->encode (convert, mbstring, sizeof (mbstring), key);
	  if (length > 0)
	    avta_term_send (mbstring, length);
	}			// if (key)
    }				// switch
}


#define clear_textbuffer(void)  get_character(-1)

static avt_char
get_character (int fd)
{
  static char filebuf[INBUFSIZE + 1];
  static size_t filebuf_pos = 0;
  static size_t filebuf_len = 0;

  if (fd == -1)			// clear textbuffer
    {
      filebuf_pos = filebuf_len = 0;
      return AVT_EOF;
    }

  // need to get more characters?
  if (filebuf_pos >= filebuf_len
      or (filebuf_pos >= sizeof (filebuf) - MB_LEN_MAX))
    {
      // update
      if (text_delay == 0)
	avt_lock_updates (false);

      size_t offset = 0;
      char *p = filebuf;

      // move trailing rest to beginning
      if (filebuf_len > filebuf_pos)
	{
	  offset = filebuf_len - filebuf_pos;
	  memmove (filebuf, filebuf + filebuf_pos, offset);
	  p += offset;
	}

      ssize_t nread = read (fd, p, sizeof (filebuf) - offset);

      // waiting for data
      if (nread == -1 and errno == EAGAIN)
	{
	  if (cursor_active)
	    avt_activate_cursor (true);

	  do
	    {
	      nread = read (fd, p, sizeof (filebuf) - offset);

	      while (avt_key_pressed ())
		process_key (avt_get_key ());
	    }
	  while (nread == -1 and errno == EAGAIN
		 and avt_update () == AVT_NORMAL);

	  if (cursor_active)
	    avt_activate_cursor (false);
	}

      if (nread < 0)
	{
	  filebuf_len = filebuf_pos = 0;
	  return AVT_EOF;
	}

      filebuf_len = offset + nread;
      filebuf_pos = 0;

      if (text_delay == 0)
	avt_lock_updates (true);
    }

  avt_char ch;
  size_t num = convert->decode (convert, &ch, filebuf + filebuf_pos);
  filebuf_pos += num;

  return ch;
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
    faint = false;

  switch (color)
    {
    case 0:
      if (not faint)
	avt_set_text_color (0x000000);
      else
	avt_set_text_color (0x888888);
      break;
    case 1:			// red
      avt_set_text_color (0x880000);
      break;
    case 2:			// green
      avt_set_text_color (0x008800);
      break;
    case 3:			// brown
      avt_set_text_color (0x888800);
      break;
    case 4:			// blue
      avt_set_text_color (0x000088);
      break;
    case 5:			// magenta
      avt_set_text_color (0x880088);
      break;
    case 6:			// cyan
      avt_set_text_color (0x008888);
      break;
    case 7:			// lightgray
      avt_set_text_color (0xCCCCCC);
      break;
    case 8:			// darkgray
      avt_set_text_color (0x888888);
      break;
    case 9:			// lightred
      avt_set_text_color (0xFF0000);
      break;
    case 10:			// lightgreen
      avt_set_text_color (0x00FF00);
      break;
    case 11:			// yellow
      avt_set_text_color (0xFFFF00);
      break;
    case 12:			// lightblue
      avt_set_text_color (0x0000FF);
      break;
    case 13:			// lightmagenta
      avt_set_text_color (0xFF00FF);
      break;
    case 14:			// lightcyan
      avt_set_text_color (0x00FFFF);
      break;
    case 15:			// white
      avt_set_text_color (0xFFFFFF);
    }
}

static void
set_background_color (int color)
{
  switch (color)
    {
    case 0:			// black
      avt_set_text_background_color (0x000000);
      break;
    case 1:			// red
      avt_set_text_background_color (0x880000);
      break;
    case 2:			// green
      avt_set_text_background_color (0x008800);
      break;
    case 3:			// brown
      avt_set_text_background_color (0x888800);
      break;
    case 4:			// blue
      avt_set_text_background_color (0x000088);
      break;
    case 5:			// magenta
      avt_set_text_background_color (0x880088);
      break;
    case 6:			// cyan
      avt_set_text_background_color (0x008888);
      break;
    case 7:			// lightgray
      avt_set_text_background_color (0xCCCCCC);
      break;
    case 8:			// darkgray
      avt_set_text_background_color (0x888888);
      break;
    case 9:			// lightred
      avt_set_text_background_color (0xFF0000);
      break;
    case 10:			// lightgreen
      avt_set_text_background_color (0x00FF00);
      break;
    case 11:			// yellow
      avt_set_text_background_color (0xFFFF00);
      break;
    case 12:			// lightblue
      avt_set_text_background_color (0x0000FF);
      break;
    case 13:			// lighmagenta
      avt_set_text_background_color (0xFF00FF);
      break;
    case 14:			// lightcyan
      avt_set_text_background_color (0x00FFFF);
      break;
    case 15:			// ballooncolor
      avt_set_text_background_ballooncolor ();
    }
}

static void
ansi_graphic_code (int mode)
{
  switch (mode)
    {
    case 0:			// normal
      faint = false;
      text_color = 0;
      text_background_color = 0xF;
      avt_normal_text ();
      set_foreground_color (text_color);
      set_background_color (text_background_color);
      break;

    case 1:			// bold
      faint = false;
      avt_bold (true);
      // bold is sometimes assumed to light colors
      if (text_color > 0 and text_color < 7)
	{
	  text_color += 8;
	  set_foreground_color (text_color);
	}
      break;

    case 2:			// faint
      avt_bold (false);
      faint = true;
      if (text_color == 0)
	set_foreground_color (text_color);
      break;

    case 4:			// underlined
    case 21:			// double underlined (ambiguous)
      avt_underlined (true);
      break;

    case 5:			// blink
      // unsupported
      break;

    case 22:			// normal intensity
      avt_bold (false);
      faint = false;
      break;

    case 24:			// not underlined
      avt_underlined (false);
      break;

    case 25:			// blink off
      // unsupported
      break;

    case 7:			// inverse
      avt_inverse (true);
      break;

    case 27:			// not inverse
      avt_inverse (false);
      break;

    case 8:			// hidden
    case 9:
      set_foreground_color (text_background_color);
      break;

    case 28:			// not hidden
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
      if (not nocolor)
	{
	  text_color = (mode - 30);
	  // bold is sometimes assumed to be in light color
	  if (text_color > 0 and text_color < 7 and avt_get_bold ())
	    text_color += 8;
	  set_foreground_color (text_color);
	}
      break;

    case 38:			// foreground normal, underlined
      text_color = 0;
      set_foreground_color (text_color);
      avt_underlined (true);
      break;

    case 39:			// foreground normal
      text_color = 0;
      set_foreground_color (text_color);
      avt_underlined (false);
      break;

    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
      if (not nocolor)
	{
	  text_background_color = (mode - 40);
	  set_background_color (text_background_color);
	}
      break;

    case 49:			// background normal
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
      if (not nocolor)
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
      if (not nocolor)
	{
	  text_background_color = (mode - 100 + 8);
	  set_background_color (text_background_color);
	}
      break;
    }
}

// avoid anything that draws the balloon here
static void
reset_terminal (void)
{
  clear_textbuffer ();

  region_min_y = 1;
  region_max_y = max_y;
  insert_mode = false;

  avt_reserve_single_keys (true);
  avt_newline_mode (false);
  avt_set_auto_margin (true);
  activate_cursor (true);
  avt_set_scroll_mode (1);
  G0 = avt_iso8859_1 ();
  G1 = &vt100_converter;
  set_encoding (default_encoding);	// not G0!

  // like vt102
  avt_set_origin_mode (false);

  avt_reset_tab_stops ();
  ansi_graphic_code (0);
  avta_term_slowprint (false);
  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
}

// full reset - eventually draws the balloon
static void
full_reset (void)
{
  reset_terminal ();
  avt_viewport (1, region_min_y, max_x, region_max_y);
  avt_clear ();
  avt_save_position ();
}

// Esc [ ...
// CSI
static void
CSI_sequence (int fd, avt_char last_character)
{
  avt_char ch;
  char sequence[80];
  unsigned int pos = 0;

  do
    {
      ch = get_character (fd);

      // ignore CSI [ + one character
      if (pos == 0 and ch == L'[')
	{
	  get_character (fd);	// ignore one char
	  return;
	}

      // CAN or SUB cancel the escape-sequence
      if (ch == L'\x18' or ch == L'\x1A')
	return;

      sequence[pos] = (char) ch;
      pos++;
    }
  while (pos < sizeof (sequence) and ch < L'@');
  sequence[pos] = '\0';
  pos++;

#ifdef DEBUG
  fprintf (stderr, "CSI %s\n", sequence);
#endif

  // ch has last character in the sequence
  switch (ch)
    {
    case L'@':			// ICH
      if (sequence[0] == '@')
	avt_insert_spaces (1);
      else
	avt_insert_spaces (strtol (sequence, NULL, 10));
      break;

    case L'A':			// CUU
      if (sequence[0] == 'A')
	avt_move_y (avt_where_y () - 1);
      else
	avt_move_y (avt_where_y () - strtol (sequence, NULL, 10));
      break;

    case L'a':			// HPR
      if (sequence[0] == 'a')
	avt_move_x (avt_where_x () + 1);
      else
	avt_move_x (avt_where_x () + strtol (sequence, NULL, 10));
      break;

    case L'B':			// CUD
      if (sequence[0] == 'B')
	avt_move_y (avt_where_y () + 1);
      else
	avt_move_y (avt_where_y () + strtol (sequence, NULL, 10));
      break;

    case L'b':			// REP
      if (sequence[0] == 'b')
	avt_put_char (last_character);
      else
	{
	  int count = strtol (sequence, NULL, 10);
	  for (int i = 0; i < count; i++)
	    avt_put_char (last_character);
	}
      break;

    case L'C':			// CUF
      if (sequence[0] == 'C')
	avt_move_x (avt_where_x () + 1);
      else
	avt_move_x (avt_where_x () + strtol (sequence, NULL, 10));
      break;

    case L'c':			// DA
      if (sequence[0] == 'c')
	avta_term_send_literal (DS);
      else if (sequence[0] == '?')
	{			// I have no real infos about that :-(
	  if (sequence[1] == '1' and sequence[2] == 'c')
	    activate_cursor (false);
	  else if (sequence[1] == '2' and sequence[2] == 'c')
	    activate_cursor (true);
	  else if (sequence[1] == '0' and sequence[2] == 'c')
	    activate_cursor (true);	// normal?
	  else if (sequence[1] == '8' and sequence[2] == 'c')
	    activate_cursor (true);	// very visible
	}
      break;

    case L'D':			// CUB
      if (sequence[0] == 'D')
	avt_move_x (avt_where_x () - 1);
      else
	avt_move_x (avt_where_x () - strtol (sequence, NULL, 10));
      break;

    case L'd':			// VPA
      if (sequence[0] == 'd')
	avt_move_y (1);
      else
	avt_move_y (strtol (sequence, NULL, 10));
      break;

    case L'E':			// CNL
      avt_move_x (1);
      if (sequence[0] == 'E')
	avt_move_y (avt_where_y () + 1);
      else
	avt_move_y (avt_where_y () + strtol (sequence, NULL, 10));
      break;

    case L'e':			// VPR
      if (sequence[0] == 'e')
	avt_move_y (avt_where_y () + 1);
      else
	avt_move_y (avt_where_y () + strtol (sequence, NULL, 10));
      break;

    case L'F':			// CPL
      avt_move_x (1);
      if (sequence[0] == 'F')
	avt_move_y (avt_where_y () - 1);
      else
	avt_move_y (avt_where_y () - strtol (sequence, NULL, 10));
      break;

      // L'f', HVP: see H

    case L'g':			// TBC
      if (sequence[0] == 'g' or sequence[0] == '0')
	avt_set_tab (avt_where_x (), false);
      else			// TODO: TBC 1-5 are not distinguished here
	avt_clear_tab_stops ();
      break;

    case L'G':			// CHA
      if (sequence[0] == 'G')
	avt_move_x (1);
      else
	avt_move_x (strtol (sequence, NULL, 10));
      break;

    case L'H':			// CUP
    case L'f':			// HVP
      if (sequence[0] == 'H' or sequence[0] == 'f')
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

    case L'h':			// DECSET
    case L'l':			// DECRST
      if (sequence[0] == '?')
	{
	  int val = strtol (&sequence[1], NULL, 10);
	  bool on = (ch == L'h');

	  switch (val)
	    {
	    case 1:
	      dec_cursor_seq[1] = on ? 'O' : '[';
	      break;
	    case 5:		// hack: non-standard!
	      // by definition it is reverse video
	      if (on)
		avt_flash ();
	      break;
	    case 6:
	      avt_set_origin_mode (on);
	      break;
	    case 7:
	      avt_set_auto_margin (on);
	      break;
	    case 9:		// X10 mouse
	      mouse_mode = on ? 1 : 0;
	      avt_set_mouse_visible (on);
	      break;
	    case 1000:		// X11 mouse
	      mouse_mode = on ? 2 : 0;
	      avt_set_mouse_visible (on);
	      break;
	    case 25:
	      activate_cursor (on);
	      break;
	    case 56:		// AKFAvatar extension
	      avta_term_slowprint (on);
	      break;
	    case 66:
	      application_keypad = on;
	      break;
	    case 47:		// Use Alternate Screen Buffer
	      // Workaround - no Alternate Screen Buffer supported
	      full_reset ();
	      break;
	    }
	}
      else
	{
	  int val = strtol (sequence, NULL, 10);
	  bool on = (ch == L'h');

	  switch (val)
	    {
	    case 4:
	      insert_mode = on;
	      break;
	    case 20:
	      avt_newline_mode (on);
	      break;
	    }
	}
      break;

    case L'J':			// ED
      if (sequence[0] == '0' or sequence[0] == 'J')
	avt_clear_down ();
      else if (sequence[0] == '1')
	avt_clear_up ();
      else if (sequence[0] == '2')
	avt_clear ();
      break;

    case L'K':			// EL
      if (sequence[0] == '0' or sequence[0] == 'K')
	avt_clear_eol ();
      else if (sequence[0] == '1')
	avt_clear_bol ();
      else if (sequence[0] == '2')
	avt_clear_line ();
      break;

    case L'm':			// SGR
      if (sequence[0] == 'm')
	ansi_graphic_code (0);
      else
	{
	  char *next;

	  next = &sequence[0];
	  while (*next >= '0' and * next <= '9')
	    {
	      ansi_graphic_code (strtol (next, &next, 10));
	      if (*next == ';')
		next++;
	    }
	}
      break;

    case L'L':			// IL
      if (sequence[0] == 'L')
	avt_insert_lines (avt_where_y (), 1);
      else
	avt_insert_lines (avt_where_y (), strtol (sequence, NULL, 10));
      break;

    case L'M':			// DL
      if (sequence[0] == 'M')
	avt_delete_lines (avt_where_y (), 1);
      else
	avt_delete_lines (avt_where_y (), strtol (sequence, NULL, 10));
      break;

    case L'n':			// DSR
      if (sequence[0] == '5' and sequence[1] == 'n')
	avta_term_send_literal (CSI "0n");	// device okay
      // CSI "3n" for failure
      else if (sequence[0] == '6' and sequence[1] == 'n')
	{
	  // report cursor position
	  char s[80];
	  snprintf (s, sizeof (s), CSI "%d;%dR",
		    avt_where_x (), avt_where_y ());
	  avta_term_send (s, strlen (s));
	}
      // other values are unknown
      break;

    case L'P':			// DCH
      if (sequence[0] == 'P')
	avt_delete_characters (1);
      else
	avt_delete_characters (strtol (sequence, NULL, 10));
      break;

    case L'r':			// CSR
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
	  else			// origin_mode not set
	    {
	      region_min_y = min;
	      region_max_y = max;
	    }
	}
      break;

    case L's':			// SCP
      avt_save_position ();
      break;

      // AKFAvatar extension - set balloon size (comp. to xterm)
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

    case L'u':			// RCP
      avt_restore_position ();
      break;

    case L'X':			// ECH
      if (sequence[0] == 'X')
	avt_erase_characters (1);
      else
	avt_erase_characters (strtol (sequence, NULL, 10));
      break;

    case L'Z':			// CBT
      if (sequence[0] == 'Z')
	avt_last_tab ();
      else
	{
	  int count = strtol (sequence, NULL, 10);
	  for (int i = 0; i < count; i++)
	    avt_last_tab ();
	}
      break;

    case L'`':			// HPA
      if (sequence[0] == '`')
	avt_move_x (1);
      else
	avt_move_x (strtol (sequence, NULL, 10));
      break;

#ifdef DEBUG
    default:
      fprintf (stderr, "unsupported CSI sequence: %s", sequence);
#endif
    }
}

// APC: Application Program Command
static void
APC_sequence (int fd)
{
  avt_char ch, old;
  wchar_t command[1024];
  unsigned int p;

  p = 0;
  ch = old = L'\0';

  // skip until "\a" or "\x9c" or "\033\\" is found
  while (p < (sizeof (command) / sizeof (wchar_t))
	 and ch != L'\a' and ch != L'\x9c' and (old != L'\033' or ch !=
						L'\\'))
    {
      old = ch;
      ch = get_character (fd);
      if (ch >= L' ' and ch != L'\x9c' and (old != L'\033' or ch != L'\\'))
	command[p++] = (wchar_t) ch;
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
  avt_char ch, old;

  ch = get_character (fd);

  if (ch == L'P')		// set palette (Linux)
    {
      // ignore 7 characters
      for (int i = 0; i < 7; i++)
	get_character (fd);
      return;
    }

  // ignore L'R' reset palette (Linux)
  if (ch == L'R')
    return;

  // skip until "\a" or "\x9c" or "\033\\" is found
  do
    {
      old = ch;
      ch = get_character (fd);
    }
  while (ch != L'\a' and ch != L'\x9c' and (old != L'\033' or ch != L'\\'));
}

static void
escape_sequence (int fd, avt_char last_character)
{
  avt_char ch;
  static int saved_text_color, saved_text_background_color;
  static bool saved_underline_state, saved_bold_state;
  static const struct avt_charenc *saved_G0, *saved_G1;

  ch = get_character (fd);

#ifdef DEBUG
  if (ch != L'[')
    fprintf (stderr, "ESC %lc\n", ch);
#endif

  // ESC [ch]
  switch (ch)
    {
    case L'\x18':		// CAN
    case L'\x1A':		// SUB
      // cancel escape sequence
      break;

    case L'7':			// DECSC
      avt_save_position ();
      saved_text_color = text_color;
      saved_text_background_color = text_background_color;
      saved_underline_state = avt_get_underlined ();
      saved_bold_state = avt_get_bold ();
      saved_G0 = G0;
      saved_G1 = G1;
      break;

    case L'8':			// DECRC
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

    case L'%':			// select charset
      {
	avt_char ch2;

	ch2 = get_character (fd);
	if (ch2 == L'@')
	  set_encoding (G0);	// unsure
	else if (ch2 == L'G' or ch2 == L'8' /* obsolete */ )
	  set_encoding (avt_utf8 ());
	/* else if (ch2 == L'B')
	   set_encoding ("UTF-1"); */// does anybody use that?
      }
      break;

      // Note: this is not compatible to ISO 2022!
    case L'(':			// set G0
      {
	avt_char ch2;

	ch2 = get_character (fd);
	if (ch2 == L'B')
	  G0 = avt_iso8859_1 ();
	else if (ch2 == L'0')
	  G0 = &vt100_converter;
	else if (ch2 == L'U')
	  G0 = avt_cp437 ();
	else if (ch2 == L'K')
	  G0 = avt_utf8 ();	// "user-defined"
      }
      break;

    case L')':			// set G1
      {
	avt_char ch2;

	ch2 = get_character (fd);
	if (ch2 == L'B')
	  G1 = avt_iso8859_1 ();
	else if (ch2 == L'0')
	  G1 = &vt100_converter;
	else if (ch2 == L'U')
	  G1 = avt_cp437 ();
	else if (ch2 == L'K')
	  G1 = avt_utf8 ();	// "user-defined"
      }
      break;

    case L'[':			// CSI
      CSI_sequence (fd, last_character);
      break;

    case L'c':			// RIS - reset device
      full_reset ();
      saved_text_color = text_color;
      saved_text_background_color = text_background_color;
      break;

    case L'D':			// move down or scroll up one line
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

    case L'H':			// HTS
      avt_set_tab (avt_where_x (), true);
      break;

    case L'M':			// RI - scroll down one line
      if (avt_where_y () > region_min_y)
	avt_move_y (avt_where_y () - 1);
      else
	avt_insert_lines (region_min_y, 1);
      break;

    case L'Z':			// DECID
      avta_term_send_literal (DS);
      break;

      // OSC: Operating System Command
    case L']':
      OSC_sequence (fd);
      break;

      // APC: Application Program Command
    case L'_':
      APC_sequence (fd);
      break;

    case L'>':			// DECPNM
      application_keypad = false;
      break;

    case L'=':			// DECPAM
      application_keypad = true;
      break;

#ifdef DEBUG
    default:
      fprintf (stderr, "unsupported escape sequence %lc\n", ch);
      break;
#endif
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
  bool stop;
  avt_char ch;
  avt_char last_character;

  // check, if fd is valid
  if (fd < 0)
    return;

  last_character = 0x0000;
  stop = false;

  dec_cursor_seq[0] = '\033';
  dec_cursor_seq[1] = '[';
  dec_cursor_seq[2] = ' ';	// to be filled later
  avt_set_mouse_visible (false);	// TODO: just wheel supported

  reset_terminal ();
  avt_lock_updates (true);

  while ((ch = get_character (fd)) != AVT_EOF and not stop)
    {
      if (ch == L'\033')	// Esc
	escape_sequence (fd, last_character);
      else if (ch == L'\x9b')	// CSI
	CSI_sequence (fd, last_character);
      else if (ch == L'\x9d')	// OSC
	OSC_sequence (fd);
      else if (ch == L'\x9f')	// APC
	APC_sequence (fd);
      else if (ch == L'\x0E')	// SO
	set_encoding (G1);
      else if (ch == L'\x0F')	// SI
	set_encoding (G0);
      else
	{
	  if (insert_mode)
	    avt_insert_spaces (1);

	  last_character = (avt_char) ch;

	  stop = avt_put_char ((avt_char) ch);
	}
    }

  avta_closeterm (fd);

  activate_cursor (false);
  avt_reserve_single_keys (false);
  avt_newline_mode (true);
  avt_lock_updates (false);

  prg_input = -1;
}


extern int
avta_term_start (const char *working_dir, char *prg_argv[])
{
  default_encoding = avt_systemencoding ();
  set_encoding (default_encoding);
  clear_textbuffer ();

  max_x = avt_get_max_x ();
  max_y = avt_get_max_y ();
  region_min_y = 1;
  region_max_y = max_y;
  insert_mode = false;

  // check if AKFAvatar was initialized
  if (max_x < 0 or max_y < 0)
    return -1;

  return
    avta_term_initialize (&prg_input, max_x, max_y, nocolor,
			  working_dir, prg_argv);
}
