/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99, POSIX.1-2001
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

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "avtdata.h"
#include "version.h"
#include "rgb.h"		// only for DEFAULT_COLOR

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <iso646.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>

// include images
#include "btn.xpm"
#include "balloonpointer.xbm"
#include "thinkpointer.xbm"
#include "round_upper_left.xbm"
#include "round_upper_right.xbm"
#include "round_lower_left.xbm"
#include "round_lower_right.xbm"
#include "btn_yes.xbm"
#include "btn_no.xbm"
#include "btn_right.xbm"
#include "btn_left.xbm"
#include "btn_up.xbm"
#include "btn_down.xbm"
#include "btn_cancel.xbm"
#include "btn_ff.xbm"
#include "btn_fb.xbm"
#include "btn_stop.xbm"
#include "btn_pause.xbm"
#include "btn_help.xbm"
#include "btn_eject.xbm"
#include "btn_circle.xbm"

#define COPYRIGHTYEAR "2013"
#define WIDE_COPYRIGHTYEAR L"1013"

#if MINIMALWIDTH < 800
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 10
#  define AVATAR_MARGIN 10
   // Delay for moving in or out - the higher, the slower
#  define MOVE_DELAY 2.5
#else // MINIMALWIDTH >= 800
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 15
#  define AVATAR_MARGIN 20
   // Delay for moving in or out - the higher, the slower
#  define MOVE_DELAY 1.8
#endif // MINIMALWIDTH >= 800

#define BASE_BUTTON_WIDTH 32
#define BASE_BUTTON_HEIGHT 32

#define BUTTON_DISTANCE 10

#define AVT_BUTTON_COLOR        0x665533
#define AVT_BALLOON_COLOR       AVT_COLOR_FLORAL_WHITE

#define NOT_BOLD   false

#define AVT_XBM_INFO(img)  img##_bits, img##_width, img##_height

#define SHADOWOFFSET 5

// for static linking avoid to drag in unneeded object files
#pragma GCC poison  avt_colorname avt_palette avt_colors
#pragma GCC poison  avt_avatar_image_default

#define BALLOONPOINTER_OFFSET 20

// Note: any event should have an associated key, so key == event

#define AVT_KEYBUFFER_SIZE  512

struct avt_position
{
  int x, y;
};

struct avt_area
{
  int x, y, width, height;
};

struct avt_settings
{
  avt_graphic *avatar_image;
  avt_graphic *cursor_character;
  wchar_t *name;

  void (*quit_audio) (void);
  void (*quit_encoding) (void);
  void (*bell) (void);

  // delay values for printing text and flipping the page
  int text_delay, flip_page_delay;

  avt_color ballooncolor;
  avt_color background_color;
  avt_color text_color;
  avt_color text_background_color;
  avt_color bitmap_color;	// color for bitmaps

  bool newline_mode;		// when off, you need an extra CR
  bool underlined, bold, inverse;	// text underlined, bold?
  bool auto_margin;		// automatic new lines?
  bool avatar_visible;		// avatar visible?
  bool text_cursor_visible;	// shall the text cursor be visible?
  bool text_cursor_actually_visible;	// is it actually visible?
  bool markup;			// markup-syntax activated?
  bool hold_updates;		// holding updates back?
  bool tab_stops[AVT_LINELENGTH];

  // origin mode
  // Home: textfield (false) or viewport (true)
  // avt_initialize sets it to true for backwards compatibility
  bool origin_mode;

  short int avatar_mode;
  short int scroll_mode;
  short int textdir_rtl;
  short int linestart;		// beginning of line - depending on text direction
  short int balloonheight, balloonmaxheight, balloonwidth;

  struct avt_position cursor, saved_position;

  struct avt_area window;	// if screen is in fact larger
  struct avt_area textfield;
  struct avt_area viewport;	// sub-window in textfield
};

struct avt_key_buffer
{
  unsigned short int position, end;
  avt_char buffer[AVT_KEYBUFFER_SIZE];
};

enum avt_button_type
{
  btn_cancel, btn_yes, btn_no, btn_right, btn_left, btn_down, btn_up,
  btn_fastforward, btn_fastbackward, btn_stop, btn_pause, btn_help,
  btn_eject, btn_circle
};

static avt_graphic *screen;
static avt_graphic *base_button;
static avt_graphic *raw_image;
static int fontwidth, fontheight, fontunderline;
static struct avt_key_buffer avt_keys;

static struct avt_backend backend;

static struct avt_settings avt = {
  .background_color = DEFAULT_COLOR,
  .ballooncolor = AVT_BALLOON_COLOR,
  .textdir_rtl = AVT_LEFT_TO_RIGHT
};

struct avt_button
{
  short int x, y;
  avt_char key;
  avt_graphic *background;
};

#define MAX_BUTTONS 15

static struct avt_button avt_buttons[MAX_BUTTONS];

// 0 = normal; 1 = quit-request; -1 = error
int _avt_STATUS;

// forward declaration
static void avt_drawchar (avt_char ch, avt_graphic * surface);
//-----------------------------------------------------------------------------

extern void
avt_bell_function (void (*f) (void))
{
  if (f != avt_bell)
    avt.bell = f;
  else				// error
    {
      avt_set_error ("bell function cannot be avt_bell (recursive call)");
      _avt_STATUS = AVT_ERROR;
    }
}

extern void
avt_quit_encoding_function (void (*f) (void))
{
  avt.quit_encoding = f;
}

extern void
avt_quit_audio_function (void (*f) (void))
{
  avt.quit_audio = f;
}

// add key into buffer
extern void
avt_add_key (avt_char key)
{
  int new_end;

  new_end = (avt_keys.end + 1) % AVT_KEYBUFFER_SIZE;

  // if buffer is not full
  if (new_end != avt_keys.position)
    {
      avt_keys.buffer[avt_keys.end] = key;
      avt_keys.end = new_end;
    }
}

#ifndef DISABLE_DEPRECATED

extern int
avt_key (avt_char * ch)
{
  backend.wait_key ();

  if (ch)
    *ch = avt_keys.buffer[avt_keys.position];

  avt_keys.position = (avt_keys.position + 1) % AVT_KEYBUFFER_SIZE;

  return _avt_STATUS;
}

#endif

extern avt_char
avt_get_key (void)
{
  avt_char ch;

  ch = AVT_KEY_NONE;
  backend.wait_key ();

  // wait_key might also return on error or a quit request
  // whithout adding something to the key buffer...

  if (avt_keys.position != avt_keys.end)
    {
      ch = avt_keys.buffer[avt_keys.position];
      avt_keys.position = (avt_keys.position + 1) % AVT_KEYBUFFER_SIZE;
    }

  return ch;
}

extern bool
avt_key_pressed (void)
{
  return (avt_keys.position != avt_keys.end);
}

extern void
avt_clear_keys (void)
{
  avt_keys.position = avt_keys.end = 0;
}

/*
 * fills the screen with the background color,
 * but doesn't update the screen yet
 */
static inline void
avt_free_screen (void)
{
  avt_fill (screen, avt.background_color);
}

static inline avt_graphic *
avt_get_window (void)
{
  return avt_get_area (screen, avt.window.x, avt.window.y,
		       avt.window.width, avt.window.height);
}

static inline void
avt_update_all (void)
{
  backend.update_area (screen, 0, 0, screen->width, screen->height);
}

static inline void
avt_update_window (void)
{
  backend.update_area (screen, avt.window.x, avt.window.y,
		       avt.window.width, avt.window.height);
}

static inline void
avt_update_textfield (void)
{
  if (not avt.hold_updates and avt.textfield.x >= 0)
    backend.update_area (screen, avt.textfield.x, avt.textfield.y,
			 avt.textfield.width, avt.textfield.height);
}

static inline void
avt_update_viewport (void)
{
  if (not avt.hold_updates and avt.viewport.x >= 0)
    backend.update_area (screen, avt.viewport.x, avt.viewport.y,
			 avt.viewport.width, avt.viewport.height);
}

// recalculate positions after screen has been resized
static void
avt_resized (void)
{
  struct avt_area oldwindow;

  oldwindow = avt.window;

  // new position of the window on the screen
  if (screen->width > avt.window.width)
    avt.window.x = (screen->width / 2) - (avt.window.width / 2);
  else
    avt.window.x = 0;

  if (screen->height > avt.window.height)
    avt.window.y = (screen->height / 2) - (avt.window.height / 2);
  else
    avt.window.y = 0;

  // recalculate textfield & viewport positions
  if (avt.textfield.x >= 0)
    {
      avt.textfield.x = avt.textfield.x - oldwindow.x + avt.window.x;
      avt.textfield.y = avt.textfield.y - oldwindow.y + avt.window.y;

      avt.viewport.x = avt.viewport.x - oldwindow.x + avt.window.x;
      avt.viewport.y = avt.viewport.y - oldwindow.y + avt.window.y;

      if (avt.textdir_rtl)
	avt.linestart = avt.viewport.x + avt.viewport.width - fontwidth;
      else
	avt.linestart = avt.viewport.x;

      avt.cursor.x = avt.cursor.x - oldwindow.x + avt.window.x;
      avt.cursor.y = avt.cursor.y - oldwindow.y + avt.window.y;
    }
}

extern void
avt_resize (int width, int height)
{
  if (backend.resize)
    {
      avt_graphic *oldwindowimage;

      // minimal size
      if (width < MINIMALWIDTH)
	width = MINIMALWIDTH;
      if (height < MINIMALHEIGHT)
	height = MINIMALHEIGHT;

      // save the window
      oldwindowimage = avt_get_window ();

      backend.resize (screen, width, height);

      // recalculate positions
      avt_resized ();

      // restore image in new position
      avt_free_screen ();
      avt_put_graphic (oldwindowimage, screen, avt.window.x, avt.window.y);
      avt_free_graphic (oldwindowimage);

      // make all changes visible
      avt_update_all ();
    }
}

static inline void
avt_release_raw_image (void)
{
  if (raw_image)
    {
      avt_free_graphic (raw_image);
      raw_image = NULL;
    }
}

static inline void
bell (void)
{
  if (avt.bell)
    {
      void (*temp_bell) (void);

      // block recursion
      temp_bell = avt.bell;
      avt.bell = NULL;
      temp_bell ();
      avt.bell = temp_bell;
    }
}

static int
calculate_balloonmaxheight (void)
{
  int avatar_height;

  avatar_height = 0;

  if (avt.avatar_image)
    avatar_height = avt.avatar_image->height + AVATAR_MARGIN;

  avt.balloonmaxheight = (avt.window.height - avatar_height - (2 * TOPMARGIN)
			  - (2 * BALLOON_INNER_MARGIN)) / fontheight;

  // check, whether image is too high
  // at least 10 lines
  if (avt.balloonmaxheight < 10)
    {
      avt_set_error ("Avatar image too large");
      _avt_STATUS = AVT_ERROR;
      avt_free_graphic (avt.avatar_image);
      avt.avatar_image = NULL;
    }

  return _avt_STATUS;
}

// set the inner window for the avatar-display
static void
avt_avatar_window (void)
{
  avt.window.x = 0;
  avt.window.y = 0;
  avt.window.width = MINIMALWIDTH;
  avt.window.height = MINIMALHEIGHT;

  // window may be smaller than the screen
  if (screen->width > MINIMALWIDTH)
    avt.window.x = (screen->width / 2) - (MINIMALWIDTH / 2);

  if (screen->height > MINIMALHEIGHT)
    avt.window.y = (screen->height / 2) - (MINIMALHEIGHT / 2);

  calculate_balloonmaxheight ();
}

/****************************************************************************/
// image loaders

static avt_graphic *
avt_load_image_avtdata (avt_data * data)
{
  avt_graphic *image;

  if (not data)
    return NULL;

  image = avt_load_image_xpm_data (data);

  if (not image)
    image = avt_load_image_xbm_data (data, avt.bitmap_color);

  if (not image)
    image = avt_load_image_bmp_data (data);

  return image;
}

static avt_graphic *
avt_load_image_file (const char *filename)
{
  avt_graphic *image;
  avt_data d;

  image = NULL;

  avt_data_init (&d);
  if (d.open_file (&d, filename))
    image = avt_load_image_avtdata (&d);
  d.done (&d);

  if (not image and backend.graphic_file)
    image = backend.graphic_file (filename);

  return image;
}

static avt_graphic *
avt_load_image_stream (avt_stream * stream)
{
  avt_graphic *image;
  avt_data d;

  image = NULL;

  avt_data_init (&d);
  if (d.open_stream (&d, (FILE *) stream, false))
    image = avt_load_image_avtdata (&d);
  d.done (&d);

  if (not image and backend.graphic_stream)
    image = backend.graphic_stream (stream);

  return image;
}

static avt_graphic *
avt_load_image_memory (void *data, size_t size)
{
  avt_graphic *image;
  avt_data d;

  image = NULL;

  avt_data_init (&d);
  if (d.open_memory (&d, data, size))
    image = avt_load_image_avtdata (&d);
  d.done (&d);

  if (not image and backend.graphic_memory)
    image = backend.graphic_memory (data, size);

  return image;
}


extern const char *
avt_copyright (void)
{
  return "Copyright (c) " COPYRIGHTYEAR " Andreas K. Foerster";
}

