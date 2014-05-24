/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for the audio-output
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013,2014
 * Andreas K. Foerster <info@akfoerster.de>
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

#define _ISOC99_SOURCE
#define _XOPEN_SOURCE 600

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "avtdata.h"
#include "SDL.h"
#include "SDL_audio.h"

#include <iso646.h>

// lower audio buffer size for lower latency, but it could become choppy
// this is the number of samples, not bytes
#define OUTPUT_BUFFER 1024

static bool avt_audio_initialized;

static volatile avt_audio *current_sound;
static volatile bool loop = false;
static avt_char audio_key;
static SDL_AudioSpec audiospec;

static void avt_quit_audio_sdl (void);

static void
audio_ended (void)
{
  SDL_PauseAudio (SDL_TRUE);
  current_sound = NULL;

  if (audio_key)
    avt_push_key (audio_key);
}

// callbacks

static void
get_audio (void *userdata, uint8_t * stream, int len)
{
  (void) userdata;
  avt_audio *snd = (avt_audio *) current_sound;
  int r = snd->get (snd, stream, len);

  if (r < len)
    {
      if (loop)
	{
	  snd->rewind (snd);
	  snd->get (snd, stream + r, len - r);
	}
      else			// no loop
	{
	  // clear rest of buffer
	  SDL_memset (stream + r, audiospec.silence, len - r);

	  if (r <= 0)		// nothing left
	    audio_ended ();
	}
    }
}

#ifdef AUDIO_S32LSB

// convert to 32bit, least sgnificant byte will be 0

static void
get_audio24 (void *userdata, uint8_t * stream, int len)
{
  (void) userdata;
  avt_audio *snd = (avt_audio *) current_sound;
  uint32_t *p;
  uint8_t buffer[(len * 3) / sizeof (*p)];
  uint8_t *b;
  int r;

get_sound:
  r = snd->get (snd, buffer, (len * 3) / sizeof (*p));
  r /= 3;			// number of samples read

  b = buffer;
  p = (uint32_t *) stream;

  // big or little endian?
  if (AVT_AUDIO_S24BE == snd->audio_type)
    {
      for (int i = r; i; --i, ++p, b += 3)
        {
	  register uint32_t sample = *((uint32_t *) b);
	  *p = SDL_SwapBE32 (sample) & 0xFFFFFF00u;
	}
    }
  else
    {
      for (int i = r; i; --i, ++p, b += 3)
        {
	  register uint32_t sample = *((uint32_t *) b);
	  *p = SDL_SwapLE32 (sample) << 8;
	}
    }

  if (r < len / (int) sizeof (*p))
    {
      if (loop)
	{
	  snd->rewind (snd);
	  stream += r * sizeof (*p);
	  len -= r * sizeof (*p);
	  if (len > 0)
	    goto get_sound;
	}
      else
	{
	  // clear rest of buffer
	  SDL_memset (stream + (r * sizeof (*p)),
		      audiospec.silence, len - (r * sizeof (*p)));

	  if (r <= 0)		// nothing left
	    audio_ended ();
	}
    }
}

#else // no 32bit support

// SDL-1.2 cleared the buffer before calling the callback

// convert to 16bit by dropping the least significant byte(s)

static void
get_audio24 (void *userdata, uint8_t * stream, int len)
{
  (void) userdata;
  avt_audio *snd = (avt_audio *) current_sound;
  uint16_t *p;
  uint8_t buffer[len * 3 / sizeof (*p)];
  uint8_t *b;
  int r;

get_sound:
  r = snd->get (snd, buffer, (len * 3) / sizeof (*p));
  r /= 3;			// number of samples read

  b = buffer;
  p = (uint16_t *) stream;

  // big or little endian?
  if (AVT_AUDIO_S24BE == snd->audio_type)
    {
      for (int i = r; i; --i, ++p, b += 3)
	{
	  register uint16_t sample = *((uint16_t *) b);
	  *p = SDL_SwapBE16 (sample);
	}
    }
  else
    {
      for (int i = r; i; --i, ++p, b += 3)
	{
	  register uint16_t sample = *((uint16_t *) (b + 1));
	  *p = SDL_SwapLE16 (sample);
	}
    }

  if (r < len / (int) sizeof (*p))
    {
      if (loop)
	{
	  snd->rewind (snd);
	  stream += r * sizeof (*p);
	  len -= r * sizeof (*p);
	  if (len > 0)
	    goto get_sound;
	}
      else if (r <= 0)		// nothing left
	audio_ended ();
    }
}


static void
get_audio32 (void *userdata, uint8_t * stream, int len)
{
  (void) userdata;
  avt_audio *snd = (avt_audio *) current_sound;
  uint16_t *p;
  uint32_t buffer[len / sizeof (*p)];
  uint32_t *b;
  int r;

get_sound:
  r = snd->get (snd, buffer, len * (sizeof (*b) / sizeof (*p)));
  r /= sizeof (*b);		// number of samples read

  b = buffer;
  p = (uint16_t *) stream;

  // big or little endian?
  if (AVT_AUDIO_S32BE == snd->audio_type)
    {
      for (int i = r; i; --i, ++p, ++b)
	*p = SDL_SwapBE32 (*b) >> 16;
    }
  else
    {
      for (int i = r; i; --i, ++p, ++b)
	*p = SDL_SwapLE32 (*b) >> 16;
    }

  if (r < len / (int) sizeof (*p))
    {
      if (loop)
	{
	  snd->rewind (snd);
	  stream += r * sizeof (*p);
	  len -= r * sizeof (*p);
	  if (len > 0)
	    goto get_sound;
	}
      else if (r <= 0)		// nothing left
	audio_ended ();
    }
}

