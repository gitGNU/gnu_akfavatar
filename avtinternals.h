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
#else
#  define MINIMALWIDTH 800
#  define MINIMALHEIGHT 600
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

#if defined(__GNUC__) and not defined(_WIN32)
#  define AVT_HIDDEN __attribute__((__visibility__("hidden")))
#else
#  define AVT_HIDDEN
#endif // __GNUC__


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

struct avt_backend
{
  void (*update_area) (avt_graphic *screen, int x, int y, int width, int height);
  void (*quit) (void);
  void (*wait_key) (void);
  void (*resize) (avt_graphic * screen, int width, int height);

  avt_graphic *(*graphic_file) (const char *filename);
  avt_graphic *(*graphic_stream) (avt_stream *stream);
  avt_graphic *(*graphic_memory) (void *data, size_t size);
};

#define avt_isblank(c)  ((c) == ' ' or (c) == '\t')
#define avt_min(a, b) ((a) < (b) ? (a) : (b))
#define avt_max(a, b) ((a) > (b) ? (a) : (b))

/* avatar.c */
AVT_HIDDEN extern int _avt_STATUS;

AVT_HIDDEN extern void avt_quit_audio_function (void (*) (void));

AVT_HIDDEN extern struct avt_backend *avt_start_common (avt_graphic *new_screen);
AVT_HIDDEN extern avt_graphic *avt_data_to_graphic (void *data, short width, short height);
AVT_HIDDEN extern avt_graphic *avt_new_graphic (short width, short height);
AVT_HIDDEN extern void avt_free_graphic (avt_graphic * gr);
AVT_HIDDEN extern avt_graphic *avt_load_image_xpm (char **xpm);
AVT_HIDDEN extern bool avt_check_buttons (int x, int y);
AVT_HIDDEN extern void avt_add_key (avt_char key);
AVT_HIDDEN extern void avt_resize (int width, int height);


/* avttiming.c */
AVT_HIDDEN extern void avt_delay (int milliseconds); // only for under a second

/* audio-sdl.c */
AVT_HIDDEN extern void avt_lock_audio (void);
AVT_HIDDEN extern void avt_unlock_audio (avt_audio *snd);

/* audio-common */
AVT_HIDDEN extern int avt_start_audio_common (void (*quit_backend) (void));

/* avtposix.c / avtwindows.c */
/* currently not used */
extern void get_user_home (char *home_dir, size_t size);
extern void edit_file (const char *name, const char *encoding);
extern FILE *open_config_file (const char *name, bool writing);

/* mingw/askdrive.c */
extern int avta_ask_drive (int max_idx);

#endif /* AVTINTERNALS_H */