extern const char *
avt_license (void)
{
  return "GPLv3+: GNU GPL version 3 or later "
    "<http://gnu.org/licenses/gpl.html>";
}

extern const wchar_t *
avt_wide_copyright (void)
{
  return L"Copyright \u00A9 " WIDE_COPYRIGHTYEAR L" Andreas K. F\u00F6rster";
}

extern const wchar_t *
avt_wide_license (void)
{
  return L"GPLv3+: GNU GPL version 3 or later "
    L"<http://gnu.org/licenses/gpl.html>";
}

extern bool
avt_initialized (void)
{
  return (screen != NULL);
}

extern int
avt_get_status (void)
{
  return _avt_STATUS;
}

extern void
avt_set_status (int status)
{
  _avt_STATUS = status;
}

// shows or clears the text cursor in the current position
// note: this function is rather time consuming
static void
avt_show_text_cursor (bool on)
{
  if (on != avt.text_cursor_actually_visible and not avt.hold_updates)
    {
      if (on)
	{
	  // save character under cursor
	  avt_graphic_segment (screen, avt.cursor.x, avt.cursor.y,
			       fontwidth, fontheight, avt.cursor_character, 0,
			       0);

	  // show text-cursor
	  avt_darker_area (screen, avt.cursor.x, avt.cursor.y,
			   fontwidth, fontheight, 0x50);
	  backend.update_area (screen, avt.cursor.x, avt.cursor.y,
			       fontwidth, fontheight);
	}
      else
	{
	  // restore saved character
	  avt_put_graphic (avt.cursor_character, screen,
			   avt.cursor.x, avt.cursor.y);
	  backend.update_area (screen, avt.cursor.x, avt.cursor.y,
			       fontwidth, fontheight);
	}

      avt.text_cursor_actually_visible = on;
    }
}

extern void
avt_activate_cursor (bool on)
{
  avt.text_cursor_visible = on;

  if (screen and avt.textfield.x >= 0)
    avt_show_text_cursor (avt.text_cursor_visible);
}

static inline void
avt_no_textfield (void)
{
  avt.textfield.x = avt.textfield.y
    = avt.textfield.width = avt.textfield.height = -1;
  avt.viewport = avt.textfield;
}

extern void
avt_clear_screen (void)
{
  if (screen)
    {
      avt_free_screen ();
      avt_update_all ();
    }

  // undefine textfield / viewport
  avt_no_textfield ();
  avt.avatar_visible = false;
}

#define NAME_PADDING (BORDER_3D_WIDTH + 3)

static void
avt_show_name (void)
{
  if (screen and avt.avatar_image and avt.name)
    {
      // save old character colors
      avt_color old_text_color = avt.text_color;
      avt_color old_background_color = avt.text_background_color;

      avt.text_color = AVT_COLOR_BLACK;
      avt.text_background_color = AVT_COLOR_TAN;

      int x, y;

      if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	x = avt.window.x + (avt.window.width / 2)
	  + (avt.avatar_image->width / 2) + BUTTON_DISTANCE;
      else			// left
	x = avt.window.x + AVATAR_MARGIN + avt.avatar_image->width
	  + BUTTON_DISTANCE;

      if (AVT_HEADER == avt.avatar_mode)
	y = avt.window.y + TOPMARGIN + avt.avatar_image->height
	  - fontheight - 2 * NAME_PADDING;
      else
	y = avt.window.y + avt.window.height - AVATAR_MARGIN
	  - fontheight - 2 * NAME_PADDING;

      // draw sign
      avt_bar3d (screen, x, y,
		 (wcslen (avt.name) * fontwidth) + 2 * NAME_PADDING,
		 fontheight + 2 * NAME_PADDING, avt.text_background_color,
		 false);

      // show name
      avt.cursor.x = x + NAME_PADDING;
      avt.cursor.y = y + NAME_PADDING;

      wchar_t *p = avt.name;
      while (*p)
	{
	  avt_drawchar ((avt_char) * p++, screen);
	  avt.cursor.x += fontwidth;
	}

      // restore old character colors
      avt.text_color = old_text_color;
      avt.text_background_color = old_background_color;
    }
}

/* draw the avatar image,
 * but doesn't update the screen yet
 */
static void
avt_draw_avatar (void)
{
  struct avt_position pos;

  if (screen)
    {
      // fill the screen with background color
      // (not only the window!)
      avt_free_screen ();

      avt_release_raw_image ();	// not needed anymore

      avt_avatar_window ();

      if (avt.avatar_image)
	{
	  if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	    pos.x =
	      avt.window.x + (avt.window.width / 2) -
	      (avt.avatar_image->width / 2);
	  else			// left
	    pos.x = avt.window.x + AVATAR_MARGIN;

	  if (AVT_HEADER == avt.avatar_mode)
	    pos.y = avt.window.y + TOPMARGIN;
	  else			// bottom
	    pos.y =
	      avt.window.y + avt.window.height - avt.avatar_image->height -
	      AVATAR_MARGIN;

	  avt_put_graphic (avt.avatar_image, screen, pos.x, pos.y);
	}

      if (avt.name)
	avt_show_name ();
    }
}

extern void
avt_show_avatar (void)
{
  if (screen)
    {
      avt_draw_avatar ();
      avt_update_all ();

      avt_no_textfield ();
      avt.avatar_visible = true;
    }
}

