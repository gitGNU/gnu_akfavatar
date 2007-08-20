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

/* $Id: avatar-audio.c,v 2.1 2007-08-20 17:55:15 akf Exp $ */

#include "avatar-audio.h"
#include "avatar.h"
#include "SDL.h"
#include "SDL_audio.h"

extern int _avt_STATUS;

static SDL_AudioSpec audiospec;
static Uint8 *sound = NULL;	/* Pointer to wave data */
static Uint8 *soundpos = NULL;	/* Current play position */
static Uint32 soundlen = 0;	/* Length of wave data */
static Uint32 soundleft = 0;	/* Length of left unplayed wave data */

extern int avt_checkevent (void);

void
fill_audio (void *unused, Uint8 * stream, int len)
{
  /* only play, when there is data left */
  if (soundleft <= 0)
    return;

  /* Mix as much data as possible */
  len = (len > soundleft ? soundleft : len);
  SDL_MixAudio (stream, soundpos, len, SDL_MIX_MAXVOLUME);
  soundpos += len;
  soundleft -= len;
}

/* must be called AFTER avt_initialize! */
int
avt_initialize_audio (void)
{
  if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
    return AVATARERROR;

  return _avt_STATUS;
}

/* stops audio, but leaves the file loaded */
void
avt_stop_audio (void)
{
  SDL_CloseAudio ();
}

/* should be called BEFORE avt_quit */
void
avt_quit_audio (void)
{
  avt_free_wave ();
  SDL_QuitSubSystem (SDL_INIT_AUDIO);
}

int
avt_load_wave_file (const char *file)
{
  if (sound)
    avt_free_wave ();

  SDL_LockAudio ();

  if (SDL_LoadWAV (file, &audiospec, &sound, &soundlen) == NULL)
    {
      sound = NULL;
      return AVATARERROR;
    }

  soundpos = sound;
  soundleft = soundlen;
  audiospec.callback = fill_audio;
  SDL_UnlockAudio ();

  return _avt_STATUS;
}

int
avt_load_wave_data (void *data, int datasize)
{
  if (sound)
    avt_free_wave ();

  SDL_LockAudio ();

  if (SDL_LoadWAV_RW (SDL_RWFromMem (data, datasize), 1,
		      &audiospec, &sound, &soundlen) == NULL)
    {
      sound = NULL;
      return AVATARERROR;
    }

  soundpos = sound;
  soundleft = soundlen;
  audiospec.callback = fill_audio;
  SDL_UnlockAudio ();

  return _avt_STATUS;
}

void
avt_free_wave (void)
{
  SDL_CloseAudio ();
  soundpos = NULL;
  soundlen = 0;
  soundleft = 0;
  SDL_FreeWAV (sound);
  sound = NULL;
}

int
avt_play_audio (void)
{
  /* no sound? - just ignore it */
  if (!sound)
    return _avt_STATUS;

  /* close audio, in case it is left open */
  SDL_CloseAudio ();
  SDL_LockAudio ();

  /* rewind to beginning of sound */
  soundpos = sound;
  soundleft = soundlen;

  if (SDL_OpenAudio (&audiospec, NULL) < 0)
    {
      avt_free_wave ();
      return AVATARERROR;
    }

  SDL_UnlockAudio ();
  SDL_PauseAudio (0);

  return _avt_STATUS;
}

int
avt_wait_audio_end (void)
{
  if (soundleft > 0)
    {
      /* loop while sound is still playing, and there is no event */
      while ((soundleft > 0) && !avt_checkevent ())
	SDL_Delay (1);	/* give some time to other processes */
      /* wait for last buffer + somewhat extra to be sure */
      SDL_Delay (((audiospec.samples * 1000) / audiospec.freq) + 100);
      avt_checkevent ();
    }

  return _avt_STATUS;
}
