/*
 * internal prototypes for AKFAvatar
 * Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is part of AKFAvatar
 * This file is not part of the official API
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

#ifndef AVTINTERNALS_H
#define AVTINTERNALS_H

#include "akfavatar.h"
#include <stdio.h>		/* FILE */
#include <stdint.h>
#include <iso646.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"

// define an empty restrict unless the compiler is in C99 mode
#if not defined(__STDC_VERSION__) or __STDC_VERSION__ < 199901L
#define restrict
#endif

/*
 * this will be used, when somebody forgets to set the
 * encoding
 */
#define MB_DEFAULT_ENCODING "UTF-8"


// AVT_BYTE_ORDER
#ifdef AVT_BYTE_ORDER

#define AVT_LITTLE_ENDIAN  1234
#define AVT_BIG_ENDIAN     4321

#else

#if defined(__GLIBC__) or defined(__UCLIBC__) or defined(__dietlibc__)

#include <endian.h>

#define AVT_LITTLE_ENDIAN  __LITTLE_ENDIAN
#define AVT_BIG_ENDIAN     __BIG_ENDIAN
#define AVT_BYTE_ORDER     __BYTE_ORDER

#else // no endian.h

#define AVT_LITTLE_ENDIAN  1234
#define AVT_BIG_ENDIAN     4321

// big endian - FIXME: these may be wrong or at least incomplete...
#if not defined(__LITTLE_ENDIAN__) \
    and (defined(__sparc__) or defined(__ARMEB__) or defined(__MIPSEB__) \
    or defined(__ppc__) or defined(__POWERPC__) or defined(_M_PPC) \
    or defined(__hppa__))

#define AVT_BYTE_ORDER  AVT_BIG_ENDIAN

#else  // assume little endian as default

#define AVT_BYTE_ORDER  AVT_LITTLE_ENDIAN

#endif // not big endian system
#endif // no endian.h
#endif // not AVT_BYTE_ORDER

typedef uint_least32_t avt_color;

typedef struct avt_graphic
{
  short width, height;
  bool transparent;
  bool free_pixels;
  avt_color color_key;
  avt_color *pixels;
} avt_graphic;


struct avt_audio
{
  unsigned char *sound;		/* Pointer to sound data */
  size_t length;		/* Length of sound data in bytes */
  size_t capacity;		/* Capacity in bytes */
  int audio_type;		/* Type of raw data */
  int samplingrate;
  int channels;
  bool complete;
};

struct avt_position
{
  short x, y;
};

struct avt_area
{
  short x, y, width, height;
};

struct avt_settings
{
  avt_graphic *avatar_image;
  avt_graphic *cursor_character;
  wchar_t *name;

  // for an external keyboard/mouse handlers
  avt_keyhandler ext_keyhandler;
  avt_mousehandler ext_mousehandler;

  // delay values for printing text and flipping the page
  int text_delay, flip_page_delay;

  avt_color ballooncolor;
  avt_color background_color;
  avt_color text_color;
  avt_color text_background_color;
  avt_color bitmap_color;	// color for bitmaps

  avt_char pointer_motion_key;	// key simulated be pointer motion
  avt_char pointer_button_key;	// key simulated for mouse button 1-3

  bool newline_mode;		// when off, you need an extra CR
  bool underlined, bold, inverse;	// text underlined, bold?
  bool auto_margin;		// automatic new lines?
  bool avatar_visible;		// avatar visible?
  bool text_cursor_visible;	// shall the text cursor be visible?
  bool text_cursor_actually_visible;	// is it actually visible?
  bool reserve_single_keys;	// reserve single keys?
  bool markup;			// markup-syntax activated?
  bool hold_updates;		// holding updates back?
  bool tab_stops[AVT_LINELENGTH];

  // origin mode
  // Home: textfield (false) or viewport (true)
  // avt_initialize sets it to true for backwards compatibility
  bool origin_mode;

  char encoding[100];

  short int avatar_mode;
  short int scroll_mode;
  short int textdir_rtl;
  short int linestart;		// beginning of line - depending on text direction
  short int balloonheight, balloonmaxheight, balloonwidth;

  struct avt_position cursor, saved_position;

  struct avt_area textfield;
  struct avt_area viewport;	// sub-window in textfield
};


#define AVT_AUDIO_ENDED 1
#define AVT_TIMEOUT 2
#define AVT_PUSH_KEY 3

#define avt_isblank(c)  ((c) == ' ' or (c) == '\t')
#define avt_min(a, b) ((a) < (b) ? (a) : (b))
#define avt_max(a, b) ((a) > (b) ? (a) : (b))

/* avatar-sdl.c */
extern void avt_update_area (int x, int y, int width, int height);
extern avt_graphic *avt_load_image_file_sdl (const char *filename);
extern avt_graphic *avt_load_image_stream_sdl (avt_stream * stream);
extern avt_graphic *avt_load_image_memory_sdl (void *data, size_t size);
extern int avt_checkevent (void);
extern avt_char avt_set_pointer_motion_key (avt_char key);
extern avt_char avt_set_pointer_buttons_key (avt_char key);
extern void avt_get_pointer_position (int *x, int *y);

// TODO: reduce external functions
/* avatar.c */
extern int _avt_STATUS;
extern void (*avt_alert_func) (void);
extern void (*avt_quit_audio_func) (void);
extern avt_graphic *screen;
extern struct avt_area window;	// if screen is in fact larger

extern struct avt_settings *avt_start_common (avt_graphic *new_screen);
extern void avt_fill (avt_graphic * s, avt_color color);
extern avt_graphic *avt_get_window (void);
extern void avt_free_graphic (avt_graphic * gr);
extern avt_graphic * avt_load_image_xpm (char **xpm);
extern void avt_free_screen (void);
extern void avt_update_all (void);
extern void avt_put_graphic (avt_graphic * source, avt_graphic * destination,
		 int x, int y);
extern bool avt_check_buttons (int x, int y);
extern void avt_quit_common (void);

/* audio-sdl.c */
extern void avt_lock_audio (void);
extern void avt_unlock_audio (avt_audio *snd);

/* audio-common */
extern int avt_activate_audio_alert (void);
extern void avt_deactivate_audio_alert (void);

/* avtposix.c / avtwindows.c */
/* currently not used */
extern void get_user_home (char *home_dir, size_t size);
extern void edit_file (const char *name, const char *encoding);
extern FILE *open_config_file (const char *name, bool writing);

/* mingw/askdrive.c */
extern int avta_ask_drive (int max_idx);

#endif /* AVTINTERNALS_H */