static void
avt_draw_balloon2 (int offset, avt_color ballooncolor)
{
  struct avt_area shape;

  // full size
  shape.x = avt.textfield.x - BALLOON_INNER_MARGIN + offset;
  shape.width = avt.textfield.width + (2 * BALLOON_INNER_MARGIN);
  shape.y = avt.textfield.y - BALLOON_INNER_MARGIN + offset;
  shape.height = avt.textfield.height + (2 * BALLOON_INNER_MARGIN);

  // horizontal shape
  avt_bar (screen, shape.x, shape.y + round_upper_left_height,
	   shape.width,
	   shape.height
	   - (round_upper_left_height + round_lower_left_height),
	   ballooncolor);

  // vertical shape
  avt_bar (screen, shape.x + round_upper_left_width, shape.y,
	   shape.width - (round_upper_left_width + round_upper_right_width),
	   shape.height, ballooncolor);

  // draw corners
  avt_put_image_xbm (screen, shape.x, shape.y,
		     round_upper_left_bits, round_upper_left_width,
		     round_upper_left_height, ballooncolor);

  avt_put_image_xbm (screen,
		     shape.x + shape.width - round_upper_right_width, shape.y,
		     round_upper_right_bits, round_upper_right_width,
		     round_upper_right_height, ballooncolor);

  avt_put_image_xbm (screen, shape.x,
		     shape.y + shape.height - round_lower_left_height,
		     round_lower_left_bits, round_lower_left_width,
		     round_lower_left_height, ballooncolor);

  avt_put_image_xbm (screen,
		     shape.x + shape.width - round_lower_right_width,
		     shape.y + shape.height - round_lower_right_height,
		     round_lower_right_bits, round_lower_right_width,
		     round_lower_right_height, ballooncolor);

  // draw balloonpointer
  // only if there is an avatar image
  if (avt.avatar_image
      and AVT_FOOTER != avt.avatar_mode and AVT_HEADER != avt.avatar_mode)
    {
      struct avt_position position;
      unsigned char *bits;

      // balloonpointer and thinkpointer must have the same size!
      if (avt.avatar_mode == AVT_THINK)
	bits = thinkpointer_bits;
      else
	bits = balloonpointer_bits;

      position.x = avt.window.x + avt.avatar_image->width
	+ (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET + offset;

      position.y = avt.window.y + (avt.balloonmaxheight * fontheight)
	+ (2 * BALLOON_INNER_MARGIN) + TOPMARGIN + offset;

      short y_offset = 0;

      // if the balloonpointer is too large, cut it
      if (balloonpointer_height > (avt.avatar_image->height / 2))
	y_offset = balloonpointer_height - (avt.avatar_image->height / 2);

      // only draw the balloonpointer, when it fits
      if (position.x + balloonpointer_width + BALLOONPOINTER_OFFSET
	  + BALLOON_INNER_MARGIN < avt.window.x + avt.window.width)
	avt_put_image_xbm_part (screen, position.x, position.y, y_offset,
				bits, balloonpointer_width,
				balloonpointer_height, ballooncolor);
    }
}

static void
avt_draw_balloon (void)
{
  short centered_y;

  if (not avt.avatar_visible)
    avt_draw_avatar ();

  avt.textfield.width = (avt.balloonwidth * fontwidth);
  avt.textfield.height = (avt.balloonheight * fontheight);
  centered_y =
    avt.window.y + (avt.window.height / 2) - (avt.textfield.height / 2);

  if (not avt.avatar_image)
    avt.textfield.y = centered_y;	// middle of the window
  else
    {
      // align with balloon
      if (AVT_HEADER == avt.avatar_mode)
	avt.textfield.y =
	  avt.window.y + avt.avatar_image->height + AVATAR_MARGIN +
	  TOPMARGIN + BALLOON_INNER_MARGIN;
      else
	avt.textfield.y =
	  avt.window.y +
	  ((avt.balloonmaxheight - avt.balloonheight) * fontheight) +
	  TOPMARGIN + BALLOON_INNER_MARGIN;

      // in separate or heading mode it might also be better to center it
      if ((AVT_FOOTER == avt.avatar_mode and avt.textfield.y > centered_y)
	  or (AVT_HEADER == avt.avatar_mode and avt.textfield.y < centered_y))
	avt.textfield.y = centered_y;
    }

  // horizontally centered as default
  avt.textfield.x =
    avt.window.x + (avt.window.width / 2) -
    (avt.balloonwidth * fontwidth / 2);

  // align horizontally with balloonpointer
  if (avt.avatar_image
      and AVT_FOOTER != avt.avatar_mode and AVT_HEADER != avt.avatar_mode)
    {
      // left border not aligned with balloon pointer?
      if (avt.textfield.x >
	  avt.window.x + avt.avatar_image->width + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET)
	avt.textfield.x =
	  avt.window.x + avt.avatar_image->width + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET;

      // right border not aligned with balloon pointer?
      if (avt.textfield.x + avt.textfield.width <
	  avt.window.x + avt.avatar_image->width + balloonpointer_width
	  + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET)
	{
	  avt.textfield.x =
	    avt.window.x + avt.avatar_image->width - avt.textfield.width
	    + balloonpointer_width
	    + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET;

	  // align with right window-border
	  if (avt.textfield.x >
	      avt.window.x + avt.window.width -
	      (avt.balloonwidth * fontwidth) - (2 * BALLOON_INNER_MARGIN))
	    avt.textfield.x =
	      avt.window.x + avt.window.width -
	      (avt.balloonwidth * fontwidth) - (2 * BALLOON_INNER_MARGIN);
	}
    }

  avt.viewport = avt.textfield;

  // first draw shadow
  avt_draw_balloon2 (SHADOWOFFSET, avt_darker (avt.background_color, 0x20));

  avt_draw_balloon2 (0, avt.ballooncolor);

  if (avt.textdir_rtl)
    avt.linestart = avt.viewport.x + avt.viewport.width - fontwidth;
  else
    avt.linestart = avt.viewport.x;

  avt.avatar_visible = true;

  // cursor at top 
  avt.cursor.x = avt.linestart;
  avt.cursor.y = avt.viewport.y;

  // reset saved position
  avt.saved_position = avt.cursor;

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  // update everything
  // (there may be leftovers from large images)
  avt_update_all ();
}

extern void
avt_text_direction (int direction)
{
  struct avt_area area;

  avt.textdir_rtl = direction;

  /*
   * if there is already a ballon,
   * recalculate the linestart and put the cursor in the first position
   */
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      if (avt.textdir_rtl)
	avt.linestart = area.x + area.width - fontwidth;
      else
	avt.linestart = area.x;

      avt.cursor.x = avt.linestart;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_set_balloon_width (int width)
{
  if (width != avt.balloonwidth)
    {
      if (width < AVT_LINELENGTH and width > 0)
	avt.balloonwidth = (width > 7) ? width : 7;
      else
	avt.balloonwidth = AVT_LINELENGTH;

      // if balloon is visible, redraw it
      if (avt.textfield.x >= 0)
	{
	  avt.avatar_visible = false;	// force to redraw everything
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_balloon_height (int height)
{
  if (height != avt.balloonheight)
    {
      if (height > 0 and height < avt.balloonmaxheight)
	avt.balloonheight = height;
      else
	avt.balloonheight = avt.balloonmaxheight;

      // if balloon is visible, redraw it
      if (avt.textfield.x >= 0)
	{
	  avt.avatar_visible = false;	// force to redraw everything
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_balloon_size (int height, int width)
{
  if (height != avt.balloonheight or width != avt.balloonwidth)
    {
      if (height > 0 and height < avt.balloonmaxheight)
	avt.balloonheight = height;
      else
	avt.balloonheight = avt.balloonmaxheight;

      if (width < AVT_LINELENGTH and width > 0)
	avt.balloonwidth = (width > 7) ? width : 7;
      else
	avt.balloonwidth = AVT_LINELENGTH;

      // if balloon is visible, redraw it
      if (avt.textfield.x >= 0)
	{
	  avt.avatar_visible = false;	// force to redraw everything
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_avatar_mode (int mode)
{
  if (mode >= AVT_SAY and mode <= AVT_FOOTER)
    avt.avatar_mode = mode;

  // if balloon is visible, remove it
  if (screen and avt.textfield.x >= 0)
    avt_show_avatar ();
}

extern void
avt_bell (void)
{
  bell ();
}

// flashes the screen
extern void
avt_flash (void)
{
  avt_graphic *oldwindowimage;

  if (not screen)
    return;

  oldwindowimage = avt_get_window ();

  // fill the whole screen with color
  avt_fill (screen, 0xFFFF00);
  avt_update_all ();
  avt_delay (150);

  // fill the whole screen with background color
  avt_fill (screen, avt.background_color);

  // restore image
  avt_put_graphic (oldwindowimage, screen, avt.window.x, avt.window.y);
  avt_free_graphic (oldwindowimage);

  // make visible again
  avt_update_all ();
}

extern bool
avt_check_buttons (int x, int y)
{
  struct avt_button *button;

  x -= avt.window.x;
  y -= avt.window.y;

  for (int nr = 0; nr < MAX_BUTTONS; nr++)
    {
      button = &avt_buttons[nr];

      if (button->background
	  and (y >= button->y) and (y <= (button->y + BASE_BUTTON_HEIGHT))
	  and (x >= button->x) and (x <= (button->x + BASE_BUTTON_WIDTH)))
	{
	  avt_push_key (button->key);
	  return true;
	}
    }

  return false;
}

extern int
avt_where_x (void)
{
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.origin_mode)
	return ((avt.cursor.x - avt.viewport.x) / fontwidth) + 1;
      else
	return ((avt.cursor.x - avt.textfield.x) / fontwidth) + 1;
    }
  else
    return -1;
}

extern int
avt_where_y (void)
{
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.origin_mode)
	return ((avt.cursor.y - avt.viewport.y) / fontheight) + 1;
      else
	return ((avt.cursor.y - avt.textfield.y) / fontheight) + 1;
    }
  else
    return -1;
}

extern bool
avt_home_position (void)
{
  if (not screen or avt.textfield.x < 0)
    return true;		// about to be set to home position
  else
    return (avt.cursor.y == avt.viewport.y and avt.cursor.x == avt.linestart);
}

// this always means the full textfield
extern int
avt_get_max_x (void)
{
  if (screen)
    return avt.balloonwidth;
  else
    return -1;
}

// this always means the full textfield
extern int
avt_get_max_y (void)
{
  if (screen)
    return avt.balloonheight;
  else
    return -1;
}

extern void
avt_move_x (int x)
{
  struct avt_area area;

  if (screen and avt.textfield.x >= 0)
    {
      if (x < 1)
	x = 1;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.cursor.x = (x - 1) * fontwidth + area.x;

      // max-pos exeeded?
      if (avt.cursor.x > area.x + area.width - fontwidth)
	avt.cursor.x = area.x + area.width - fontwidth;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_move_y (int y)
{
  struct avt_area area;

  if (screen and avt.textfield.x >= 0)
    {
      if (y < 1)
	y = 1;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.cursor.y = (y - 1) * fontheight + area.y;

      // max-pos exeeded?
      if (avt.cursor.y > area.y + area.height - fontheight)
	avt.cursor.y = area.y + area.height - fontheight;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_move_xy (int x, int y)
{
  struct avt_area area;

  if (screen and avt.textfield.x >= 0)
    {
      if (x < 1)
	x = 1;

      if (y < 1)
	y = 1;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.cursor.x = (x - 1) * fontwidth + area.x;
      avt.cursor.y = (y - 1) * fontheight + area.y;

      // max-pos exeeded?
      if (avt.cursor.x > area.x + area.width - fontwidth)
	avt.cursor.x = area.x + area.width - fontwidth;

      if (avt.cursor.y > area.y + area.height - fontheight)
	avt.cursor.y = area.y + area.height - fontheight;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_save_position (void)
{
  avt.saved_position = avt.cursor;
}

extern void
avt_restore_position (void)
{
  avt.cursor = avt.saved_position;
}

extern void
avt_insert_spaces (int num)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt_graphic_segment (screen, avt.cursor.x, avt.cursor.y,
		       avt.viewport.width - (avt.cursor.x - avt.viewport.x)
		       - (num * fontwidth), fontheight,
		       screen, avt.cursor.x + (num * fontwidth),
		       avt.cursor.y);

  avt_bar (screen, avt.cursor.x, avt.cursor.y,
	   num * fontwidth, fontheight, avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  // update line
  if (not avt.hold_updates)
    backend.update_area (screen, avt.viewport.x, avt.cursor.y,
			 avt.viewport.width, fontheight);
}

extern void
avt_delete_characters (int num)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt_graphic_segment (screen, avt.cursor.x + (num * fontwidth),
		       avt.cursor.y,
		       avt.viewport.width - (avt.cursor.x - avt.viewport.x) -
		       (num * fontwidth), fontheight, screen,
		       avt.cursor.x, avt.cursor.y);

  avt_bar (screen,
	   avt.viewport.x + avt.viewport.width - (num * fontwidth),
	   avt.cursor.y, num * fontwidth, fontheight,
	   avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  // update line
  if (not avt.hold_updates)
    backend.update_area (screen, avt.viewport.x, avt.cursor.y,
			 avt.viewport.width, fontheight);
}

extern void
avt_erase_characters (int num)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  int x = (avt.textdir_rtl) ? avt.cursor.x - (num * fontwidth) : avt.cursor.x;

  avt_bar (screen, x, avt.cursor.y, num * fontwidth, fontheight,
	   avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  // update area
  if (not avt.hold_updates)
    backend.update_area (screen, x, avt.cursor.y,
			 num * fontwidth, fontheight);
}

extern void
avt_delete_lines (int line, int num)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (not avt.origin_mode)
    line -= (avt.viewport.y - avt.textfield.y) / fontheight;

  // check if values are sane
  if (line < 1 or num < 1 or line > (avt.viewport.height / fontheight))
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt_graphic_segment (screen, avt.viewport.x,
		       avt.viewport.y + ((line - 1 + num) * fontheight),
		       avt.viewport.width,
		       avt.viewport.height - ((line - 1 + num) * fontheight),
		       screen, avt.viewport.x,
		       avt.viewport.y + ((line - 1) * fontheight));

  avt_bar (screen, avt.viewport.x,
	   avt.viewport.y + avt.viewport.height - (num * fontheight),
	   avt.viewport.width, num * fontheight, avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  avt_update_viewport ();
}

extern void
avt_insert_lines (int line, int num)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (not avt.origin_mode)
    line -= (avt.viewport.y - avt.textfield.y) / fontheight;

  // check if values are sane
  if (line < 1 or num < 1 or line > (avt.viewport.height / fontheight))
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt_graphic_segment (screen, avt.viewport.x,
		       avt.viewport.y + ((line - 1) * fontheight),
		       avt.viewport.width,
		       avt.viewport.height - ((line - 1 + num) * fontheight),
		       screen, avt.viewport.x,
		       avt.viewport.y + ((line - 1 + num) * fontheight));

  avt_bar (screen, avt.viewport.x,
	   avt.viewport.y + ((line - 1) * fontheight),
	   avt.viewport.width, num * fontheight, avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  avt_update_viewport ();
}

extern void
avt_viewport (int x, int y, int width, int height)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  // make coordinates 0-offset
  --x;
  --y;

  // sanitize values

  if (x < 0)
    x = 0;
  else if (x >= avt.balloonwidth)
    x = avt.balloonwidth - 1;

  if (y < 0)
    y = 0;
  else if (y >= avt.balloonheight)
    y = avt.balloonheight - 1;

  if (width <= 0 or width > avt.balloonwidth - x)
    width = avt.balloonwidth - x;

  if (height <= 0 or height > avt.balloonheight - y)
    height = avt.balloonheight - y;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt.viewport.x = avt.textfield.x + (x * fontwidth);
  avt.viewport.y = avt.textfield.y + (y * fontheight);
  avt.viewport.width = width * fontwidth;
  avt.viewport.height = height * fontheight;

  if (avt.textdir_rtl)
    avt.linestart = avt.viewport.x + avt.viewport.width - fontwidth;
  else
    avt.linestart = avt.viewport.x;

  avt.cursor.x = avt.linestart;
  avt.cursor.y = avt.viewport.y;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);
}

extern void
avt_newline_mode (bool mode)
{
  avt.newline_mode = mode;
}


extern bool
avt_get_newline_mode (void)
{
  return avt.newline_mode;
}

extern void
avt_set_auto_margin (bool mode)
{
  avt.auto_margin = mode;
}

extern bool
avt_get_auto_margin (void)
{
  return avt.auto_margin;
}

extern void
avt_set_origin_mode (bool mode)
{
  struct avt_area area;

  avt.origin_mode = mode;

  if (avt.text_cursor_visible and avt.textfield.x >= 0)
    avt_show_text_cursor (false);

  if (avt.origin_mode)
    area = avt.viewport;
  else
    area = avt.textfield;

  if (avt.textdir_rtl)
    avt.linestart = area.x + area.width - fontwidth;
  else
    avt.linestart = area.x;

  // cursor to position 1,1
  // when origin mode is off, then it may be outside the viewport (sic)
  avt.cursor.x = avt.linestart;
  avt.cursor.y = area.y;

  // reset saved position
  avt.saved_position = avt.cursor;

  if (avt.text_cursor_visible and avt.textfield.x >= 0)
    avt_show_text_cursor (true);
}

extern bool
avt_get_origin_mode (void)
{
  return avt.origin_mode;
}

extern void
avt_clear (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  // use background color of characters
  avt_bar (screen, avt.viewport.x, avt.viewport.y,
	   avt.viewport.width, avt.viewport.height,
	   avt.text_background_color);

  avt.cursor.x = avt.linestart;

  if (avt.origin_mode)
    avt.cursor.y = avt.viewport.y;
  else
    avt.cursor.y = avt.textfield.y;

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  avt_update_viewport ();
}

extern void
avt_clear_up (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  avt_bar (screen, avt.viewport.x, avt.viewport.y + fontheight,
	   avt.viewport.width, avt.cursor.y, avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    backend.update_area (screen, avt.viewport.x,
			 avt.viewport.y + fontheight, avt.viewport.width,
			 avt.cursor.y);
}

extern void
avt_clear_down (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt_bar (screen, avt.viewport.x, avt.cursor.y, avt.viewport.width,
	   avt.viewport.height - (avt.cursor.y - avt.viewport.y),
	   avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    backend.update_area (screen, avt.viewport.x, avt.cursor.y,
			 avt.viewport.width,
			 avt.viewport.height - (avt.cursor.y -
						avt.viewport.y));
}

extern void
avt_clear_eol (void)
{
  int x, width;

  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.textdir_rtl)		// right to left
    {
      x = avt.viewport.x;
      width = avt.cursor.x + fontwidth - avt.viewport.x;
    }
  else				// left to right
    {
      x = avt.cursor.x;
      width = avt.viewport.width - (avt.cursor.x - avt.viewport.x);
    }

  avt_bar (screen, x, avt.cursor.y, width, fontheight,
	   avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    backend.update_area (screen, x, avt.cursor.y, width, fontheight);
}

// clear beginning of line
extern void
avt_clear_bol (void)
{
  int x, width;

  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.textdir_rtl)		// right to left
    {
      x = avt.cursor.x;
      width = avt.viewport.width - (avt.cursor.x - avt.viewport.x);
    }
  else				// left to right
    {
      x = avt.viewport.x;
      width = avt.cursor.x + fontwidth - avt.viewport.x;
    }

  avt_bar (screen, x, avt.cursor.y, width, fontheight,
	   avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    backend.update_area (screen, x, avt.cursor.y, width, fontheight);
}

extern void
avt_clear_line (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  avt_bar (screen, avt.viewport.x, avt.cursor.y,
	   avt.viewport.width, fontheight, avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    backend.update_area (screen, avt.viewport.x, avt.cursor.y,
			 avt.viewport.width, fontheight);
}

extern int
avt_flip_page (void)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return _avt_STATUS;

  // do nothing when the textfield is already empty
  if (avt.cursor.x == avt.linestart and avt.cursor.y == avt.viewport.y)
    return _avt_STATUS;

  /* the viewport must be updated,
     if it's not updated letter by letter */
  if (not avt.text_delay)
    avt_update_viewport ();

  avt_wait (avt.flip_page_delay);
  avt_clear ();
  return _avt_STATUS;
}

static void
avt_scroll_up (void)
{
  switch (avt.scroll_mode)
    {
    case -1:
      // move cursor outside of balloon
      avt.cursor.y += fontheight;
      break;
    case 1:
      avt_delete_lines (((avt.viewport.y - avt.textfield.y) / fontheight) + 1,
			1);

      if (avt.origin_mode)
	avt.cursor.y = avt.viewport.y + avt.viewport.height - fontheight;
      else
	avt.cursor.y = avt.textfield.y + avt.textfield.height - fontheight;

      if (avt.newline_mode)
	avt.cursor.x = avt.linestart;
      break;
    case 0:
      avt_flip_page ();
      break;
    }
}

static void
avt_carriage_return (void)
{
  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt.cursor.x = avt.linestart;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);
}

extern int
avt_new_line (void)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return _avt_STATUS;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  if (avt.newline_mode)
    avt.cursor.x = avt.linestart;

  /* if the cursor is at the last line of the viewport
   * scroll up
   */
  if (avt.cursor.y == avt.viewport.y + avt.viewport.height - fontheight)
    avt_scroll_up ();
  else
    avt.cursor.y += fontheight;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  return _avt_STATUS;
}

extern bool
avt_combining (avt_char ch)
{
  // this is still very incomplete
  return (ch >= 0x0300 and ch <= 0x036F)
    or ch == 0x0374 or ch == 0x0375 or ch == 0x037A
    or ch == 0x0385 or ch == 0x1FBD
    or (ch >= 0x1FBF and ch <= 0x1FC1)
    or (ch >= 0x1FCD and ch <= 0x1FCF)
    or (ch >= 0x1FDD and ch <= 0x1FDF)
    or (ch >= 0x1FED and ch <= 0x1FEF)
    or ch == 0x1FFD or ch == 0x1FFE
    or (ch >= 0x0483 and ch <= 0x0489)
    or (ch >= 0x0591 and ch <= 0x05BD)
    or ch == 0x05BF or ch == 0x05C1 or ch == 0x05C2
    or ch == 0x05C4 or ch == 0x05C5 or ch == 0x05C7
    or ch == 0x0E31
    or (ch >= 0x0E34 and ch <= 0x0E3A)
    or (ch >= 0x0E47 and ch <= 0x0E4E)
    or (ch >= 0x135D and ch <= 0x135F)
    or (ch >= 0x1DC0 and ch <= 0x1DFF)
    or (ch >= 0x20D0 and ch <= 0x20FF) or (ch >= 0xFE20 and ch <= 0xFE2F);
}

// avt_drawchar: draws the raw char - with no interpretation
static void
avt_drawchar (avt_char ch, avt_graphic * surface)
{
  const uint_least8_t *font_line;	// pixel line from font definition
  uint_least16_t line;		// normalized pixel line might get modified

  // only draw character when it fully fits
  if (avt.cursor.x < 0 or avt.cursor.y < 0
      or avt.cursor.x > surface->width - fontwidth
      or avt.cursor.y > surface->height - fontheight)
    return;

  font_line = (const uint_least8_t *) avt_get_font_char ((int) ch);

  if (not font_line)
    font_line = (const uint_least8_t *) avt_get_font_char (0);

  if (not avt_combining (ch))
    {
      // fill with background color
      avt_bar (surface, avt.cursor.x, avt.cursor.y,
	       fontwidth, fontheight, avt.text_background_color);
    }
  else				// combining
    avt_backspace ();

  for (int y = 0; y < fontheight; y++)
    {
      if (fontwidth > CHAR_BIT)
	{
	  line = *(const uint_least16_t *) font_line;
	  font_line += 2;
	}
      else
	{
	  line = *font_line << CHAR_BIT;
	  font_line++;
	}

      if (avt.bold and not NOT_BOLD)
	line = line bitor (line >> 1);

      if (avt.underlined and y == fontunderline)
	line = 0xFFFF;

      if (avt.inverse)
	line = compl line;

      // leftmost bit set, gets shifted to the right in the for loop
      uint_least16_t scanbit = 0x8000;
      avt_color *p = avt_pixel (surface, avt.cursor.x, avt.cursor.y + y);
      for (int x = 0; x < fontwidth; x++, p++, scanbit >>= 1)
	if (line bitand scanbit)
	  *p = avt.text_color;
    }				// for (int y...
}

extern bool
avt_is_printable (avt_char ch)
{
  return (bool) (avt_get_font_char ((int) ch) != NULL);
}

// make current char visible
static void
avt_showchar (void)
{
  if (not avt.hold_updates)
    {
      backend.update_area (screen, avt.cursor.x, avt.cursor.y,
			   fontwidth, fontheight);
      avt.text_cursor_actually_visible = false;
    }
}

// advance position - only in the textfield
extern int
avt_forward (void)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return _avt_STATUS;

  if (avt.textdir_rtl)
    avt.cursor.x -= fontwidth;
  else
    avt.cursor.x += fontwidth;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  return _avt_STATUS;
}

/*
 * if cursor is horizontally outside of the viewport
 *  and the balloon is visible
 * and auto_margin is activated
 * then start a new line
 */
static void
check_auto_margin (void)
{
  if (screen and avt.textfield.x >= 0 and avt.auto_margin)
    {
      if (avt.cursor.x < avt.viewport.x
	  or avt.cursor.x > avt.viewport.x + avt.viewport.width - fontwidth)
	{
	  if (not avt.newline_mode)
	    avt_carriage_return ();
	  avt_new_line ();
	}

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_reset_tab_stops (void)
{
  for (int i = 0; i < AVT_LINELENGTH; i++)
    if (i % 8 == 0)
      avt.tab_stops[i] = true;
    else
      avt.tab_stops[i] = false;
}

extern void
avt_clear_tab_stops (void)
{
  memset (&avt.tab_stops, false, sizeof (avt.tab_stops));
}

extern void
avt_set_tab (int x, bool onoff)
{
  if (x > 0 and x <= AVT_LINELENGTH)
    avt.tab_stops[x - 1] = onoff;
}

// advance to next tabstop
extern void
avt_next_tab (void)
{
  int x;

  // here we count zero based
  x = avt_where_x () - 1;

  if (avt.textdir_rtl)		// right to left
    {
      for (int i = x; i >= 0; i--)
	{
	  if (avt.tab_stops[i])
	    {
	      avt_move_x (i);
	      break;
	    }
	}
    }
  else				// left to right
    {
      for (int i = x + 1; i < AVT_LINELENGTH; i++)
	{
	  if (avt.tab_stops[i])
	    {
	      avt_move_x (i + 1);
	      break;
	    }
	}
    }
}

// go to last tabstop
extern void
avt_last_tab (void)
{
  int x;

  // here we count zero based
  x = avt_where_x () - 1;

  if (avt.textdir_rtl)		// right to left
    {
      for (int i = x; i < AVT_LINELENGTH; i++)
	{
	  if (avt.tab_stops[i])
	    {
	      avt_move_x (i);
	      break;
	    }
	}
    }
  else				// left to right
    {
      for (int i = x + 1; i >= 0; i--)
	{
	  if (avt.tab_stops[i])
	    {
	      avt_move_x (i + 1);
	      break;
	    }
	}
    }
}

static void
avt_clearchar (void)
{
  avt_bar (screen, avt.cursor.x, avt.cursor.y, fontwidth, fontheight,
	   avt.text_background_color);
  avt_showchar ();
}

extern void
avt_backspace (void)
{
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.cursor.x != avt.linestart)
	{
	  if (avt.text_cursor_visible)
	    avt_show_text_cursor (false);

	  if (avt.textdir_rtl)
	    avt.cursor.x += fontwidth;
	  else
	    avt.cursor.x -= fontwidth;
	}

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

/*
 * writes a character to the textfield -
 * interprets control characters
 */
extern int
avt_put_char (avt_char ch)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  switch (ch)
    {
    case 0x000A:		// LF: Line Feed
    case 0x000B:		// VT: Vertical Tab
    case 0x0085:		// NEL: NExt Line
    case 0x2028:		// LS: Line Separator
    case 0x2029:		// PS: Paragraph Separator
      avt_new_line ();
      break;

    case 0x000D:		// CR: Carriage Return
      avt_carriage_return ();
      break;

    case 0x000C:		// FF: Form Feed
      avt_flip_page ();
      break;

    case 0x0009:		// HT: Horizontal Tab
      avt_next_tab ();
      break;

    case 0x0008:		// BS: Back Space
      avt_backspace ();
      break;

    case 0x0007:		// BEL
      bell ();
      break;

      /*
       * ignore BOM here
       * must be handled outside of the library
       */
    case 0xFEFF:
      break;

      // LRM/RLM: only supported at the beginning of a line
    case 0x200E:		// LEFT-TO-RIGHT MARK (LRM)
      avt_text_direction (AVT_LEFT_TO_RIGHT);
      break;

    case 0x200F:		// RIGHT-TO-LEFT MARK (RLM)
      avt_text_direction (AVT_RIGHT_TO_LEFT);
      break;

      // other ignorable (invisible) characters
    case 0x200B:
    case 0x200C:
    case 0x200D:
      break;

    case 0x0020:		// SP: space
      if (avt.auto_margin)
	check_auto_margin ();

      if (avt.cursor.x < avt.viewport.x + avt.viewport.width)
	{
	  if (not avt.underlined and not avt.inverse)
	    avt_clearchar ();
	  else			// underlined or inverse
	    {
	      avt_drawchar (0x0020, screen);
	      avt_showchar ();
	    }
	  avt_forward ();
	}
      /*
       * no delay for the space char
       * it'd be annoying if you have a sequence of spaces
       */
      break;

    default:
      if (ch > 0x0020 or ch == 0x0000)
	{
	  if (avt.markup and ch == 0x005F)	// '_'
	    avt.underlined = not avt.underlined;
	  else if (avt.markup and ch == 0x002A)	// '*'
	    avt.bold = not avt.bold;
	  else			// not a markup character
	    {
	      if (avt.auto_margin)
		check_auto_margin ();

	      if (avt.cursor.x < avt.viewport.x + avt.viewport.width)
		{
		  avt_drawchar (ch, screen);
		  avt_showchar ();
		  if (avt.text_delay)
		    avt_delay (avt.text_delay);
		  avt_forward ();
		}

	      avt_update ();
	    }			// if not markup
	}			// if (ch > 0x0020)
    }				// switch

  return _avt_STATUS;
}

/*
 * bold can be stored as b\bbo\bol\bld\bd
 * underlinded as _\bu_\bn_\bd_\be_\br_\bl_\bi_\bn_\be_\bd
 */
static int
avt_overstrike (const wchar_t * txt)
{
  int r;

  r = 0;

  // check if all conditions are met
  if (not * txt or not * (txt + 1) or not * (txt + 2) or * (txt + 1) != L'\b')
    r = -1;
  else
    {
      if (*txt == L'_')
	{
	  avt.underlined = true;
	  if (avt_put_char ((avt_char) * (txt + 2)))
	    r = -1;
	  avt.underlined = false;
	}
      else if (*txt == *(txt + 2))
	{
	  avt.bold = true;
	  if (avt_put_char ((avt_char) * txt))
	    r = -1;
	  avt.bold = false;
	}
    }

  return r;
}

/*
 * writes L'\0' terminated string to textfield -
 * interprets control characters
 */
extern int
avt_say (const wchar_t * txt)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // nothing to do, when there is no text 
  if (not txt or not * txt)
    return avt_update ();

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  while (*txt)
    {
      if (*(txt + 1) == L'\b')
	{
	  if (avt_overstrike (txt))
	    break;
	  txt += 2;
	}
      else
	{
	  avt_char c = (avt_char) * txt;
	  if (0xD800 <= *txt and * txt <= 0xDBFF)	// UTF-16 high surrogate
	    {
	      c = ((*txt & 0x3FF) << 10) + (*(txt + 1) & 0x3FF) + 0x10000;
	      txt++;
	    }

	  if (avt_put_char (c))
	    break;
	}
      txt++;
    }

  return _avt_STATUS;
}

/*
 * writes string with given length to textfield -
 * interprets control characters
 */
extern int
avt_say_len (const wchar_t * txt, size_t len)
{
  // nothing to do, when txt == NULL
  // but do allow a text to start with zeros here
  if (not screen or not txt or _avt_STATUS != AVT_NORMAL)
    return avt_update ();

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  for (size_t i = 0; i < len; i++, txt++)
    {
      if (*(txt + 1) == L'\b' and i < len - 1)
	{
	  if (avt_overstrike (txt))
	    break;
	  txt += 2;
	  i += 2;
	}
      else
	{
	  avt_char c = (avt_char) * txt;
	  if (0xD800 <= *txt and * txt <= 0xDBFF)	// UTF-16 high surrogate
	    {
	      c = ((*txt & 0x3FF) << 10) + (*(txt + 1) & 0x3FF) + 0x10000;
	      txt++;
	      i++;
	    }

	  if (avt_put_char (c))
	    break;
	}
    }

  return _avt_STATUS;
}

extern int
avt_tell_len (const wchar_t * txt, size_t len)
{
  int width, height, line_length;
  size_t pos;
  const wchar_t *p;

  if (not txt or not * txt or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  width = 0;
  height = 1;
  line_length = 0;
  pos = 1;
  p = txt;

  while ((len > 0 and pos <= len) or (len == 0 and * p))
    {
      switch (*p)
	{
	case L'\n':
	case L'\v':
	case L'\x0085':	// NEL: NExt Line
	case L'\x2028':	// LS: Line Separator
	case L'\x2029':	// PS: Paragraph Separator
	  if (width < line_length)
	    width = line_length;
	  line_length = 0;
	  height++;
	  break;

	case L'\r':
	  if (width < line_length)
	    width = line_length;
	  line_length = 0;
	  break;

	case L'\t':
	  // FIXME
	  line_length += 8;
	  break;

	case L'\b':
	  line_length--;
	  break;

	case L'\a':
	case L'\xFEFF':
	case L'\x200E':
	case L'\x200F':
	case L'\x200B':
	case L'\x200C':
	case L'\x200D':
	  // no width
	  break;

	  // '_' is tricky because of overstrike
	  // must be therefore before '*'
	case L'_':
	  if (avt.markup)
	    {
	      if (*(p + 1) == L'\b')
		line_length++;
	      break;
	    }
	  // else fall through

	case L'*':
	  if (avt.markup)
	    break;
	  // else fall through

	default:
	  if ((*p >= 32 or * p == 0) and (*p < 0xD800 or * p > 0xDBFF)
	      and not avt_combining (*p))
	    {
	      line_length++;
	      if (avt.auto_margin and line_length > AVT_LINELENGTH)
		{
		  width = AVT_LINELENGTH;
		  height++;
		  line_length = 0;
		}
	    }
	  break;
	}

      p++;
      pos++;
    }

  if (width < line_length)
    width = line_length;

  avt_set_balloon_size (height, width);
  avt_clear ();

  if (len)
    avt_say_len (txt, len);
  else
    avt_say (txt);

  return _avt_STATUS;
}

extern int
avt_tell (const wchar_t * txt)
{
  return avt_tell_len (txt, 0);
}

extern void
avt_free (void *ptr)
{
  if (ptr)
    free (ptr);
}

static void
update_menu_bar (int menu_start, int menu_end, int line_nr, int old_line,
		 avt_graphic * plain_menu)
{
  if (line_nr != old_line)
    {
      // restore oldline
      if (old_line >= menu_start and old_line <= menu_end)
	{
	  int y = (old_line - 1) * fontheight;
	  avt_graphic_segment (plain_menu, 0, y,
			       avt.viewport.width, fontheight,
			       screen, avt.viewport.x, avt.viewport.y + y);
	  backend.update_area (screen, avt.viewport.x,
			       avt.viewport.y + y, avt.viewport.width,
			       fontheight);
	}

      // show bar
      if (line_nr >= menu_start and line_nr <= menu_end)
	{
	  int y = avt.viewport.y + ((line_nr - 1) * fontheight);
	  avt_darker_area (screen, avt.viewport.x, y, avt.viewport.width,
			   fontheight, 0x20);
	  backend.update_area (screen, avt.viewport.x, y,
			       avt.viewport.width, fontheight);
	}
    }
}

extern int
avt_choice (int *result, int start_line, int items, int key,
	    bool back, bool forward)
{
  int res;			// shadow for result

  res = -1;

  if (result)
    *result = -1;

  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (not screen or start_line <= 0 or items <= 0
      or start_line + items - 1 > avt.balloonheight)
    return AVT_FAILURE;

  avt_graphic *plain_menu;
  plain_menu = avt_get_area (screen, avt.viewport.x, avt.viewport.y,
			     avt.viewport.width, avt.viewport.height);

  int end_line = start_line + items - 1;

  int last_key;
  if (key)
    last_key = key + items - 1;
  else
    last_key = AVT_KEY_NONE;

  int line_nr = -1;
  int old_line = 0;

  avt_clear_keys ();
  avt_set_pointer_motion_key (0xE902);
  avt_set_pointer_buttons_key (AVT_KEY_ENTER);

  // check pointer position
  int x, y;
  avt_get_pointer_position (&x, &y);

  if (x >= avt.viewport.x
      and x <= avt.viewport.x + avt.viewport.width
      and y >= avt.viewport.y + ((start_line - 1) * fontheight)
      and y < avt.viewport.y + (end_line * fontheight))
    {
      line_nr = ((y - avt.viewport.y) / fontheight) + 1;
      update_menu_bar (start_line, end_line, line_nr, old_line, plain_menu);
      old_line = line_nr;
    }

  while (res == -1 and _avt_STATUS == AVT_NORMAL)
    {
      avt_char ch;

      ch = avt_get_key ();

      if (key and (ch >= key) and (ch <= last_key))
	res = (int) (ch - key + 1);
      else if (AVT_KEY_DOWN == ch)
	{
	  if (line_nr != end_line)
	    {
	      if (line_nr < start_line or line_nr > end_line)
		line_nr = start_line;
	      else
		line_nr++;

	      update_menu_bar (start_line, end_line, line_nr,
			       old_line, plain_menu);
	      old_line = line_nr;
	    }
	  else if (forward)
	    res = items;
	}
      else if (AVT_KEY_UP == ch)
	{
	  if (line_nr != start_line)
	    {
	      if (line_nr < start_line or line_nr > end_line)
		line_nr = end_line;
	      else
		line_nr--;
	      update_menu_bar (start_line, end_line, line_nr,
			       old_line, plain_menu);
	      old_line = line_nr;
	    }
	  else if (back)
	    res = 1;
	}
      else if (back and (AVT_KEY_PAGEUP == ch))
	res = 1;
      else if (forward and (AVT_KEY_PAGEDOWN == ch))
	res = items;
      else if ((AVT_KEY_ENTER == ch or AVT_KEY_RIGHT == ch)
	       and line_nr >= start_line and line_nr <= end_line)
	res = line_nr - start_line + 1;
      else if (0xE902 == ch)	// mouse motion
	{
	  avt_get_pointer_position (&x, &y);

	  if (x >= avt.viewport.x
	      and x <= avt.viewport.x + avt.viewport.width
	      and y >= avt.viewport.y + ((start_line - 1) * fontheight)
	      and y < avt.viewport.y + (end_line * fontheight))
	    line_nr = ((y - avt.viewport.y) / fontheight) + 1;

	  if (line_nr != old_line)
	    {
	      update_menu_bar (start_line, end_line, line_nr, old_line,
			       plain_menu);
	      old_line = line_nr;
	    }
	}
    }				// while

  avt_set_pointer_motion_key (AVT_KEY_NONE);
  avt_set_pointer_buttons_key (AVT_KEY_NONE);
  avt_clear_keys ();

  avt_free_graphic (plain_menu);

  if (result)
    *result = res;

  return _avt_STATUS;
}

extern void
avt_lock_updates (bool lock)
{
  avt.hold_updates = lock;

  // side effect: set text_delay to 0
  if (avt.hold_updates)
    avt.text_delay = 0;

  // if hold_updates is not set update the textfield
  avt_update_textfield ();
}

static inline void
avt_button_inlay (int x, int y, const unsigned char *bits,
		  int width, int height, avt_color color)
{
  avt_put_image_xbm (screen,
		     x + (BASE_BUTTON_WIDTH / 2) - (width / 2),
		     y + (BASE_BUTTON_WIDTH / 2) - (height / 2),
		     bits, width, height, color);
}

// coordinates are relative to window
static void
avt_show_button (int x, int y, enum avt_button_type type,
		 avt_char key, avt_color color)
{
  struct avt_position pos;
  int buttonnr;
  struct avt_button *button;

  // find free button number
  buttonnr = 0;
  while (buttonnr < MAX_BUTTONS and avt_buttons[buttonnr].background)
    buttonnr++;

  if (buttonnr == MAX_BUTTONS)
    {
      avt_set_error ("too many buttons");
      _avt_STATUS = AVT_ERROR;
      return;
    }

  button = &avt_buttons[buttonnr];

  button->x = x;
  button->y = y;
  button->key = key;

  pos.x = x + avt.window.x;
  pos.y = y + avt.window.y;

  button->background =
    avt_get_area (screen, pos.x, pos.y,
		  BASE_BUTTON_WIDTH, BASE_BUTTON_HEIGHT);

  avt_put_graphic (base_button, screen, pos.x, pos.y);

  switch (type)
    {
    case btn_cancel:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_cancel), color);
      break;

    case btn_yes:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_yes), color);
      break;

    case btn_no:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_no), color);
      break;

    case btn_right:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_right), color);
      break;

    case btn_left:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_left), color);
      break;

    case btn_up:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_up), color);
      break;

    case btn_down:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_down), color);
      break;

    case btn_fastforward:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_fastforward), color);
      break;

    case btn_fastbackward:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_fastbackward), color);
      break;

    case btn_stop:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_stop), color);
      break;

    case btn_pause:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_pause), color);
      break;

    case btn_help:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_help), color);
      break;

    case btn_eject:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_eject), color);
      break;

    case btn_circle:
      avt_button_inlay (pos.x, pos.y, AVT_XBM_INFO (btn_circle), color);
      break;
    }

  backend.update_area (screen, pos.x, pos.y,
		       BASE_BUTTON_WIDTH, BASE_BUTTON_HEIGHT);
}

