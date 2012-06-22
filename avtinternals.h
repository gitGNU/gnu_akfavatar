/*
 * internal prototypes for AKFAvatar
 * Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
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

#ifndef AVTINTERNALS_H
#define AVTINTERNALS_H

#include "akfavatar.h"
#include <stdio.h>		/* FILE */

#define AVT_LITTLE_ENDIAN  1234
#define AVT_BIG_ENDIAN     4321

/* AVT_BYTE_ORDER */
#ifndef AVT_BYTE_ORDER
#if defined(__linux__)
#include <endian.h>
#define AVT_BYTE_ORDER  __BYTE_ORDER
#else /* not __linux__ */
#if defined(__sparc__) || defined(__MIPSEB__) \
    || defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) \
    || defined(__hppa__)
#define AVT_BYTE_ORDER  AVT_BIG_ENDIAN
#else
#define AVT_BYTE_ORDER  AVT_LITTLE_ENDIAN
#endif
#endif
#endif

struct avt_audio
{
  unsigned char *sound;		/* Pointer to sound data */
  size_t length;		/* Length of sound data in bytes */
  size_t capacity;		/* Capacity in bytes */
  int audio_type;		/* Type of raw data */
  int samplingrate;
  int channels;
};

#define AVT_AUDIO_ENDED 1
#define AVT_TIMEOUT 2

#define avt_isblank(c)  ((c) == ' ' || (c) == '\t')
#define avt_min(a, b) ((a) < (b) ? (a) : (b))
#define avt_max(a, b) ((a) > (b) ? (a) : (b))

/* avatar-sdl.c */
extern int _avt_STATUS;
extern int avt_checkevent (void);
extern int avt_wait_event (void);
extern void (*avt_alert_func) (void);
extern void (*avt_quit_audio_func) (void);

/* audio-sdl.c */
extern void avt_lock_audio (void);
extern void avt_unlock_audio (avt_audio *snd);

/* avtposix.c / avtwindows.c */
/* currently not used */
extern void get_user_home (char *home_dir, size_t size);
extern void edit_file (const char *name, const char *encoding);
extern FILE *open_config_file (const char *name, bool writing);

/* mingw/askdrive.c */
extern int avta_ask_drive (int max_idx);

#endif /* AVTINTERNALS_H */
