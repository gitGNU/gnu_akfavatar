/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for the audio-output
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99
 *
 * other software
 * required: SDL1.2
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

/* don't make functions deprecated for this file */
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "SDL.h"
#include "SDL_audio.h"
#include "alert.c"

/* lower audio buffer size for lower latency, but it could become choppy */
#define OUTPUT_BUFFER 4096

#ifndef _SDL_stdinc_h
#  define OLD_SDL 1
#endif

#ifdef OLD_SDL
#  include <stdlib.h>
#  undef SDL_malloc
#  define SDL_malloc              malloc
#  undef SDL_memcpy
#  define SDL_memcpy              memcpy
#  undef SDL_memcmp
#  define SDL_memcmp              memcmp
#  undef SDL_free
#  define SDL_free                free
#endif /* OLD_SDL */

#pragma GCC poison  malloc free strlen memcpy memcmp getenv putenv

struct avt_audio
{
  SDL_AudioSpec audiospec;
  uint8_t *sound;		/* Pointer to sound data */
  uint32_t len;			/* Length of sound data */
  uint32_t capacity;		/* Capacity in bytes */
  int audio_type;		/* Type of raw data */
  uint8_t wave;			/* wether SDL_FreeWav is needed? */
};

static bool avt_audio_initialized;

/* short sound for the "avt_bell" function */
static avt_audio *my_alert;

/* current sound */
static struct avt_audio current_sound;
static volatile int32_t soundpos = 0;	/* Current play position */
static volatile int32_t soundleft = 0;	/* Length of left unplayed wave data */
static bool loop = false;
static volatile bool playing = false;

/* table for decoding mu-law */
static const int16_t mulaw_decode[256] = {
  -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956, -23932,
  -22908, -21884, -20860, -19836, -18812, -17788, -16764, -15996, -15484,
  -14972, -14460, -13948, -13436, -12924, -12412, -11900, -11388, -10876,
  -10364, -9852, -9340, -8828, -8316, -7932, -7676, -7420, -7164, -6908,
  -6652, -6396, -6140, -5884, -5628, -5372, -5116, -4860, -4604, -4348,
  -4092, -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004, -2876,
  -2748, -2620, -2492, -2364, -2236, -2108, -1980, -1884, -1820, -1756,
  -1692, -1628, -1564, -1500, -1436, -1372, -1308, -1244, -1180, -1116,
  -1052, -988, -924, -876, -844, -812, -780, -748, -716, -684, -652, -620,
  -588, -556, -524, -492, -460, -428, -396, -372, -356, -340, -324, -308,
  -292, -276, -260, -244, -228, -212, -196, -180, -164, -148, -132, -120,
  -112, -104, -96, -88, -80, -72, -64, -56, -48, -40, -32, -24, -16, -8, 0,
  32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956, 23932, 22908,
  21884, 20860, 19836, 18812, 17788, 16764, 15996, 15484, 14972, 14460,
  13948, 13436, 12924, 12412, 11900, 11388, 10876, 10364, 9852, 9340, 8828,
  8316, 7932, 7676, 7420, 7164, 6908, 6652, 6396, 6140, 5884, 5628, 5372,
  5116, 4860, 4604, 4348, 4092, 3900, 3772, 3644, 3516, 3388, 3260, 3132,
  3004, 2876, 2748, 2620, 2492, 2364, 2236, 2108, 1980, 1884, 1820, 1756,
  1692, 1628, 1564, 1500, 1436, 1372, 1308, 1244, 1180, 1116, 1052, 988,
  924, 876, 844, 812, 780, 748, 716, 684, 652, 620, 588, 556, 524, 492, 460,
  428, 396, 372, 356, 340, 324, 308, 292, 276, 260, 244, 228, 212, 196, 180,
  164, 148, 132, 120, 112, 104, 96, 88, 80, 72, 64, 56, 48, 40, 32, 24, 16,
  8, 0
};