#endif // no 32bit support


// must be called AFTER avt_start!
extern int
avt_start_audio (void)
{
  if (not avt_audio_initialized)
    {
      if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
	{
	  avt_set_error ("error initializing audio");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      SDL_memset (&audiospec, 0, sizeof (audiospec));

      // set this before calling anything from this lib
      avt_audio_initialized = true;

      avt_start_audio_common (&avt_quit_audio_sdl);
    }

  audio_key = 0;

  return _avt_STATUS;
}

// stops audio
extern void
avt_stop_audio (void)
{
  SDL_PauseAudio (SDL_TRUE);
  loop = false;
  audio_key = 0;
  current_sound = NULL;
}

static void
avt_quit_audio_sdl (void)
{
  if (avt_audio_initialized)
    {
      SDL_CloseAudio ();
      current_sound = NULL;
      SDL_memset (&audiospec, 0, sizeof (audiospec));
      loop = false;
      SDL_QuitSubSystem (SDL_INIT_AUDIO);
      avt_audio_initialized = false;
    }
}

extern void
avt_lock_audio (void)
{
  SDL_LockAudio ();
}

extern void
avt_unlock_audio (avt_audio * snd)
{
  current_sound = snd;
  SDL_UnlockAudio ();
}

// Is this sound currently playing? NULL for any sound
extern bool
avt_audio_playing (avt_audio * snd)
{
  return (current_sound and (not snd or current_sound == snd));
}

extern int
avt_play_audio (avt_audio * snd, int playmode)
{
  if (not avt_audio_initialized or not snd)
    return _avt_STATUS;

  if (playmode != AVT_PLAY and playmode != AVT_LOOP)
    return AVT_FAILURE;

  snd->rewind (snd);

  unsigned int format;

  switch (snd->audio_type)
    {
    case AVT_AUDIO_U8:
      format = AUDIO_U8;
      break;

    case AVT_AUDIO_S8:
      format = AUDIO_S8;
      break;

    case AVT_AUDIO_S16LE:
      format = AUDIO_S16LSB;
      break;

    case AVT_AUDIO_S16BE:
      format = AUDIO_S16MSB;
      break;

#ifdef AUDIO_S32LSB

    case AVT_AUDIO_S24LE:
      format = AUDIO_S32SYS;
      break;

    case AVT_AUDIO_S24BE:
      format = AUDIO_S32SYS;
      break;

    case AVT_AUDIO_S32LE:
      format = AUDIO_S32LSB;
      break;

    case AVT_AUDIO_S32BE:
      format = AUDIO_S32MSB;
      break;

#endif

    default:			// all other get converted
      format = AUDIO_S16SYS;
      break;
    }

  void *callback = get_audio;

  if (AVT_AUDIO_S24LE == snd->audio_type
      or AVT_AUDIO_S24BE == snd->audio_type)
    callback = get_audio24;

#ifndef AUDIO_S32LSB
  if (AVT_AUDIO_S32LE == snd->audio_type
      or AVT_AUDIO_S32BE == snd->audio_type)
    callback = get_audio32;
#endif

  // eventually (re)open audio device
  if (snd->samplingrate != audiospec.freq
      or snd->channels != audiospec.channels
      or format != audiospec.format
      or callback != (void *) audiospec.callback)
    {
      SDL_CloseAudio ();

      audiospec.format = format;
      audiospec.freq = snd->samplingrate;
      audiospec.channels = snd->channels;
      audiospec.samples = OUTPUT_BUFFER;
      audiospec.callback = callback;

      if (SDL_OpenAudio (&audiospec, NULL) != 0)
	{
	  SDL_memset (&audiospec, 0, sizeof (audiospec));
	  avt_set_error ("error opening audio device");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}
    }

  // load sound
  current_sound = snd;
  loop = (playmode == AVT_LOOP);
  SDL_PauseAudio (SDL_FALSE);

  return _avt_STATUS;
}

extern avt_char
avt_set_audio_end_key (avt_char key)
{
  avt_char old;

  old = audio_key;
  audio_key = key;

  return old;
}

extern int
avt_wait_audio_end (void)
{
  avt_char old_audio_key;

  if (not current_sound)
    return _avt_STATUS;

  old_audio_key = audio_key;
  audio_key = 0xE903;

  // end the loop, but wait for end of sound
  loop = false;

  while (current_sound and _avt_STATUS == AVT_NORMAL)
    avt_get_key ();		// end of audio also sends a pseudo key

  audio_key = old_audio_key;

  return _avt_STATUS;
}

extern void
avt_pause_audio (bool pause)
{
  SDL_PauseAudio ((int) pause);
}