static void
avt_clear_buttons (void)
{
  struct avt_button *button;

  for (int nr = 0; nr < MAX_BUTTONS; nr++)
    {
      button = &avt_buttons[nr];

      if (button->background)
	{
	  avt_put_graphic (button->background, screen,
			   button->x + avt.window.x,
			   button->y + avt.window.y);
	  backend.update_area (screen, button->x + avt.window.x,
			       button->y + avt.window.y,
			       BASE_BUTTON_WIDTH, BASE_BUTTON_HEIGHT);
	  avt_free_graphic (button->background);
	  button->background = NULL;
	}

      button->x = button->y = 0;
      button->key = 0;
    }
}

static inline bool
avt_is_linebreak (wchar_t c)
{
  return (c == L'\n' or c == L'\v' or c == L'\x85'
	  or c == L'\x2028' or c == L'\x2029');
}

// checks for formfeed or text separators
static inline bool
avt_is_pagebreak (wchar_t c)
{
  return (c == L'\f' or (c >= L'\x1C' and c <= L'\x1F'));
}

// checks for invisible characters (including '\a'!)
static inline bool
avt_is_invisible (wchar_t c)
{
  return (c == L'\a'
	  or c == L'\xFEFF' or c == L'\x200E' or c == L'\x200F'
	  or c == L'\x200B' or c == L'\x200C' or c == L'\x200D');
}