/* table for decoding A-law */
static const int16_t alaw_decode[256] = {
  -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736, -7552, -7296, -8064,
  -7808, -6528, -6272, -7040, -6784, -2752, -2624, -3008, -2880, -2240,
  -2112, -2496, -2368, -3776, -3648, -4032, -3904, -3264, -3136, -3520,
  -3392, -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
  -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136, -11008,
  -10496, -12032, -11520, -8960, -8448, -9984, -9472, -15104, -14592,
  -16128, -15616, -13056, -12544, -14080, -13568, -344, -328, -376, -360,
  -280, -264, -312, -296, -472, -456, -504, -488, -408, -392, -440, -424,
  -88, -72, -120, -104, -24, -8, -56, -40, -216, -200, -248, -232, -152,
  -136, -184, -168, -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
  -1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696, -688, -656, -752,
  -720, -560, -528, -624, -592, -944, -912, -1008, -976, -816, -784, -880,
  -848, 5504, 5248, 6016, 5760, 4480, 4224, 4992, 4736, 7552, 7296, 8064,
  7808, 6528, 6272, 7040, 6784, 2752, 2624, 3008, 2880, 2240, 2112, 2496,
  2368, 3776, 3648, 4032, 3904, 3264, 3136, 3520, 3392, 22016, 20992, 24064,
  23040, 17920, 16896, 19968, 18944, 30208, 29184, 32256, 31232, 26112,
  25088, 28160, 27136, 11008, 10496, 12032, 11520, 8960, 8448, 9984, 9472,
  15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568, 344, 328, 376,
  360, 280, 264, 312, 296, 472, 456, 504, 488, 408, 392, 440, 424, 88, 72,
  120, 104, 24, 8, 56, 40, 216, 200, 248, 232, 152, 136, 184, 168, 1376,
  1312, 1504, 1440, 1120, 1056, 1248, 1184, 1888, 1824, 2016, 1952, 1632,
  1568, 1760, 1696, 688, 656, 752, 720, 560, 528, 624, 592, 944, 912, 1008,
  976, 816, 784, 880, 848
};

/* this is the callback function */
static void
fill_audio (void *userdata, uint8_t * stream, int len)
{
  /* only play, when there is data left */
  if (soundleft <= 0)
    {
      if (loop)
	{
	  /* rewind to beginning */
	  soundpos = 0;
	  soundleft = current_sound.len;
	}
      else			/* no more data */
	{
	  SDL_Event event;

	  SDL_PauseAudio (1);	/* shut up */
	  playing = false;
	  event.type = SDL_USEREVENT;
	  event.user.code = AVT_AUDIO_ENDED;
	  event.user.data1 = event.user.data2 = NULL;
	  SDL_PushEvent (&event);
	  return;
	}
    }

  /* Copy as much data as possible */
  if (len > soundleft)
    len = soundleft;

  SDL_memcpy (stream, current_sound.sound + soundpos, len);

  soundpos += len;
  soundleft -= len;
}

static void
short_audio_sound (void)
{
  /* if my_alert is loaded and nothing is currently playing */
  if (my_alert && !playing)
    avt_play_audio (my_alert, AVT_PLAY);
}

