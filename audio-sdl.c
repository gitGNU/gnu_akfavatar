/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for the audio-output
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
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
#include "SDL.h"
#include "SDL_audio.h"

#include <iso646.h>

// lower audio buffer size for lower latency, but it could become choppy
#define OUTPUT_BUFFER 1024

static bool avt_audio_initialized;

// current sound
static struct avt_audio current_sound;
static volatile int32_t soundpos = 0;	// Current play position
static volatile int32_t soundleft = 0;	// Length of left unplayed audio data
static volatile bool loop = false;
static volatile bool playing = false;
static avt_char audio_key;

static void avt_quit_audio_sdl (void);

// this is the callback function
static void
fill_audio (void *userdata, uint8_t * stream, int len)
{
  (void) userdata;

  // only play, when there is data left
  if (soundleft <= 0)
    {
      if (not current_sound.complete)
	return;
      else if (loop)
	{
	  // rewind to beginning
	  soundpos = 0;
	  soundleft = current_sound.length;
	}
      else			// finished
	{
	  SDL_PauseAudio (1);	// shut up
	  playing = false;

	  if (audio_key)
	    avt_push_key (audio_key);
	  return;
	}
    }

  // Copy as much data as possible
  if (len <= soundleft)
    {
      SDL_memcpy (stream, current_sound.sound + soundpos, len);

      soundpos += len;
      soundleft -= len;
    }
  else				// len > soundleft
    {
      SDL_memcpy (stream, current_sound.sound + soundpos, soundleft);

      // silence rest
      SDL_memset (stream + soundleft,
		  (AVT_AUDIO_U8 != current_sound.audio_type) ? 0 : 128,
		  len - soundleft);


      soundleft = 0;
    }
}

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
  SDL_CloseAudio ();
  playing = false;
  soundpos = 0;
  soundleft = 0;
  loop = false;
  audio_key = 0;
  current_sound.length = 0;
  current_sound.sound = NULL;
}

static void
avt_quit_audio_sdl (void)
{
  if (avt_audio_initialized)
    {
      SDL_CloseAudio ();
      soundpos = 0;
      soundleft = 0;
      current_sound.length = 0;
      current_sound.sound = NULL;
      loop = false;
      playing = false;
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
  size_t old_length;

  old_length = current_sound.length;
  current_sound = *snd;
  soundleft += current_sound.length - old_length;

  SDL_UnlockAudio ();
}

// Is this sound currently playing? NULL for any sound
extern bool
avt_audio_playing (avt_audio * snd)
{
  if (snd and snd->sound != current_sound.sound)
    return false;		// not same sound

  return playing;
}

extern int
avt_play_audio (avt_audio * snd, int playmode)
{
  SDL_AudioSpec audiospec;

  if (not avt_audio_initialized)
    return _avt_STATUS;

  // no sound? - just ignore it
  if (not snd)
    return _avt_STATUS;

  if (playmode != AVT_PLAY and playmode != AVT_LOOP)
    return AVT_FAILURE;

  // close audio, in case it is left open
  SDL_CloseAudio ();
  SDL_LockAudio ();

  SDL_memset (&audiospec, 0, sizeof (audiospec));

  // load sound
  current_sound = *snd;

  switch (snd->audio_type)
    {
    case AVT_AUDIO_U8:
      audiospec.format = AUDIO_U8;
      break;

    case AVT_AUDIO_S8:
      audiospec.format = AUDIO_S8;
      break;

    default:			// all other get converted
      audiospec.format = AUDIO_S16SYS;
      break;
    }

  audiospec.freq = snd->samplingrate;
  audiospec.channels = snd->channels;
  audiospec.callback = fill_audio;
  audiospec.samples = OUTPUT_BUFFER;
  audiospec.userdata = &current_sound;	// not really used

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
      avt_set_error ("error opening audio device");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }
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

  if (not playing)
    return _avt_STATUS;

  old_audio_key = audio_key;
  audio_key = 0xE903;

  // end the loop, but wait for end of sound
  loop = false;

  while (playing and _avt_STATUS == AVT_NORMAL)
    avt_get_key ();		// end of audio also sends a pseudo key

  audio_key = old_audio_key;

  return _avt_STATUS;
}

extern void
avt_pause_audio (bool pause)
{
  SDL_PauseAudio ((int) pause);
}