static size_t
avt_pager_line (const wchar_t * txt, size_t pos, size_t len,
		size_t horizontal)
{
  const wchar_t *tpos;

  tpos = txt + pos;

  avt.underlined = avt.bold = false;

  // handle pagebreaks
  if (avt_is_pagebreak (*tpos))
    {
      // draw separator line
      for (int i = AVT_LINELENGTH; i > 0; i--)
	avt_put_char (0x2550);

      tpos++;
      pos++;

      // evtl. skip line end
      while (pos < len and (txt[pos] == L'\r' or avt_is_linebreak (txt[pos])))
	pos++;

      return pos;
    }

  // search for linebreak or pagebreak or end of text
  const wchar_t *p = tpos;
  size_t line_length = 0;

  while (pos + line_length < len and not avt_is_linebreak (*p)
	 and not avt_is_pagebreak (*p))
    {
      line_length++;
      p++;
    }

  pos += line_length;

  // skip linebreak, but not a pagebreak
  if (pos < len and avt_is_linebreak (*p))
    pos++;

  // speedup horizontal scrolling
  horizontal *= 4;

  if (line_length > horizontal)
    {
      // skip horizontal characters, iff visible
      for (size_t i = 0; i < horizontal; i++)
	{
	  // FIXME: find solution for tabulators

	  // skip invisible characters
	  while (line_length and avt_is_invisible (*tpos))
	    {
	      tpos++;
	      line_length--;
	    }

	  // handle backspace (used for overstrike text)
	  while (line_length > 2 and tpos[1] == L'\b')
	    {
	      tpos += 2;
	      line_length -= 2;
	    }

	  // hande markup mode
	  while (avt.markup and (*tpos == L'_' or * tpos == L'*'))
	    {
	      if (*tpos == L'_')
		avt.underlined = not avt.underlined;
	      else if (*tpos == L'*')
		avt.bold = not avt.bold;

	      tpos++;
	      line_length--;
	    }

	  if (not line_length)
	    break;

	  tpos++;
	  line_length--;
	}

      if (line_length)
	avt_say_len (tpos, line_length);
    }

  return pos;
}

static size_t
avt_pager_screen (const wchar_t * txt, size_t pos, size_t len,
		  size_t horizontal)
{
  avt.hold_updates = true;
  avt_bar (screen, avt.textfield.x, avt.textfield.y,
	   avt.textfield.width, avt.textfield.height,
	   avt.text_background_color);

  for (int line_nr = 0; line_nr < avt.balloonheight; line_nr++)
    {
      avt.cursor.x = avt.linestart;
      avt.cursor.y = line_nr * fontheight + avt.textfield.y;
      pos = avt_pager_line (txt, pos, len, horizontal);
    }

  avt.hold_updates = false;
  avt_update_textfield ();

  return pos;
}

static size_t
avt_pager_lines_back (const wchar_t * txt, size_t pos, int lines)
{
  if (pos > 0)
    pos--;			// go to last linebreak

  lines--;

  while (lines--)
    {
      if (pos > 0 and avt_is_linebreak (txt[pos]))
	pos--;			// go before last linebreak

      while (pos > 0 and not avt_is_linebreak (txt[pos]))
	pos--;
    }

  if (pos > 0)
    pos++;

  return pos;
}

