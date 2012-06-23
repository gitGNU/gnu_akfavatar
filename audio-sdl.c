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

static bool avt_audio_initialized;

/* current sound */
static struct avt_audio current_sound;
static SDL_AudioSpec audiospec;
static volatile int32_t soundpos = 0;	/* Current play position */
static volatile int32_t soundleft = 0;	/* Length of left unplayed wave data */
static bool loop = false;
static volatile bool playing = false;

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
	  soundleft = current_sound.length;
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
      avt_start_audio_common ();
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
  current_sound.length = 0;
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
      current_sound.length = 0;
      current_sound.sound = NULL;
      loop = false;
      playing = false;
      SDL_QuitSubSystem (SDL_INIT_AUDIO);
      avt_quit_audio_common ();
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
  size_t old_length;

  old_length = current_sound.length;
  current_sound = *snd;
  soundleft += current_sound.length - old_length;

  SDL_UnlockAudio ();
}

/* Is this sound currently playing? NULL for any sound */
extern bool
avt_audio_playing (avt_audio * snd)
{
  if (snd && snd->sound != current_sound.sound)
    return false;		/* not same sound */

  return playing;
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
  current_sound = *snd;

  switch (snd->audio_type)
    {
    case AVT_AUDIO_U8:
      audiospec.format = AUDIO_U8;
      break;

    case AVT_AUDIO_S8:
      audiospec.format = AUDIO_S8;
      break;

    default:			/* all other get converted */
      audiospec.format = AUDIO_S16SYS;
      break;
    }

  audiospec.freq = snd->samplingrate;
  audiospec.channels = snd->channels;
  audiospec.callback = fill_audio;
  audiospec.samples = OUTPUT_BUFFER;
  audiospec.userdata = &current_sound;

  loop = (playmode == AVT_LOOP);

  if (SDL_OpenAudio (&audiospec, NULL) == 0)
    {
      soundpos = 0;
      soundleft = current_sound.length;
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