/* must be called AFTER avt_start! */
extern int
avt_start_audio (void)
{
  if (!avt_audio_initialized)
    {
      if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
	{
	  SDL_SetError ("error initializing audio");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      /* set this before calling anything from this lib */
      avt_audio_initialized = true;

      my_alert = avt_load_audio_data ((void *) &avt_alert_data,
				      avt_alert_data_size, AVT_LOAD);
      avt_alert_func = short_audio_sound;
      avt_quit_audio_func = avt_quit_audio;
    }

  return _avt_STATUS;
}


#ifndef DISABLE_DEPRECATED
extern int
avt_initialize_audio (void)
{
  return avt_start_audio ();
}
#endif

/* stops audio */
extern void
avt_stop_audio (void)
{
  SDL_CloseAudio ();
  playing = false;
  soundpos = 0;
  soundleft = 0;
  loop = false;
  current_sound.len = 0;
  current_sound.sound = NULL;
}

extern void
avt_quit_audio (void)
{
  if (avt_audio_initialized)
    {
      avt_quit_audio_func = NULL;
      SDL_CloseAudio ();
      soundpos = 0;
      soundleft = 0;
      current_sound.len = 0;
      current_sound.sound = NULL;
      loop = false;
      playing = false;
      avt_alert_func = avt_flash;
      avt_free_audio (my_alert);
      my_alert = NULL;
      SDL_QuitSubSystem (SDL_INIT_AUDIO);
      avt_audio_initialized = false;
    }
}

static size_t
avt_required_audio_size (avt_audio * snd, size_t data_size)
{
  size_t out_size;

  switch (snd->audio_type)
    {
    case AVT_AUDIO_MULAW:
    case AVT_AUDIO_ALAW:
      out_size = 2 * data_size;	/* one byte becomes 2 bytes */
      break;

    case AVT_AUDIO_S24SYS:
    case AVT_AUDIO_S24LE:
    case AVT_AUDIO_S24BE:
      out_size = (data_size * 2) / 3;	/* reduced to 16 Bit */
      break;

    case AVT_AUDIO_S32SYS:
    case AVT_AUDIO_S32LE:
    case AVT_AUDIO_S32BE:
      out_size = data_size / 2;	/* reduced to 16 Bit */
      break;

    default:
      out_size = data_size;
      break;
    }

  return out_size;
}

extern int
avt_set_raw_audio_capacity (avt_audio * snd, size_t data_size)
{
  void *new_sound;
  size_t out_size;

  if (!snd)
    return AVT_FAILURE;

  new_sound = NULL;
  out_size = 0;

  if (data_size > 0)
    {
      out_size = avt_required_audio_size (snd, data_size);
      new_sound = SDL_realloc (snd->sound, out_size);

      if (new_sound == NULL)
	{
	  SDL_SetError ("out of memory");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}
    }

  snd->sound = (uint8_t *) new_sound;
  snd->capacity = out_size;

  /* eventually shrink length */
  if (snd->len > out_size)
    snd->len = out_size;

  return _avt_STATUS;
}

extern int
avt_add_raw_audio_data (avt_audio * snd, void *data, size_t data_size)
{
  size_t i, old_size, new_size, out_size;
  bool active;

  if (_avt_STATUS != AVT_NORMAL || snd == NULL || data == NULL
      || data_size == 0)
    return avt_checkevent ();

  /* audio structure must have been created with avt_load_raw_audio_data */
  if (snd->audio_type == AVT_AUDIO_UNKNOWN)
    {
      SDL_SetError ("unknown audio format");
      return AVT_FAILURE;
    }

  out_size = avt_required_audio_size (snd, data_size);
  old_size = snd->len;
  new_size = old_size + out_size;

  /* if it's currently playing, lock it */
  active = (playing && snd->sound == current_sound.sound);
  if (active)
    SDL_LockAudio ();

  /* eventually get more memory for output buffer */
  if (new_size > snd->capacity)
    {
      void *new_sound;
      size_t new_capacity;

      /* get twice the capacity */
      new_capacity = 2 * snd->capacity;

      /*
       * the capacity must never be lower than new_size
       * and it may still be 0
       */
      if (new_capacity < new_size)
	new_capacity = new_size;

      new_sound = SDL_realloc (snd->sound, new_capacity);

      if (new_sound == NULL)
	{
	  SDL_SetError ("out of memory");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      snd->sound = (uint8_t *) new_sound;
      snd->capacity = new_capacity;
    }

  /* convert or copy the data */
  switch (snd->audio_type)
    {
    case AVT_AUDIO_S16SYS:
    case AVT_AUDIO_U8:
    case AVT_AUDIO_S8:
      /* linear PCM, same bit size and endianness */
      SDL_memcpy (snd->sound + old_size, data, out_size);
      break;

    case AVT_AUDIO_MULAW:	/* mu-law, logarithmic PCM */
      {
	uint8_t *in;
	int16_t *out;

	in = (uint8_t *) data;
	out = (int16_t *) (snd->sound + old_size);
	for (i = data_size; i > 0; i--)
	  *out++ = mulaw_decode[*in++];
	break;
      }

    case AVT_AUDIO_ALAW:	/* A-law, logarithmic PCM */
      {
	uint8_t *in;
	int16_t *out;

	in = (uint8_t *) data;
	out = (int16_t *) (snd->sound + old_size);
	for (i = data_size; i > 0; i--)
	  *out++ = alaw_decode[*in++];
	break;
      }

    case AVT_AUDIO_S16LE:
      {
	uint8_t *in = (uint8_t *) data;
	uint16_t *out = (uint16_t *) (snd->sound + old_size);

	for (i = out_size / 2; i > 0; i--, in += 2)
	  *out++ = (in[1] << 8) | in[0];
      }
      break;

    case AVT_AUDIO_S16BE:
      {
	uint8_t *in = (uint8_t *) data;
	uint16_t *out = (uint16_t *) (snd->sound + old_size);

	for (i = out_size / 2; i > 0; i--, in += 2)
	  *out++ = (in[0] << 8) | in[1];
      }
      break;

      /* the following ones are all converted to 16 bits */

    case AVT_AUDIO_S24LE:
      {
	uint8_t *in = (uint8_t *) data;
	uint16_t *out = (uint16_t *) (snd->sound + old_size);

	for (i = out_size / sizeof (*out); i > 0; i--, in += 3)
	  *out++ = (in[2] << 8) | in[1];
      }
      break;

    case AVT_AUDIO_S24BE:
      {
	uint8_t *in = (uint8_t *) data;
	uint16_t *out = (uint16_t *) (snd->sound + old_size);

	for (i = out_size / sizeof (*out); i > 0; i--, in += 3)
	  *out++ = (in[0] << 8) | in[1];
      }
      break;

    case AVT_AUDIO_S32LE:
      {
	uint8_t *in = (uint8_t *) data;
	uint16_t *out = (uint16_t *) (snd->sound + old_size);

	for (i = out_size / sizeof (*out); i > 0; i--, in += 4)
	  *out++ = (in[3] << 8) | in[2];
      }
      break;

    case AVT_AUDIO_S32BE:
      {
	uint8_t *in = (uint8_t *) data;
	uint16_t *out = (uint16_t *) (snd->sound + old_size);

	for (i = out_size / sizeof (*out); i > 0; i--, in += 4)
	  *out++ = (in[0] << 8) | in[1];
      }
      break;

    default:
      SDL_SetError ("Internal error");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  snd->len = new_size;

  if (active)
    {
      current_sound.sound = snd->sound;	/* might have changed */
      current_sound.len = new_size;
      current_sound.capacity = snd->capacity;
      soundleft += out_size;
      SDL_UnlockAudio ();
    }

  return avt_checkevent ();
}

extern avt_audio *
avt_load_raw_audio_data (void *data, size_t data_size,
			 int samplingrate, int audio_type, int channels)
{
  int format;
  struct avt_audio *s;

  if (channels < 1 || channels > 2)
    {
      SDL_SetError ("only 1 or 2 channels supported");
      return NULL;
    }

  /* use NULL, if we have nothing to add, yet */
  if (data_size == 0)
    data = NULL;
  else if (data == NULL)
    data_size = 0;

  /* adjustments for later optimizations */
  if (SDL_LIL_ENDIAN == SDL_BYTEORDER)
    {
      switch (audio_type)
	{
	case AVT_AUDIO_S16LE:
	  audio_type = AVT_AUDIO_S16SYS;
	  break;

	case AVT_AUDIO_S24SYS:
	  audio_type = AVT_AUDIO_S24LE;
	  break;

	case AVT_AUDIO_S32SYS:
	  audio_type = AVT_AUDIO_S32LE;
	  break;
	}
    }
  else if (SDL_BIG_ENDIAN == SDL_BYTEORDER)
    {
      switch (audio_type)
	{
	case AVT_AUDIO_S16BE:
	  audio_type = AVT_AUDIO_S16SYS;
	  break;

	case AVT_AUDIO_S24SYS:
	  audio_type = AVT_AUDIO_S24BE;
	  break;

	case AVT_AUDIO_S32SYS:
	  audio_type = AVT_AUDIO_S32BE;
	  break;
	}
    }

  /* convert audio_type into SDL format number */
  switch (audio_type)
    {
    case AVT_AUDIO_U8:
      format = AUDIO_U8;
      break;

    case AVT_AUDIO_S8:
      format = AUDIO_S8;
      break;

    case AVT_AUDIO_S16LE:
    case AVT_AUDIO_S16BE:
    case AVT_AUDIO_S16SYS:
    case AVT_AUDIO_S24LE:
    case AVT_AUDIO_S24BE:
    case AVT_AUDIO_S24SYS:
    case AVT_AUDIO_S32LE:
    case AVT_AUDIO_S32BE:
    case AVT_AUDIO_S32SYS:
      /* size and endianess will get adjusted while loading */
      format = AUDIO_S16SYS;
      break;

    case AVT_AUDIO_MULAW:
    case AVT_AUDIO_ALAW:
      /* will be converted to S16SYS */
      format = AUDIO_S16SYS;
      break;

    default:
      SDL_SetError ("unsupported audio type");
      return NULL;
    }

  /* get memory for struct */
  s = (struct avt_audio *) SDL_malloc (sizeof (struct avt_audio));
  if (s == NULL)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  s->sound = NULL;
  s->len = 0;
  s->capacity = 0;
  s->audio_type = audio_type;
  s->wave = false;
  s->audiospec.format = format;
  s->audiospec.freq = samplingrate;
  s->audiospec.channels = channels;
  s->audiospec.samples = OUTPUT_BUFFER;
  s->audiospec.callback = fill_audio;
  s->audiospec.userdata = NULL;

  if (data_size == 0
      || avt_add_raw_audio_data (s, data, data_size) == AVT_NORMAL)
    return s;
  else
    {
      SDL_free (s);
      return NULL;
    }
}

extern void
avt_finalize_raw_audio (avt_audio * snd)
{
  /* eventually free unneeded memory */
  if (snd->capacity > snd->len)
    {
      void *new_sound;
      size_t new_capacity;

      new_capacity = snd->len;
      new_sound = SDL_realloc (snd->sound, new_capacity);

      if (new_sound)
	{
	  snd->sound = (uint8_t *) new_sound;
	  snd->capacity = new_capacity;
	}
    }
}

/* Is this sound currently playing? NULL for any sound */
extern bool
avt_audio_playing (avt_audio * snd)
{
  if (snd && snd->sound != current_sound.sound)
    return false;		/* not same sound */

  return playing;
}

extern void
avt_free_audio (avt_audio * snd)
{
  if (snd)
    {
      /* Is this sound currently playing? Then stop it! */
      if (playing && snd->sound == current_sound.sound)
	avt_stop_audio ();

      /* free the sound data */
      if (snd->wave)
	SDL_FreeWAV (snd->sound);
      else
	SDL_free (snd->sound);

      /* free the rest */
      SDL_free (snd);
    }
}

extern int
avt_play_audio (avt_audio * snd, int playmode)
{
  if (!avt_audio_initialized)
    return _avt_STATUS;

  /* no sound? - just ignore it */
  if (!snd)
    return _avt_STATUS;

  if (playmode != AVT_PLAY && playmode != AVT_LOOP)
    return AVT_FAILURE;

  /* close audio, in case it is left open */
  SDL_CloseAudio ();
  SDL_LockAudio ();

  /* load sound */
  current_sound.sound = snd->sound;
  current_sound.len = snd->len;
  current_sound.capacity = snd->capacity;
  current_sound.audiospec = snd->audiospec;
  current_sound.audiospec.callback = fill_audio;
  current_sound.audiospec.samples = OUTPUT_BUFFER;

  loop = (playmode == AVT_LOOP);

  if (SDL_OpenAudio (&current_sound.audiospec, NULL) == 0)
    {
      soundpos = 0;
      soundleft = current_sound.len;
      SDL_UnlockAudio ();
      playing = true;
      SDL_PauseAudio (0);
      return _avt_STATUS;
    }
  else
    {
      SDL_SetError ("error opening audio device");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }
}

extern int
avt_wait_audio_end (void)
{
  if (!playing)
    return _avt_STATUS;

  /* end the loop, but wait for end of sound */
  loop = false;

  while (playing && _avt_STATUS == AVT_NORMAL)
    avt_wait_event ();		/* end of audio also triggers event */

  return _avt_STATUS;
}

extern void
avt_pause_audio (bool pause)
{
  SDL_PauseAudio ((int) pause);
}
