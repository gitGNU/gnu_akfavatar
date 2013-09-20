/*
 * internal prototypes for AKFAvatar
 * Copyright (c) 2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
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

// the following symbols should not be exported by the library
#pragma GCC visibility push(hidden)

#include "avtdata.h"		// to get the hidden visibility
#include "avtgraphic.h"

#if defined(VGA)
#  define MINIMALWIDTH 640
#  define MINIMALHEIGHT 480
#else
#  define MINIMALWIDTH 800
#  define MINIMALHEIGHT 600
#endif // not VGA

#define AVT_COLOR_BLACK         0x000000
#define AVT_COLOR_WHITE         0xFFFFFF
#define AVT_COLOR_FLORAL_WHITE  0xFFFAF0
#define AVT_COLOR_TAN           0xD2B48C

// define an empty restrict unless the compiler is in C99 mode
#if not defined(__STDC_VERSION__) or __STDC_VERSION__ < 199901L
#define restrict
#endif

/*
 * this will be used by  the *_mb functions
 * when no encoding was set
 */
#define MB_DEFAULT_ENCODING "UTF-8"

// AVT_BYTE_ORDER
#ifdef AVT_BYTE_ORDER

#define AVT_LITTLE_ENDIAN  1234
#define AVT_BIG_ENDIAN     4321

#elif defined(__BYTE_ORDER__) and defined(__ORDER_LITTLE_ENDIAN__) \
       and defined(__ORDER_BIG_ENDIAN__)

#define AVT_LITTLE_ENDIAN  __ORDER_LITTLE_ENDIAN__
#define AVT_BIG_ENDIAN     __ORDER_BIG_ENDIAN__
#define AVT_BYTE_ORDER     __BYTE_ORDER__

#elif defined(__GLIBC__) or defined(__UCLIBC__) or defined(__dietlibc__)

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

#else // assume little endian as default

#define AVT_BYTE_ORDER  AVT_LITTLE_ENDIAN

#endif // not big endian system
#endif // not AVT_BYTE_ORDER


struct avt_audio
{
  unsigned char *sound;		/* Pointer to sound data */
  size_t length;		/* Length of sound data in bytes */
  int audio_type;		/* Type of raw data */
  int samplingrate;
  int channels;
  bool complete;

  size_t (*get) (avt_audio *self, void *data, size_t size);
  void (*rewind) (avt_audio *self);
  void (*done) (avt_audio *self);

  /* data for backends */
  /* may have different meanings */
  void *address;
  avt_data *data;
  size_t size2;
  size_t position;
};

struct avt_backend
{
  void (*update_area) (avt_graphic * screen, int x, int y, int width,
		       int height);
  void (*quit) (void);
  void (*wait_key) (void);
  void (*resize) (avt_graphic * screen, int width, int height);

  avt_graphic *(*graphic_file) (const char *filename);
  avt_graphic *(*graphic_stream) (avt_stream * stream);
  avt_graphic *(*graphic_memory) (void *data, size_t size);
};

#define avt_min(a, b) ((a) < (b) ? (a) : (b))
#define avt_max(a, b) ((a) > (b) ? (a) : (b))

/* avatar.c */
extern int _avt_STATUS;

void avt_quit_audio_function (void (*)(void));
void avt_quit_encoding_function (void (*)(void));
struct avt_backend *avt_start_common (avt_graphic * new_screen);
bool avt_check_buttons (int x, int y);
void avt_add_key (avt_char key);
avt_char avt_last_key (void);
void avt_resize (int width, int height);
void avt_update_all (void);

/* avttiming.c */
void avt_delay (int milliseconds);	// only for under a second

/* audio-sdl.c */
void avt_lock_audio (void);
void avt_unlock_audio (avt_audio * snd);

/* audio-common */
int avt_start_audio_common (void (*quit_backend) (void));

/* avtposix.c / avtwindows.c */
/* currently not used */
void get_user_home (char *home_dir, size_t size);
void edit_file (const char *name, const char *encoding);
FILE *open_config_file (const char *name, bool writing);

/* mingw/askdrive.c */
int avt_ask_drive (int max_idx);

#pragma GCC visibility pop

#endif /* AVTINTERNALS_H */
