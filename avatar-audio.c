/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for the audio-output
 * Copyright (c) 2007, 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
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
#  undef SDL_memcpy
#  define SDL_memcpy              memcpy
#  undef SDL_memcmp
#  define SDL_memcmp              memcmp
#  undef SDL_free
#  define SDL_free                free
#endif /* OLD_SDL */

#pragma GCC poison  malloc free strlen memcpy memcmp getenv putenv

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

/* table for decoding mu-law */
static const Sint16 mulaw_decode[256] = {
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
static const Sint16 alaw_decode[256] = {
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
void
fill_audio (void *userdata AVT_UNUSED, Uint8 * stream, int len)
{
  /* only play, when there is data left */
  if (soundleft <= 0)
    {
      if (loop)
	{
	  /* rewind to beginning */
	  soundpos = current_sound.sound;
	  soundleft = current_sound.len;
	}
      else
	{
	  SDL_PauseAudio (1);	/* be quiet */
	  return;
	}
    }

  /* Copy as much data as possible */
  if (len > soundleft)
    len = soundleft;

  SDL_memcpy (stream, soundpos, len);

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
  if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
    {
      SDL_SetError ("error initializing audio");
      return AVT_ERROR;
    }

  mybell =
    avt_load_raw_audio_data ((void *) &avt_bell_data, avt_bell_data_size,
			     8000, AVT_AUDIO_MULAW, AVT_AUDIO_MONO);
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

static Uint8 *
avt_get_RWdata (SDL_RWops * src, Sint32 size)
{
  Uint8 *buf;

  buf = (Uint8 *) SDL_malloc (size);
  if (buf == NULL)
    SDL_SetError ("out of memory");
  else
    {
      /* read the data into the buf */
      if (SDL_RWread (src, buf, 1, size) < size)
	{
	  SDL_SetError ("read error");
	  SDL_free (buf);
	  buf = NULL;
	}
    }

  return buf;
}

static SDL_AudioSpec *
avt_LoadAU_RW (SDL_RWops * src, int freesrc,
	       SDL_AudioSpec * spec, Uint8 ** audio_buf, Uint32 * data_size)
{
  Uint32 head_size, audio_size, encoding, samplingrate, channels;
  Uint8 *buf;
  avt_bool_t completed;

  completed = AVT_FALSE;
  buf = NULL;

  if (!src || !spec || !audio_buf || !data_size)
    goto done;

  *audio_buf = NULL;
  *data_size = 0;

  /* check magic ".snd" */
  if (SDL_ReadBE32 (src) != 0x2e736e64)
    {
      SDL_SetError ("Data is not an AU audio file"
		    " (maybe old raw data format?)");
      goto done;
    }

  head_size = SDL_ReadBE32 (src);
  audio_size = SDL_ReadBE32 (src);
  encoding = SDL_ReadBE32 (src);
  samplingrate = SDL_ReadBE32 (src);
  channels = SDL_ReadBE32 (src);

  if (channels < 1 || channels > 2)
    {
      SDL_SetError ("only 1 or 2 channels supported");
      goto done;
    }

  /* skip the rest of the header */
  if (head_size > 24)
    SDL_RWseek (src, head_size - 24, RW_SEEK_CUR);

  /*
   * Note: A header size of 24 is not standard conformant,
   * but many programs create such files anyway
   */

  /* size of audio-data not given :-( */
  if (audio_size == 0xffffffff)
    {
      Uint32 data_start;

      data_start = SDL_RWtell (src);
      audio_size = SDL_RWseek (src, 0, RW_SEEK_END) - data_start;
      SDL_RWseek (src, data_start, RW_SEEK_SET);
    }

  /* Note: linear PCM is always assumed to be signed and big endian */
  switch (encoding)
    {
    case 1:			/* mu-law */
    case 27:			/* A-law */
      {
	Uint32 i;
	Uint32 out_size;
	Sint16 *outbuf, *outp;
	Uint8 value;

	/* get larger output buffer */
	out_size = sizeof (Sint16) * audio_size;
	outbuf = (Sint16 *) SDL_malloc (out_size);
	if (outbuf == NULL)
	  {
	    SDL_SetError ("out of memory");
	    goto done;
	  }

	/* decode */
	outp = outbuf;
	if (encoding == 1)	/* mu-law */
	  {
	    for (i = 0; i < audio_size; i++, outp++)
	      {
		if (SDL_RWread (src, &value, sizeof (value), 1) == -1)
		  break;
		*outp = mulaw_decode[value];
	      }
	  }
	else			/* A-law */
	  {
	    for (i = 0; i < audio_size; i++, outp++)
	      {
		if (SDL_RWread (src, &value, sizeof (value), 1) == -1)
		  break;
		*outp = alaw_decode[value];
	      }
	  }

	/* assign values */
	spec->format = AUDIO_S16SYS;
	*audio_buf = (Uint8 *) outbuf;
	*data_size = out_size;
	completed = AVT_TRUE;
      }
      break;

    case 2:			/* 8Bit linear PCM */
      buf = avt_get_RWdata (src, audio_size);
      if (buf)
	{
	  spec->format = AUDIO_S8;	/* signed! */
	  *audio_buf = buf;
	  *data_size = audio_size;
	  completed = AVT_TRUE;
	}
      break;

    case 3:			/* 16Bit linear PCM */
      buf = avt_get_RWdata (src, audio_size);
      if (buf)
	{
	  /* convert to system endianess now, if necessary */
	  if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
	    {
	      Uint32 i;
	      Sint16 *bufp;

	      bufp = (Sint16 *) buf;
	      for (i = 0; i < (audio_size / 2); i++, bufp++)
		*bufp = SDL_Swap16 (*bufp);
	    }

	  spec->format = AUDIO_S16SYS;
	  *audio_buf = buf;
	  *data_size = audio_size;
	  completed = AVT_TRUE;
	}
      break;

    case 4:			/* 24Bit linear PCM */
    case 5:			/* 32Bit linear PCM */
      /* degrade to 16Bit */
      {
	Uint32 i;
	Uint8 BPS;		/* Bytes per Sample */
	Uint32 out_size;
	Uint32 samples;
	Sint16 *outbuf, *outp;
	Sint16 value, dummy;

	/* Bytes per Sample and skip value */
	if (encoding == 4)
	  BPS = 24 / 8;
	else
	  BPS = 32 / 8;

	samples = audio_size / BPS;
	out_size = samples * sizeof (Sint16);
	outbuf = (Sint16 *) SDL_malloc (out_size);

	if (outbuf == NULL)
	  {
	    SDL_SetError ("out of memory");
	    goto done;
	  }

	outp = outbuf;
	for (i = 0; i < samples; i++, outp++)
	  {
	    if (SDL_RWread (src, &value, sizeof (value), 1) == -1)
	      break;
	    *outp = SDL_SwapBE16 (value);

	    /* skip the rest */
	    if (SDL_RWread (src, &dummy, BPS - sizeof (value), 1) == -1)
	      break;
	  }

	/* assign values */
	spec->format = AUDIO_S16SYS;
	*audio_buf = (Uint8 *) outbuf;
	*data_size = out_size;
	completed = AVT_TRUE;
      }
      break;

    default:
      SDL_SetError ("unsupported encoding in AU file");
      goto done;
    }

  /* settings, which don't depend on the format */
  if (completed)
    {
      spec->freq = samplingrate;
      spec->channels = channels;
      spec->samples = 1024;	/* internal buffer */
      spec->callback = fill_audio;
      spec->userdata = NULL;
    }

done:
  if (src && freesrc)
    SDL_RWclose (src);

  if (completed)
    return spec;
  else
    return NULL;
}

/* use avt_load_audio_file and avt_load_audio_data instead */
#if 0 
static avt_audio_t *
avt_load_au_file (const char *file)
{
  AudioStruct *s;

  s = (AudioStruct *) SDL_malloc (sizeof (AudioStruct));
  if (s == NULL)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  s->wave = AVT_FALSE;
  if (avt_LoadAU_RW (SDL_RWFromFile (file, "rb"), 1,
		     &s->audiospec, &s->sound, &s->len) == NULL)
    {
      SDL_free (s);
      return NULL;
    }

  return (avt_audio_t *) s;
}

static avt_audio_t *
avt_load_au_data (void *data, int datasize)
{
  AudioStruct *s;

  s = (AudioStruct *) SDL_malloc (sizeof (AudioStruct));
  if (s == NULL)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  s->wave = AVT_FALSE;
  if (avt_LoadAU_RW (SDL_RWFromMem (data, datasize), 1,
		     &s->audiospec, &s->sound, &s->len) == NULL)
    {
      SDL_free (s);
      return NULL;
    }

  return (avt_audio_t *) s;
}
#endif

static avt_audio_t *
avt_load_audio_RW (SDL_RWops * src)
{
  int start, type;
  AudioStruct *s;
  SDL_AudioSpec *r;
  char head[16];

  if (src == NULL)
    return NULL;

  r = NULL;
  type = 0;
  start = SDL_RWtell (src);
  if (SDL_RWread (src, head, sizeof (head), 1))
    {
      if (SDL_memcmp (&head[0], ".snd", 4) == 0)
	type = 1;
      else
	if (SDL_memcmp (&head[0], "RIFF", 4) == 0
	    && SDL_memcmp (&head[8], "WAVE", 4) == 0)
	type = 2;
      else
	{
	  SDL_SetError ("file neither in AU nor WAVE format");
	  SDL_RWclose (src);
	  return NULL;
	}
    }

  SDL_RWseek (src, start, RW_SEEK_SET);

  s = (AudioStruct *) SDL_malloc (sizeof (AudioStruct));
  if (s == NULL)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  switch (type)
    {
    case 1:			/* AU */
      s->wave = AVT_FALSE;
      r = avt_LoadAU_RW (src, 1, &s->audiospec, &s->sound, &s->len);
      break;

    case 2:			/* Wave */
      s->wave = AVT_TRUE;
      r = SDL_LoadWAV_RW (src, 1, &s->audiospec, &s->sound, &s->len);
      break;

    default:
      SDL_SetError ("internal error");
      return NULL;
    }

  if (r == NULL)
    {
      SDL_free (s);
      return NULL;
    }

  return (avt_audio_t *) s;
}

avt_audio_t *
avt_load_audio_file (const char *file)
{
  return avt_load_audio_RW (SDL_RWFromFile (file, "rb"));
}

avt_audio_t *
avt_load_audio_stream (void *stream)
{
  return avt_load_audio_RW (SDL_RWFromFP ((FILE*) stream, 0));
}

avt_audio_t *
avt_load_audio_data (void *data, int datasize)
{
  return avt_load_audio_RW (SDL_RWFromMem (data, datasize));
}

avt_audio_t *
avt_load_raw_audio_data (void *data, int data_size,
			 int samplingrate, int audio_type, int channels)
{
  int format;
  int out_size;
  int i;
  AudioStruct *s;

  /* do we actually have data to process? */
  if (data == NULL || data_size <= 0)
    return NULL;

  if (channels < 1 || channels > 2)
    {
      SDL_SetError ("only 1 or 2 channels supported");
      return NULL;
    }

  /* first assume the data is copied unchanged */
  out_size = data_size;

  /* convert audio_type into SDL format number */
  switch (audio_type)
    {
    case AVT_AUDIO_U8:
      format = AUDIO_U8;
      break;
    case AVT_AUDIO_S8:
      format = AUDIO_S8;
      break;
    case AVT_AUDIO_U16LE:
    case AVT_AUDIO_U16BE:
    case AVT_AUDIO_U16SYS:
      /* endianess will get adjusted while loading */
      format = AUDIO_U16SYS;
      break;
    case AVT_AUDIO_S16LE:
    case AVT_AUDIO_S16BE:
    case AVT_AUDIO_S16SYS:
      /* endianess will get adjusted while loading */
      format = AUDIO_S16SYS;
      break;
    case AVT_AUDIO_MULAW:
    case AVT_AUDIO_ALAW:
      /* will be converted to S16SYS */
      format = AUDIO_S16SYS;
      /* need twice as much memory then */
      out_size = 2 * data_size;
      break;

    default:
      SDL_SetError ("unsupported audio type");
      return NULL;
    }

  /* get memory for struct */
  s = (AudioStruct *) SDL_malloc (sizeof (AudioStruct));
  if (s == NULL)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  /* get memory for output buffer */
  s->sound = (Uint8 *) SDL_malloc (out_size);
  if (s->sound == NULL)
    {
      SDL_free (s);
      SDL_SetError ("out of memory");
      return NULL;
    }

  s->len = out_size;
  s->wave = AVT_FALSE;
  s->audiospec.format = format;
  s->audiospec.freq = samplingrate;
  s->audiospec.channels = channels;
  s->audiospec.samples = 1024;	/* internal buffer */
  s->audiospec.callback = fill_audio;
  s->audiospec.userdata = NULL;

  /* convert or copy the data */
  switch (audio_type)
    {
    case AVT_AUDIO_MULAW:
      {
	Uint8 *in;
	Sint16 *out;

	in = (Uint8 *) data;
	out = (Sint16 *) s->sound;
	for (i = 0; i < data_size; i++)
	  *out++ = mulaw_decode[*in++];
	break;
      }

    case AVT_AUDIO_ALAW:
      {
	Uint8 *in;
	Sint16 *out;

	in = (Uint8 *) data;
	out = (Sint16 *) s->sound;
	for (i = 0; i < data_size; i++)
	  *out++ = alaw_decode[*in++];
	break;
      }

    case AVT_AUDIO_U16BE:
    case AVT_AUDIO_S16BE:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	SDL_memcpy (s->sound, data, out_size);
      else			/* swap bytes */
	{
	  Sint16 *in, *out;

	  in = (Sint16 *) data;
	  out = (Sint16 *) s->sound;
	  for (i = 0; i < (out_size / 2); i++, in++, out++)
	    *out = SDL_Swap16 (*in);
	}
      break;

    case AVT_AUDIO_U16LE:
    case AVT_AUDIO_S16LE:
      if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
	SDL_memcpy (s->sound, data, out_size);
      else			/* swap bytes */
	{
	  Sint16 *in, *out;

	  in = (Sint16 *) data;
	  out = (Sint16 *) s->sound;
	  for (i = 0; i < (out_size / 2); i++, in++, out++)
	    *out = SDL_Swap16 (*in);
	}
      break;

    default:			/* linear PCM */
      /* simply copy the audio data */
      SDL_memcpy (s->sound, data, out_size);
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

  if (soundleft > 0 && !avt_checkevent ())
    {
      /* wait while sound is still playing, and there is no event */
      while ((soundleft > 0) && !avt_checkevent ())
	SDL_Delay (10);		/* give some time to other processes */

      /* wait for last buffer + somewhat extra to be sure */
      SDL_Delay (((current_sound.audiospec.samples * 1000)
		  / current_sound.audiospec.freq) + 100);
    }

  return _avt_STATUS;
}
