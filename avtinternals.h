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

#if defined(VGA)
#  define MINIMALWIDTH 640
#  define MINIMALHEIGHT 480
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 10
#  define AVATAR_MARGIN 10
   // Delay for moving in or out - the higher, the slower
#  define MOVE_DELAY 2.5
#else
#  define MINIMALWIDTH 800
#  define MINIMALHEIGHT 600
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 15
#  define AVATAR_MARGIN 20
   // Delay for moving in or out - the higher, the slower
#  define MOVE_DELAY 1.8
#endif // not VGA


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

#define avt_isblank(c)  ((c) == ' ' or (c) == '\t')
#define avt_min(a, b) ((a) < (b) ? (a) : (b))
#define avt_max(a, b) ((a) > (b) ? (a) : (b))

/* avatar-sdl.c */
extern void avt_update_area (avt_graphic *screen, int x, int y, int width, int height);
extern void avt_wait_key (void);
extern avt_char avt_set_pointer_motion_key (avt_char key);
extern avt_char avt_set_pointer_buttons_key (avt_char key);
extern void avt_get_pointer_position (int *x, int *y);

/* avatar.c */
extern int _avt_STATUS;

extern void avt_quit_backend_function (void (*f) (void));
extern void avt_clear_screen_function (void (*f) (avt_color background_color));
extern void avt_resize_function (
     void (*f) (avt_graphic * screen, int width, int height));
extern void avt_quit_audio_function (void (*f) (void));
extern void avt_alert_function (void (*f) (void));
extern void avt_image_loader_functions (
          avt_graphic * (*file) (const char *filename),
          avt_graphic * (*stream) (avt_stream * stream),
          avt_graphic * (*memory) (void *data, size_t size));

extern int avt_start_common (avt_graphic *new_screen);
extern avt_graphic *avt_data_to_graphic (void *data, short width, short height);
extern avt_graphic *avt_new_graphic (short width, short height);
extern void avt_fill (avt_graphic * s, avt_color color);
extern avt_graphic *avt_get_window (void);
extern void avt_free_graphic (avt_graphic * gr);
extern avt_graphic *avt_load_image_xpm (char **xpm);
extern void avt_free_screen (void);
extern void avt_update_all (void);
extern void avt_put_graphic (avt_graphic *source, avt_graphic *destination,
		 int x, int y);
extern bool avt_check_buttons (int x, int y);
extern void avt_add_key (avt_char key);
extern void avt_resize (int width, int height);

/* avttiming.c */
extern void avt_delay (int milliseconds); // only for under a second

/* audio-sdl.c */
extern void avt_lock_audio (void);
extern void avt_unlock_audio (avt_audio *snd);

/* audio-common */
extern int avt_start_audio_common (void);
extern void avt_quit_audio_backend_function (void (*f) (void));

/* avtposix.c / avtwindows.c */
/* currently not used */
extern void get_user_home (char *home_dir, size_t size);
extern void edit_file (const char *name, const char *encoding);
extern FILE *open_config_file (const char *name, bool writing);

/* mingw/askdrive.c */
extern int avta_ask_drive (int max_idx);

#endif /* AVTINTERNALS_H */