extern int
avt_pager (const wchar_t * txt, size_t len, int startline)
{
  size_t pos;
  size_t horizontal;
  struct avt_settings old_settings;
  bool quit;
  struct avt_position button;

  if (not screen)
    return AVT_ERROR;

  // do we actually have something to show?
  if (not txt or not * txt or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // get len if not given
  if (len == 0)
    len = wcslen (txt);

  horizontal = 0;

  // find startline
  pos = 0;
  if (startline > 1)
    {
      int nr;

      nr = startline - 1;
      while (nr > 0 and pos < len)
	{
	  while (pos < len and not avt_is_linebreak (txt[pos]))
	    pos++;
	  pos++;
	  nr--;
	}
    }

  // last screen
  if (pos >= len)
    pos = avt_pager_lines_back (txt, len, avt.balloonheight + 1);

  if (avt.textfield.x < 0)
    avt_draw_balloon ();
  else
    avt.viewport = avt.textfield;

  // show close-button

  // alignment: right bottom
  button.x = avt.window.width - BASE_BUTTON_WIDTH - AVATAR_MARGIN;
  button.y = avt.window.height - BASE_BUTTON_HEIGHT - AVATAR_MARGIN;

  // the button shouldn't be clipped
  if (button.y + avt.window.y < avt.textfield.y + avt.textfield.height
      and button.x + avt.window.x < avt.textfield.x + avt.textfield.width)
    button.x = avt.textfield.x + avt.textfield.width - avt.window.x;
  // this is a workaround: moving it down clashed with a bug in SDL

  avt_show_button (button.x, button.y, btn_cancel,
		   AVT_KEY_ESCAPE, AVT_BUTTON_COLOR);

  old_settings = avt;

  avt.text_cursor_visible = false;
  avt.auto_margin = false;

  // temporarily disable the bell function
  avt.bell = NULL;

  avt_set_text_delay (0);
  if (avt.markup)
    {
      avt_normal_text ();
      avt.markup = true;
    }
  else
    avt_normal_text ();

  // show first screen
  pos = avt_pager_screen (txt, pos, len, horizontal);

  // last screen
  if (pos >= len)
    {
      pos = avt_pager_lines_back (txt, len, avt.balloonheight + 1);
      pos = avt_pager_screen (txt, pos, len, horizontal);
    }

  quit = false;
  avt_clear_keys ();

  while (not quit and _avt_STATUS == AVT_NORMAL)
    {
      switch (avt_get_key ())
	{
	case AVT_KEY_ESCAPE:	// needed for the button
	case L'q':
	case AVT_KEY_BACKSPACE:	// my 'special' number keyboard
	  quit = true;
	  break;

	case AVT_KEY_DOWN:
	case L'2':
	  if (pos < len)	// if it's not the end
	    {
	      avt.hold_updates = true;
	      avt_delete_lines (1, 1);
	      avt.cursor.x = avt.linestart;
	      avt.cursor.y =
		(avt.balloonheight - 1) * fontheight + avt.textfield.y;
	      pos = avt_pager_line (txt, pos, len, horizontal);
	      avt.hold_updates = false;
	      avt_update_textfield ();
	    }
	  break;

	case AVT_KEY_UP:
	case L'8':
	  {
	    int start_pos;

	    start_pos =
	      avt_pager_lines_back (txt, pos, avt.balloonheight + 2);

	    if (start_pos == 0)
	      pos = avt_pager_screen (txt, 0, len, horizontal);
	    else
	      {
		avt.hold_updates = true;
		avt_insert_lines (1, 1);
		avt.cursor.x = avt.linestart;
		avt.cursor.y = avt.textfield.y;
		avt_pager_line (txt, start_pos, len, horizontal);
		avt.hold_updates = false;
		avt_update_textfield ();
		pos = avt_pager_lines_back (txt, pos, 2);
	      }
	  }
	  break;

	case AVT_KEY_PAGEDOWN:
	case L'3':
	case L' ':
	case L'f':
	  if (pos < len)
	    {
	      pos = avt_pager_screen (txt, pos, len, horizontal);
	      if (pos >= len)
		{
		  pos =
		    avt_pager_lines_back (txt, len, avt.balloonheight + 1);
		  pos = avt_pager_screen (txt, pos, len, horizontal);
		}
	    }
	  break;

	case AVT_KEY_PAGEUP:
	case L'9':
	case L'b':
	  pos = avt_pager_lines_back (txt, pos, 2 * avt.balloonheight + 1);
	  pos = avt_pager_screen (txt, pos, len, horizontal);
	  break;

	case AVT_KEY_HOME:
	case L'7':
	  horizontal = 0;
	  pos = avt_pager_screen (txt, 0, len, horizontal);
	  break;

	case AVT_KEY_END:
	case L'1':
	  pos = avt_pager_lines_back (txt, len, avt.balloonheight + 1);
	  pos = avt_pager_screen (txt, pos, len, horizontal);
	  break;

	case AVT_KEY_RIGHT:
	case L'6':
	  horizontal++;
	  pos = avt_pager_lines_back (txt, pos, avt.balloonheight + 1);
	  pos = avt_pager_screen (txt, pos, len, horizontal);
	  break;

	case AVT_KEY_LEFT:
	case L'4':
	  if (horizontal)
	    {
	      horizontal--;
	      pos = avt_pager_lines_back (txt, pos, avt.balloonheight + 1);
	      pos = avt_pager_screen (txt, pos, len, horizontal);
	    }
	  break;
	}			// switch (ch)
    }				// while

  // quit request only quits the pager here
  if (_avt_STATUS == AVT_QUIT)
    _avt_STATUS = AVT_NORMAL;

  avt_clear_buttons ();
  avt = old_settings;
  avt_activate_cursor (avt.text_cursor_visible);

  return _avt_STATUS;
}

// size in Bytes!
extern avt_char
avt_input (wchar_t * s, size_t size, const wchar_t * default_text,
	   int position, int mode)
{
  avt_char ch;
  size_t len, maxlen, pos;
  int old_textdir;
  bool insert_mode;
  bool finished;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // this function only works with left to right text
  // it would be too hard otherwise
  old_textdir = avt.textdir_rtl;
  avt.textdir_rtl = AVT_LEFT_TO_RIGHT;

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  /*
   * if the cursor is beyond the end of the viewport,
   * get a new page
   */
  if (avt.cursor.y > avt.viewport.y + avt.viewport.height - fontheight)
    avt_flip_page ();

  // maxlen is the rest of line
  if (avt.textdir_rtl)
    maxlen = (avt.cursor.x - avt.viewport.x) / fontwidth;
  else
    maxlen =
      ((avt.viewport.x + avt.viewport.width) - avt.cursor.x) / fontwidth;

  // does it fit in the buffer size?
  if (maxlen > size / sizeof (wchar_t) - 1)
    maxlen = size / sizeof (wchar_t) - 1;

  // clear the input field
  avt_erase_characters (maxlen);

  if (position < 0)
    position = INT_MAX;

  // copy default_text into s as far as it fits
  if (default_text and default_text[0] != L'\0')
    {
      wcsncpy (s, default_text, size / sizeof (wchar_t));
      s[size / sizeof (wchar_t) - 1] = L'\0';
      len = wcslen (s);
      pos = avt_min (len, (size_t) position);

      int startx = avt.cursor.x;
      avt_say (s);
      avt.cursor.x = startx + (pos * fontwidth);
    }
  else				// no default_text
    {
      memset (s, 0, size);
      len = pos = 0;
    }

  insert_mode = true;
  finished = false;
  ch = AVT_KEY_NONE;

  while (not finished and _avt_STATUS == AVT_NORMAL)
    {
      avt_show_text_cursor (true);

      ch = avt_get_key ();

      if (_avt_STATUS != AVT_NORMAL)
	{
	  ch = AVT_KEY_NONE;
	  break;
	}

      switch (ch)
	{
	case AVT_KEY_ESCAPE:
	  finished = true;
	  break;

	case AVT_KEY_ENTER:
	  finished = true;
	  avt_show_text_cursor (false);
	  avt_new_line ();
	  break;

	case AVT_KEY_UP:
	case AVT_KEY_DOWN:
	  if (mode > 0)
	    finished = true;
	  break;

	case AVT_KEY_HOME:
	  avt_show_text_cursor (false);
	  avt.cursor.x -= pos * fontwidth;
	  pos = 0;
	  break;

	case AVT_KEY_END:
	  avt_show_text_cursor (false);
	  if (len < maxlen)
	    {
	      avt.cursor.x += (len - pos) * fontwidth;
	      pos = len;
	    }
	  else
	    {
	      avt.cursor.x += (maxlen - 1 - pos) * fontwidth;
	      pos = maxlen - 1;
	    }
	  break;

	case AVT_KEY_BACKSPACE:
	  if (pos > 0)
	    {
	      pos--;
	      avt_show_text_cursor (false);
	      avt_backspace ();
	      avt_delete_characters (1);
	      memmove (&s[pos], &s[pos + 1],
		       (len - pos - 1) * sizeof (wchar_t));
	      len--;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_DELETE:
	  if (pos < len)
	    {
	      avt_show_text_cursor (false);
	      avt_delete_characters (1);
	      memmove (&s[pos], &s[pos + 1],
		       (len - pos - 1) * sizeof (wchar_t));
	      len--;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_LEFT:
	  if (pos > 0)
	    {
	      pos--;
	      // delete cursor
	      avt_show_text_cursor (false);
	      avt.cursor.x -= fontwidth;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_RIGHT:
	  if (pos < len and pos < maxlen - 1)
	    {
	      pos++;
	      avt_show_text_cursor (false);
	      avt.cursor.x += fontwidth;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_INSERT:
	  insert_mode = not insert_mode;
	  break;

	default:
	  // if valid character
	  if (pos < maxlen and ch >= 32 and avt_get_font_char (ch) != NULL)
	    {
	      // delete cursor
	      avt_show_text_cursor (false);

	      if (insert_mode and pos < len)
		{
		  if (len < maxlen)
		    {
		      avt_insert_spaces (1);
		      memmove (&s[pos + 1], &s[pos],
			       (len - pos) * sizeof (wchar_t));
		      len++;
		    }
		  else		// len >= maxlen
		    memmove (&s[pos + 1], &s[pos],
			     (len - pos - 1) * sizeof (wchar_t));
		}

	      s[pos] = (wchar_t) ch;
	      avt_drawchar (ch, screen);
	      avt_showchar ();
	      pos++;
	      if (pos > len)
		len++;
	      if (pos > maxlen - 1)
		{
		  pos--;	// cursor stays where it is
		  bell ();
		}
	      else if (avt.textdir_rtl)
		avt.cursor.x -= fontwidth;
	      else
		avt.cursor.x += fontwidth;
	    }

	  break;
	}
    }

  s[len] = L'\0';

  avt_show_text_cursor (false);

  avt.textdir_rtl = old_textdir;

  return ch;
}

// size in Bytes!
extern int
avt_ask (wchar_t * s, size_t size)
{
  avt_input (s, size, NULL, -1, 0);
  return _avt_STATUS;
}

extern int
avt_move_in (void)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // fill the screen with background color
  // (not only the window!)
  avt_clear_screen ();

  if (avt.avatar_image)
    {
      struct avt_position pos;
      short destination;
      size_t start_time;

      pos.x = screen->width;

      if (AVT_HEADER == avt.avatar_mode)
	pos.y = avt.window.y + TOPMARGIN;
      else			// bottom
	pos.y =
	  avt.window.y + avt.window.height - avt.avatar_image->height -
	  AVATAR_MARGIN;

      start_time = avt_ticks ();

      if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	destination =
	  avt.window.x + (avt.window.width / 2) -
	  (avt.avatar_image->width / 2);
      else			// left
	destination = avt.window.x + AVATAR_MARGIN;

      while (pos.x > destination)
	{
	  short oldx = pos.x;

	  // move
	  pos.x = screen->width - (avt_elapsed (start_time) / MOVE_DELAY);

	  if (pos.x != oldx)
	    {
	      // draw
	      avt_put_graphic (avt.avatar_image, screen, pos.x, pos.y);

	      // update
	      if ((oldx + avt.avatar_image->width) >= screen->width)
		backend.update_area (screen, pos.x, pos.y,
				     screen->width - pos.x,
				     avt.avatar_image->height);
	      else
		backend.update_area (screen, pos.x, pos.y,
				     avt.avatar_image->width
				     + (oldx - pos.x),
				     avt.avatar_image->height);

	      // delete (not visibly yet)
	      avt_bar (screen, pos.x, pos.y,
		       avt.avatar_image->width, avt.avatar_image->height,
		       avt.background_color);
	    }

	  // check event
	  if (avt_update ())
	    return _avt_STATUS;
	}

      // final position
      avt_show_avatar ();
    }

  return _avt_STATUS;
}

extern int
avt_move_out (void)
{
  if (not screen or not avt.avatar_visible or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // needed to remove the balloon
  avt_show_avatar ();

  if (avt.avatar_image)
    {
      struct avt_position pos;
      size_t start_time;
      short start_position;

      if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	start_position =
	  avt.window.x + (avt.window.width / 2) -
	  (avt.avatar_image->width / 2);
      else
	start_position = avt.window.x + AVATAR_MARGIN;

      pos.x = start_position;

      if (AVT_HEADER == avt.avatar_mode)
	pos.y = avt.window.y + TOPMARGIN;
      else			// bottom
	pos.y =
	  avt.window.y + avt.window.height - avt.avatar_image->height -
	  AVATAR_MARGIN;

      start_time = avt_ticks ();

      // delete (not visibly yet)
      avt_bar (screen, pos.x, pos.y, avt.avatar_image->width,
	       avt.avatar_image->height, avt.background_color);

      while (pos.x < screen->width)
	{
	  short oldx;

	  oldx = pos.x;

	  // move
	  pos.x = start_position + (avt_elapsed (start_time) / MOVE_DELAY);

	  if (pos.x != oldx)
	    {
	      // draw
	      avt_put_graphic (avt.avatar_image, screen, pos.x, pos.y);

	      // update
	      if ((pos.x + avt.avatar_image->width) >= screen->width)
		backend.update_area (screen, oldx, pos.y,
				     screen->width - oldx,
				     avt.avatar_image->height);
	      else
		backend.update_area (screen, oldx, pos.y,
				     avt.avatar_image->width
				     + pos.x - oldx,
				     avt.avatar_image->height);

	      // delete (not visibly yet)
	      avt_bar (screen, pos.x, pos.y,
		       avt.avatar_image->width, avt.avatar_image->height,
		       avt.background_color);
	    }

	  // check event
	  if (avt_update ())
	    return _avt_STATUS;
	}
    }

  // fill the whole screen with background color
  avt_clear_screen ();

  return _avt_STATUS;
}

extern int
avt_wait_button (void)
{
  avt_char old_buttons_key, old_motion_key;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // alignment: right bottom
  avt_show_button (avt.window.width - BASE_BUTTON_WIDTH - AVATAR_MARGIN,
		   avt.window.height - BASE_BUTTON_HEIGHT - AVATAR_MARGIN,
		   btn_right, AVT_KEY_ENTER, AVT_BUTTON_COLOR);

  old_motion_key = avt_set_pointer_motion_key (0);	// ignore moves
  old_buttons_key = avt_set_pointer_buttons_key (AVT_KEY_ENTER);

  avt_clear_keys ();
  avt_get_key ();

  avt_clear_buttons ();
  avt_set_pointer_motion_key (old_motion_key);
  avt_set_pointer_buttons_key (old_buttons_key);
  avt_clear_keys ();

  return _avt_STATUS;
}

/*
 * maximum number of displayable navigation buttons
 * for the smallest resolution
 */
#define NAV_MAX MAX_BUTTONS

extern int
avt_navigate (const char *buttons)
{
  int button_count;
  avt_char audio_end_button, old_audio_key;
  struct avt_position button;
  int result;

  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (not screen)
    return AVT_ERROR;

  result = AVT_ERROR;		// no result
  audio_end_button = old_audio_key = 0;	// none
  button_count = strlen (buttons);

  if (not buttons or not * buttons or button_count > NAV_MAX)
    {
      avt_set_error ("No or too many buttons for navigation bar");
      return AVT_FAILURE;
    }

  // display buttons:
  button.x = avt.window.width - AVATAR_MARGIN
    - (button_count * (BASE_BUTTON_WIDTH + BUTTON_DISTANCE)) +
    BUTTON_DISTANCE;

  button.y = avt.window.height - BASE_BUTTON_HEIGHT - AVATAR_MARGIN;

  for (int i = 0; i < button_count; i++)
    {
      switch (buttons[i])
	{
	case 'l':
	  avt_show_button (button.x, button.y, btn_left, L'l',
			   AVT_BUTTON_COLOR);
	  break;

	case 'd':
	  avt_show_button (button.x, button.y, btn_down, L'd',
			   AVT_BUTTON_COLOR);
	  break;

	case 'u':
	  avt_show_button (button.x, button.y, btn_up, L'u',
			   AVT_BUTTON_COLOR);
	  break;

	case 'r':
	  avt_show_button (button.x, button.y, btn_right, L'r',
			   AVT_BUTTON_COLOR);
	  break;

	case 'x':
	  avt_show_button (button.x, button.y, btn_cancel, L'x',
			   AVT_BUTTON_COLOR);
	  break;

	case 's':
	  avt_show_button (button.x, button.y, btn_stop, L's',
			   AVT_BUTTON_COLOR);
	  if (not audio_end_button)	// 'f' has precedence
	    audio_end_button = 's';
	  break;

	case 'f':
	  avt_show_button (button.x, button.y, btn_fastforward, L'f',
			   AVT_BUTTON_COLOR);
	  audio_end_button = 'f';	// this has precedence
	  break;

	case 'b':
	  avt_show_button (button.x, button.y, btn_fastbackward, L'b',
			   AVT_BUTTON_COLOR);
	  break;

	case '+':
	  avt_show_button (button.x, button.y, btn_yes, L'+',
			   AVT_BUTTON_COLOR);
	  break;

	case '-':
	  avt_show_button (button.x, button.y, btn_no, L'-',
			   AVT_BUTTON_COLOR);
	  break;

	case 'p':
	  avt_show_button (button.x, button.y, btn_pause, L'p',
			   AVT_BUTTON_COLOR);
	  break;

	case '?':
	  avt_show_button (button.x, button.y, btn_help, L'?',
			   AVT_BUTTON_COLOR);
	  break;

	case 'e':
	  avt_show_button (button.x, button.y, btn_eject, L'e',
			   AVT_BUTTON_COLOR);
	  break;

	case '*':
	  avt_show_button (button.x, button.y, btn_circle, L'*',
			   AVT_BUTTON_COLOR);
	  break;

	default:
	  /*
	   * empty button allowed
	   * for compatibility to buttons in later versions
	   */
	  break;
	}

      button.x += BASE_BUTTON_WIDTH + BUTTON_DISTANCE;
    }

  if (audio_end_button)
    old_audio_key = avt_set_audio_end_key (audio_end_button);

  // check button presses

  while (result < 0 and _avt_STATUS == AVT_NORMAL)
    {
      avt_char ch;
      ch = avt_get_key ();

      switch (ch)
	{
	case AVT_KEY_UP:
	case AVT_KEY_HOME:
	  ch = L'u';
	  break;

	case AVT_KEY_DOWN:
	case AVT_KEY_END:
	  ch = L'd';
	  break;

	case AVT_KEY_LEFT:
	  ch = L'l';
	  break;

	case AVT_KEY_RIGHT:
	  ch = L'r';
	  break;

	case AVT_KEY_HELP:
	case AVT_KEY_F1:
	  ch = L'?';
	  break;

	case AVT_KEY_PAGEDOWN:
	  ch = L'f';
	  break;

	case AVT_KEY_PAGEUP:
	  ch = L'b';
	  break;
	}

      // check if it is one of the requested characters
      const char *b = buttons;
      while (*b)
	if (ch == (avt_char) * b++)
	  result = ch;
    }

  avt_clear_buttons ();
  avt_set_audio_end_key (old_audio_key);

  if (_avt_STATUS != AVT_NORMAL)
    result = _avt_STATUS;

  return result;
}

extern bool
avt_decide (void)
{
  struct avt_position yes_button, no_button;
  int result;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // alignment: right bottom
  yes_button.x = avt.window.width - BASE_BUTTON_WIDTH - AVATAR_MARGIN;
  yes_button.y = avt.window.height - BASE_BUTTON_HEIGHT - AVATAR_MARGIN;

  no_button.x = yes_button.x - BUTTON_DISTANCE - BASE_BUTTON_WIDTH;
  no_button.y = yes_button.y;

  // draw buttons
  avt_show_button (yes_button.x, yes_button.y, btn_yes, L'+', 0x00AA00);
  avt_show_button (no_button.x, no_button.y, btn_no, L'-', 0xAA0000);

  avt_clear_keys ();
  result = -1;			// no result
  while (result < 0 and _avt_STATUS == AVT_NORMAL)
    {
      avt_char ch;
      ch = avt_get_key ();

      if (L'-' == ch or L'0' == ch or AVT_KEY_BACKSPACE == ch
	  or AVT_KEY_ESCAPE == ch)
	result = false;
      else if (L'+' == ch or L'1' == ch or AVT_KEY_ENTER == ch)
	result = true;
    }

  avt_clear_buttons ();
  avt_clear_keys ();

  return (result > 0);
}


static void
avt_show_image (avt_graphic * image)
{
  struct avt_position pos;

  // clear the screen
  avt_free_screen ();

  // set informational variables
  avt_no_textfield ();
  avt.avatar_visible = false;

  // center image on screen
  pos.x = (screen->width / 2) - (image->width / 2);
  pos.y = (screen->height / 2) - (image->height / 2);

  // eventually increase inner window - never decrease!
  if (image->width > avt.window.width)
    {
      if (image->width <= screen->width)
	avt.window.width = image->width;
      else
	avt.window.width = screen->width;
      avt.window.x = (screen->width / 2) - (avt.window.width / 2);
    }

  if (image->height > avt.window.height)
    {
      if (image->height <= screen->height)
	avt.window.height = image->height;
      else
	avt.window.height = screen->height;
      avt.window.y = (screen->height / 2) - (avt.window.height / 2);
    }

  /*
   * if image is larger than the screen,
   * just the upper left part is shown, as far as it fits
   */
  avt_put_graphic (image, screen, pos.x, pos.y);
  avt_update_all ();
  avt_update ();
}


extern int
avt_show_image_file (const char *filename)
{
  avt_graphic *image;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  image = avt_load_image_file (filename);

  if (not image)
    {
      avt_clear_screen ();	// at least clear the screen
      avt_set_error ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


extern int
avt_show_image_stream (avt_stream * stream)
{
  avt_graphic *image;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  image = avt_load_image_stream (stream);

  if (not image)
    {
      avt_clear_screen ();	// at least clear the screen
      avt_set_error ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


extern int
avt_show_image_data (void *data, size_t size)
{
  avt_graphic *image;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  image = avt_load_image_memory (data, size);

  if (not image)
    {
      avt_clear_screen ();	// at least clear the screen
      avt_set_error ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


extern int
avt_show_image_xpm (char **xpm)
{
  avt_graphic *image = NULL;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  image = avt_load_image_xpm (xpm);

  if (not image)
    {
      avt_clear_screen ();	// at least clear the screen
      avt_set_error ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


extern int
avt_show_image_xbm (const unsigned char *bits, int width, int height,
		    int color)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  avt_clear_screen ();

  if (width <= 0 or height <= 0 or color < 0)
    {
      avt_set_error ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_put_image_xbm (screen,
		     (screen->width / 2) - (width / 2),
		     (screen->height / 2) - (height / 2),
		     bits, width, height, color);

  return _avt_STATUS;
}


extern int
avt_image_max_width (void)
{
  return screen->width;
}


extern int
avt_image_max_height (void)
{
  return screen->height;
}

/*
 * show raw image
 * only 4 Bytes per pixel supported (0RGB)
 */
extern int
avt_show_raw_image (void *image_data, int width, int height)
{
  if (not screen or _avt_STATUS != AVT_NORMAL or not image_data)
    return _avt_STATUS;

  // check if it's a different image
  if (raw_image
      and (width != raw_image->width or height != raw_image->height
	   or image_data != raw_image->pixels))
    avt_release_raw_image ();

  if (not raw_image)
    raw_image = avt_data_to_graphic (image_data, width, height);

  if (not raw_image)
    {
      avt_clear_screen ();	// at least clear the screen
      avt_set_error ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (raw_image);

  return _avt_STATUS;
}


static int
avt_put_raw_image (avt_graphic * image, int x, int y,
		   void *image_data, int width, int height)
{
  avt_graphic *dest;

  dest = avt_data_to_graphic (image_data, width, height);

  if (not dest)
    {
      avt_set_error ("export_image");
      return AVT_FAILURE;
    }

  avt_put_graphic (image, dest, x, y);

  avt_free_graphic (dest);

  return _avt_STATUS;
}

extern int
avt_put_raw_image_file (const char *file, int x, int y,
			void *image_data, int width, int height)
{
  int status;
  avt_graphic *image;

  if (not file or not * file)
    return AVT_FAILURE;

  if (not screen or _avt_STATUS != AVT_NORMAL or not image_data)
    return _avt_STATUS;

  status = _avt_STATUS;
  image = avt_load_image_file (file);

  if (image)
    {
      status = avt_put_raw_image (image, x, y, image_data, width, height);

      avt_free_graphic (image);
    }

  return status;
}

extern int
avt_put_raw_image_stream (avt_stream * stream, int x, int y,
			  void *image_data, int width, int height)
{
  int status;
  avt_graphic *image;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  status = _avt_STATUS;
  image = avt_load_image_stream (stream);

  if (image)
    {
      status = avt_put_raw_image (image, x, y, image_data, width, height);

      avt_free_graphic (image);
    }

  return status;
}

extern int
avt_put_raw_image_data (void *img, size_t imgsize, int x, int y,
			void *image_data, int width, int height)
{
  int status;
  avt_graphic *image;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  status = _avt_STATUS;
  image = avt_load_image_memory (img, imgsize);

  if (image)
    {
      status = avt_put_raw_image (image, x, y, image_data, width, height);

      avt_free_graphic (image);
    }

  return status;
}

extern int
avt_put_raw_image_xpm (char **xpm, int x, int y,
		       void *image_data, int width, int height)
{
  avt_graphic *src, *dest;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  src = dest = NULL;

  src = avt_load_image_xpm (xpm);

  if (not src)
    return AVT_FAILURE;

  dest = avt_data_to_graphic (image_data, width, height);

  if (not dest)
    {
      avt_free_graphic (src);
      avt_set_error ("export_image");
      return AVT_FAILURE;
    }

  avt_put_graphic (src, dest, x, y);

  avt_free_graphic (dest);
  avt_free_graphic (src);

  return _avt_STATUS;
}


/*
 * make background transparent
 * pixel in the upper left corner is supposed to be the background color
 */
static avt_graphic *
avt_make_transparent (avt_graphic * image)
{
  avt_set_color_key (image, *image->pixels);

  return image;
}

// change avatar image and (re)calculate balloon size
static int
avt_set_avatar_image (avt_graphic * image)
{
  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (avt.avatar_visible)
    avt_clear_screen ();

  // free old image
  if (avt.avatar_image)
    {
      avt_free_graphic (avt.avatar_image);
      avt.avatar_image = NULL;
    }

  if (avt.name)
    {
      free (avt.name);
      avt.name = NULL;
    }

  // import the avatar image
  avt.avatar_image = avt_copy_graphic (image);
  calculate_balloonmaxheight ();

  // set actual balloon size to the maximum size
  avt.balloonheight = avt.balloonmaxheight;
  avt.balloonwidth = AVT_LINELENGTH;

  return _avt_STATUS;
}


extern int
avt_avatar_image_none (void)
{
  avt_set_avatar_image (NULL);

  return _avt_STATUS;
}


extern int
avt_avatar_image_xpm (char **xpm)
{
  avt_graphic *image;

  image = avt_load_image_xpm (xpm);

  if (not image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_xbm (const unsigned char *bits,
		      int width, int height, int color)
{
  avt_graphic *image;

  if (width <= 0 or height <= 0 or color < 0 or color > 0xFFFFFF)
    {
      avt_set_error ("invalid parameters");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  image = avt_load_image_xbm (bits, width, height, color);

  if (not image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


static inline int
avt_avatar_image (avt_graphic * image)
{
  if (not image)
    return AVT_FAILURE;

  // if it's not yet transparent, make it transparent
  if (not image->transparent)
    avt_make_transparent (image);

  avt_set_avatar_image (image);
  avt_free_graphic (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_data (void *img, size_t imgsize)
{
  return avt_avatar_image (avt_load_image_memory (img, imgsize));
}


extern int
avt_avatar_image_file (const char *file)
{
  return avt_avatar_image (avt_load_image_file (file));
}


extern int
avt_avatar_image_stream (avt_stream * stream)
{
  return avt_avatar_image (avt_load_image_stream (stream));
}


extern int
avt_set_avatar_name (const wchar_t * name)
{
  // clear old name
  if (avt.name)
    {
      free (avt.name);
      avt.name = NULL;
    }

  // copy name
  if (name and * name)
    {
      int size = (wcslen (name) + 1) * sizeof (wchar_t);
      avt.name = (wchar_t *) malloc (size);
      memcpy (avt.name, name, size);
    }

  if (avt.avatar_visible)
    avt_show_avatar ();

  return _avt_STATUS;
}

extern void
avt_set_text_background_ballooncolor (void)
{
  avt.text_background_color = avt.ballooncolor;
}

// can and should be called before avt_initialize
extern void
avt_set_balloon_color (int color)
{
  if (color >= 0)
    {
      avt.ballooncolor = color;

      if (screen)
	{
	  avt_set_text_background_ballooncolor ();

	  // redraw the balloon, if it is visible
	  if (avt.textfield.x >= 0)
	    avt_draw_balloon ();
	}
    }
}

extern int
avt_get_balloon_color (void)
{
  return avt.ballooncolor;
}

// can and should be called before avt_initialize
extern void
avt_set_background_color (int color)
{
  if (color >= 0)
    {
      avt.background_color = color;

      if (screen)
	{
	  if (avt.textfield.x >= 0)
	    {
	      avt.avatar_visible = false;	// force to redraw everything
	      avt_draw_balloon ();
	    }
	  else if (avt.avatar_visible)
	    avt_show_avatar ();
	  else
	    avt_clear_screen ();
	}
    }
}

extern int
avt_get_background_color (void)
{
  return avt.background_color;
}

extern void
avt_set_bitmap_color (int color)
{
  avt.bitmap_color = color;
}

extern void
avt_set_text_color (int colornr)
{
  avt.text_color = colornr;
}

extern void
avt_set_text_background_color (int colornr)
{
  avt.text_background_color = colornr;
}

extern void
avt_inverse (bool onoff)
{
  avt.inverse = onoff;
}

extern bool
avt_get_inverse (void)
{
  return avt.inverse;
}

extern void
avt_bold (bool onoff)
{
  avt.bold = onoff;
}

extern bool
avt_get_bold (void)
{
  return avt.bold;
}

extern void
avt_underlined (bool onoff)
{
  avt.underlined = onoff;
}

extern bool
avt_get_underlined (void)
{
  return avt.underlined;
}

extern void
avt_normal_text (void)
{
  avt.underlined = avt.bold = avt.inverse = avt.markup = false;

  avt.text_color = AVT_COLOR_BLACK;
  avt.text_background_color = avt.ballooncolor;
}

extern void
avt_markup (bool onoff)
{
  avt.markup = onoff;
  avt.underlined = avt.bold = false;
}

extern void
avt_set_text_delay (int delay)
{
  avt.text_delay = delay;

  // eventually switch off updates lock
  if (avt.text_delay != 0 and avt.hold_updates)
    avt_lock_updates (false);
}

extern void
avt_set_flip_page_delay (int delay)
{
  avt.flip_page_delay = delay;
}

extern void
avt_set_scroll_mode (int mode)
{
  avt.scroll_mode = mode;
}

extern int
avt_get_scroll_mode (void)
{
  return avt.scroll_mode;
}

#define CREDITDELAY 50

// scroll one line up
static void
avt_credits_up (avt_graphic * last_line)
{
  size_t now, next_time, tickinterval;
  short moved, pixel;

  moved = 0;
  pixel = 1;
  tickinterval = CREDITDELAY;
  next_time = avt_ticks () + tickinterval;

  while (moved <= fontheight)
    {
      // move screen up
      avt_graphic_segment (screen, avt.window.x, avt.window.y + pixel,
			   avt.window.width, avt.window.height - pixel,
			   screen, avt.window.x, avt.window.y);

      if (last_line)
	avt_put_graphic (last_line, screen, avt.window.x,
			 avt.window.y + avt.window.height - moved);

      avt_update_window ();

      if (avt_update ())
	return;

      moved += pixel;
      now = avt_ticks ();

      if (next_time > now)
	avt_delay (next_time - now);
      else
	{
	  // move more pixels at once and give more time next time
	  if (pixel < fontheight - moved)
	    {
	      pixel++;
	      tickinterval += CREDITDELAY;
	    }
	}

      next_time += tickinterval;
    }
}

extern int
avt_credits (const wchar_t * text, bool centered)
{
  wchar_t line[80];
  avt_graphic *last_line;
  int old_background_color;
  const wchar_t *p;
  int length;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // store old background color
  old_background_color = avt.background_color;

  // needed to handle resizing correctly
  avt_no_textfield ();
  avt.avatar_visible = false;
  avt.hold_updates = false;

  // the background-color is used when the window is resized
  // this implicitly also clears the screen
  avt_set_background_color (AVT_COLOR_BLACK);
  avt_set_text_background_color (AVT_COLOR_BLACK);
  avt_set_text_color (AVT_COLOR_WHITE);

  avt.window.x = (screen->width / 2) - (80 * fontwidth / 2);
  avt.window.width = 80 * fontwidth;
  // horizontal values unchanged

  // last line added to credits
  last_line = avt_new_graphic (avt.window.width, fontheight);

  if (not last_line)
    {
      avt_set_error ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // cursor position for last_line
  avt.cursor.y = 0;

  // show text
  p = text;
  while (*p and _avt_STATUS == AVT_NORMAL)
    {
      // get line
      length = 0;
      while (*p and not avt_is_linebreak (*p))
	{
	  if (*p >= L' ' and length < 80)
	    {
	      line[length] = *p;
	      length++;
	    }

	  p++;
	}

      // skip linebreak
      p++;

      // draw line
      if (centered)
	avt.cursor.x = (avt.window.width / 2) - (length * fontwidth / 2);
      else
	avt.cursor.x = 0;

      // clear line
      avt_fill (last_line, AVT_COLOR_BLACK);

      // print on last_line
      for (int i = 0; i < length; i++, avt.cursor.x += fontwidth)
	avt_drawchar ((avt_char) line[i], last_line);

      avt_credits_up (last_line);
    }

  // show one empty line to avoid streakes
  avt_fill (last_line, AVT_COLOR_BLACK);
  avt_credits_up (last_line);

  // scroll up until screen is empty
  for (int i = 0;
       i < avt.window.height / fontheight and _avt_STATUS == AVT_NORMAL; i++)
    avt_credits_up (NULL);

  avt_free_graphic (last_line);
  avt_avatar_window ();

  // back to normal (also sets variables!)
  avt_set_background_color (old_background_color);
  avt_normal_text ();
  avt_clear_screen ();

  return _avt_STATUS;
}

// must be replaced by the backend
static void
default_error_function (void)
{
  avt_set_error ("backend function missing");
  _avt_STATUS = AVT_ERROR;
}

// must be replaced by the backend
static void
update_area_error (avt_graphic * screen, int x, int y, int width, int height)
{
  (void) screen;
  (void) x;
  (void) y;
  (void) width;
  (void) height;

  default_error_function ();
}

extern void
avt_quit (void)
{
  if (avt.quit_encoding)
    {
      avt.quit_encoding ();
      avt.quit_encoding = NULL;
    }

  if (avt.quit_audio)
    {
      avt.quit_audio ();
      avt.quit_audio = NULL;
    }

  avt_release_raw_image ();

  avt_set_error ("15ce822f94d7e8e4281f1c2bcdd7c56d");
  avt_set_error (NULL);

  if (screen)
    {
      avt_free_graphic (base_button);
      base_button = NULL;
      avt_free_graphic (avt.avatar_image);
      avt.avatar_image = NULL;
      avt_free_graphic (avt.cursor_character);
      avt.cursor_character = NULL;
      avt.bell = NULL;
      avt_free_graphic (screen);
      screen = NULL;
      avt.avatar_visible = false;
      avt_no_textfield ();
    }

  if (backend.quit)
    {
      backend.quit ();
      backend.quit = NULL;
    }

  backend.resize = NULL;
  backend.graphic_file = NULL;
  backend.graphic_stream = NULL;
  backend.graphic_memory = NULL;
}

extern void
avt_button_quit (void)
{
  avt_wait_button ();
  avt_move_out ();
  avt_quit ();
}


extern void
avt_reset ()
{
  _avt_STATUS = AVT_NORMAL;
  avt.newline_mode = true;
  avt.auto_margin = true;
  avt.origin_mode = true;	// for backwards compatibility
  avt.scroll_mode = 1;
  avt.textdir_rtl = AVT_LEFT_TO_RIGHT;
  avt.flip_page_delay = AVT_DEFAULT_FLIP_PAGE_DELAY;
  avt.text_delay = 0;
  avt.bitmap_color = AVT_COLOR_BLACK;
  avt.ballooncolor = AVT_BALLOON_COLOR;

  avt_clear_keys ();
  avt_reserve_single_keys (false);
  avt_set_pointer_buttons_key (0);
  avt_set_pointer_motion_key (0);
  avt_clear_screen ();		// also resets some variables
  avt_normal_text ();
  avt_reset_tab_stops ();
  avt_set_avatar_mode (AVT_SAY);
  avt_set_mouse_visible (true);
  avt_set_avatar_name (NULL);

  if (avt.avatar_image)
    avt_set_avatar_image (NULL);
}

extern struct avt_backend *
avt_start_common (avt_graphic * new_screen)
{
  // already initialized?
  if (screen)
    {
      avt_set_error ("AKFAvatar already initialized");
      _avt_STATUS = AVT_ERROR;
      return NULL;
    }

  /*
   * The backend structure is a static variable.
   * So it is automatically initialized,
   * which is okay for most functions except the following.
   * Those must be replaced by the backend.
   */
  backend.update_area = &update_area_error;
  backend.wait_key = &default_error_function;

  avt_reset ();

  if (new_screen)
    screen = new_screen;
  else
    screen = avt_new_graphic (MINIMALWIDTH, MINIMALHEIGHT);

  if (screen->width < MINIMALWIDTH or screen->height < MINIMALHEIGHT)
    {
      avt_set_error ("screen too small");
      _avt_STATUS = AVT_ERROR;
      return NULL;
    }

  avt_fill (screen, avt.background_color);

  avt_get_font_dimensions (&fontwidth, &fontheight, &fontunderline);

  // fine-tuning: avoid conflict between underscore and underlining
  if (fontheight >= 18)
    ++fontunderline;

  avt_avatar_window ();
  avt_normal_text ();
  base_button = avt_load_image_xpm (btn_xpm);

  // set actual balloon size to the maximum size
  avt.balloonheight = avt.balloonmaxheight;
  avt.balloonwidth = AVT_LINELENGTH;

  // reserve space for character under text-mode cursor
  avt.cursor_character = avt_new_graphic (fontwidth, fontheight);

  if (not avt.cursor_character)
    {
      avt_set_error ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return NULL;
    }

  // visual flash for the bell
  // when you initialize the audio stuff, you get an audio bell
  avt.bell = avt_flash;

  return &backend;
}
