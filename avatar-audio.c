/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for the audio-output
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * needed: 
 *  SDL1.2
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

/* $Id: avatar-audio.c,v 2.21 2009-01-04 23:04:27 akf Exp $ */

#include "akfavatar.h"
#include "SDL.h"
#include "SDL_audio.h"

#include "bell.c"

#ifndef _SDL_stdinc_h
#  define OLD_SDL 1
#endif

#ifdef OLD_SDL
#  include <stdlib.h>
#  undef SDL_malloc
#  define SDL_malloc              malloc
#  undef SDL_free
#  define SDL_free                free
#endif /* OLD_SDL */

#pragma GCC poison  malloc free strlen memcpy getenv putenv

typedef struct
{
  SDL_AudioSpec audiospec;
  Uint8 *sound;			/* Pointer to sound data */
  Uint32 len;			/* Length of wave data */
  Uint8 wave;			/* wether SDL_FreeWav is needed? */
} AudioStruct;

extern int _avt_STATUS;

/* short sound for the "bell" function */
static avt_audio_t *mybell;

/* current sound */
static AudioStruct current_sound;
static Uint8 *soundpos = NULL;	/* Current play position */
static Sint32 soundleft = 0;	/* Length of left unplayed wave data */
static avt_bool_t loop = AVT_FALSE;

extern int avt_checkevent (void);
extern void (*avt_bell_func) (void);
extern void (*avt_quit_audio_func) (void);

/* this is the callback function */
void
fill_audio (void *userdata AVT_UNUSED, Uint8 * stream, int len)
{
  /* only play, when there is data left */
  if (soundleft <= 0)
    {
      if (!loop)
	return;
      else			/* rewind to beginning */
	{
	  soundpos = current_sound.sound;
	  soundleft = current_sound.len;
	}
    }

  /* Mix as much data as possible */
  len = (len > soundleft ? soundleft : len);
  SDL_MixAudio (stream, soundpos, len, SDL_MIX_MAXVOLUME);
  soundpos += len;
  soundleft -= len;
}

static void
short_audio_sound (void)
{
  /* if mybell is loaded and nothing is currently playing */
  if (mybell && soundleft <= 0)
    avt_play_audio (mybell, AVT_FALSE);
}

/* must be called AFTER avt_initialize! */
int
avt_initialize_audio (void)
{
  SDL_SetError ("$Id: avatar-audio.c,v 2.21 2009-01-04 23:04:27 akf Exp $");
  SDL_ClearError ();

  if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
    {
      SDL_SetError ("error initializing audio");
      return AVT_ERROR;
    }

  mybell = avt_load_wave_data ((void *) &avt_bell_data, avt_bell_data_size);
  avt_bell_func = short_audio_sound;
  avt_quit_audio_func = avt_quit_audio;

  return _avt_STATUS;
}

/* stops audio */
void
avt_stop_audio (void)
{
  SDL_CloseAudio ();
  soundpos = NULL;
  soundleft = 0;
  loop = AVT_FALSE;
  current_sound.len = 0;
  current_sound.sound = NULL;
}

void
avt_quit_audio (void)
{
  avt_quit_audio_func = NULL;
  SDL_CloseAudio ();
  soundpos = NULL;
  soundleft = 0;
  current_sound.len = 0;
  current_sound.sound = NULL;
  avt_bell_func = avt_flash;
  avt_free_audio (mybell);
  mybell = NULL;
  SDL_QuitSubSystem (SDL_INIT_AUDIO);
}

avt_audio_t *
avt_load_wave_file (const char *file)
{
  AudioStruct *s;

  s = (AudioStruct *) SDL_malloc (sizeof (AudioStruct));
  if (s == NULL)
    return NULL;

  s->wave = AVT_TRUE;
  if (SDL_LoadWAV (file, &s->audiospec, &s->sound, &s->len) == NULL)
    {
      SDL_free (s);
      return NULL;
    }

  return (avt_audio_t *) s;
}

avt_audio_t *
avt_load_wave_data (void *data, int datasize)
{
  AudioStruct *s;

  s = (AudioStruct *) SDL_malloc (sizeof (AudioStruct));
  if (s == NULL)
    return NULL;

  s->wave = AVT_TRUE;
  if (SDL_LoadWAV_RW (SDL_RWFromMem (data, datasize), 1,
		      &s->audiospec, &s->sound, &s->len) == NULL)
    {
      SDL_free (s);
      return NULL;
    }

  return (avt_audio_t *) s;
}

void
avt_free_audio (avt_audio_t * snd)
{
  AudioStruct *s;

  if (snd)
    {
      s = (AudioStruct *) snd;

      /* free the sound data */
      if (s->wave)
	SDL_FreeWAV (s->sound);
      else
	SDL_free (s->sound);

      /* free the rest */
      SDL_free (s);
    }
}

int
avt_play_audio (avt_audio_t * snd, avt_bool_t doloop)
{
  AudioStruct *newsound;

  /* no sound? - just ignore it */
  if (!snd)
    return _avt_STATUS;

  newsound = (AudioStruct *) snd;

  /* close audio, in case it is left open */
  SDL_CloseAudio ();
  SDL_LockAudio ();

  /* load sound */
  current_sound.sound = newsound->sound;
  current_sound.len = newsound->len;
  current_sound.audiospec = newsound->audiospec;
  current_sound.audiospec.callback = fill_audio;

  /* lower audio buffer size for lower latency */
  current_sound.audiospec.samples = 1024;

  loop = (doloop != 0);

  if (SDL_OpenAudio (&current_sound.audiospec, NULL) == 0)
    {
      soundpos = current_sound.sound;
      soundleft = current_sound.len;
      SDL_UnlockAudio ();
      SDL_PauseAudio (0);
      return _avt_STATUS;
    }
  else
    return AVT_ERROR;
}

int
avt_wait_audio_end (void)
{
  /* end the loop, but wait for end of sound */
  loop = AVT_FALSE;

  if (soundleft > 0)
    {
      /* wait while sound is still playing, and there is no event */
      while ((soundleft > 0) && !avt_checkevent ())
	SDL_Delay (1);		/* give some time to other processes */

      /* wait for last buffer + somewhat extra to be sure */
      SDL_Delay (((current_sound.audiospec.samples * 1000)
		  / current_sound.audiospec.freq) + 100);
    }

  avt_checkevent ();
  return _avt_STATUS;
}
