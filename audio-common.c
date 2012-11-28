/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for audio, independent of the backend
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99
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
#define _POSIX_C_SOURCE 200112L

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "avtdata.h"

#include <stdlib.h>		// malloc / realloc / free
#include <stdint.h>
#include <string.h>		// memcmp / memcpy
#include <iso646.h>

// absolute maximum size for audio data
#define MAXIMUM_SIZE  0xFFFFFFFFU

#ifdef NO_AUDIO

extern avt_audio *
avt_prepare_raw_audio (size_t capacity,
		       int samplingrate, int audio_type, int channels)
{
  return NULL;
}

extern int
avt_add_raw_audio_data (avt_audio * snd, void *data, size_t data_size)
{
  return AVT_FAILURE;
}

extern void
avt_finalize_raw_audio (avt_audio * snd)
{
}

extern void
avt_free_audio (avt_audio * snd)
{
}

extern void
avt_quit_audio (void)
{
}

#define avt_load_audio_general(a,b,c)  NULL

#else // not NO_AUDIO

#include "alert.c"

// short sound for the "avt_bell" function
static avt_audio *alert_sound;
static void (*quit_audio_backend) (void);

// table for decoding mu-law
static const int_least16_t mulaw_decode[256] = {
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

// table for decoding A-law
static const int_least16_t alaw_decode[256] = {
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

static void
audio_alert (void)
{
  // if alert_sound is loaded and nothing is currently playing
  if (alert_sound and not avt_audio_playing (NULL))
    avt_play_audio (alert_sound, AVT_PLAY);
}

extern void
avt_quit_audio (void)
{
  if (alert_sound)
    {
      avt_free_audio (alert_sound);
      alert_sound = NULL;
    }

  avt_bell_function (avt_flash);

  if (quit_audio_backend)
    {
      quit_audio_backend ();
      quit_audio_backend = NULL;
    }

  // no need to call it again automatically
  avt_quit_audio_function (NULL);
}

static size_t
avt_required_audio_size (avt_audio * snd, size_t data_size)
{
  size_t out_size;

  switch (snd->audio_type)
    {
    case AVT_AUDIO_MULAW:
    case AVT_AUDIO_ALAW:
      out_size = 2 * data_size;	// one byte becomes 2 bytes
      break;

    case AVT_AUDIO_S24SYS:
    case AVT_AUDIO_S24LE:
    case AVT_AUDIO_S24BE:
      out_size = (data_size * 2) / 3;	// reduced to 16 Bit
      break;

    case AVT_AUDIO_S32SYS:
    case AVT_AUDIO_S32LE:
    case AVT_AUDIO_S32BE:
      out_size = data_size / 2;	// reduced to 16 Bit
      break;

    default:
      out_size = data_size;
      break;
    }

  return out_size;
}

extern int
avt_add_raw_audio_data (avt_audio * snd, void *restrict data,
			size_t data_size)
{
  size_t old_size, new_size, out_size;
  bool active;

  if (_avt_STATUS != AVT_NORMAL or not snd or not data or not data_size)
    return avt_update ();

  // audio structure must have been created with avt_prepare_raw_audio
  if (snd->audio_type == AVT_AUDIO_UNKNOWN)
    {
      avt_set_error ("unknown audio format");
      return AVT_FAILURE;
    }

  out_size = avt_required_audio_size (snd, data_size);
  old_size = snd->length;
  new_size = old_size + out_size;

  // if it's currently playing, lock it
  active = avt_audio_playing (snd);
  if (active)
    avt_lock_audio ();

  // eventually get more memory for output buffer
  if (new_size > snd->capacity)
    {
      void *new_sound;
      size_t new_capacity;

      // get twice the capacity
      new_capacity = 2 * snd->capacity;

      /*
       * the capacity must never be lower than new_size
       * and it may still be 0
       */
      if (new_capacity < new_size)
	new_capacity = new_size;

      new_sound = realloc (snd->sound, new_capacity);

      if (not new_sound)
	{
	  avt_set_error ("out of memory");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      snd->sound = (unsigned char *) new_sound;
      snd->capacity = new_capacity;
    }

  // convert or copy the data
  switch (snd->audio_type)
    {
    case AVT_AUDIO_S16SYS:
    case AVT_AUDIO_U8:
    case AVT_AUDIO_S8:
      // linear PCM, same bit size and endianness
      memcpy (snd->sound + old_size, data, out_size);
      break;

    case AVT_AUDIO_MULAW:	// mu-law, logarithmic PCM
      {
	uint_least8_t *restrict in;
	int_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (int_least16_t *) (snd->sound + old_size);

	for (size_t i = data_size; i > 0; i--)
	  *out++ = mulaw_decode[*in++];
	break;
      }

    case AVT_AUDIO_ALAW:	// A-law, logarithmic PCM
      {
	uint_least8_t *restrict in;
	int_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (int_least16_t *) (snd->sound + old_size);

	for (size_t i = data_size; i > 0; i--)
	  *out++ = alaw_decode[*in++];
	break;
      }

    case AVT_AUDIO_S16LE:
      {
	uint_least8_t *restrict in;
	uint_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (uint_least16_t *) (snd->sound + old_size);

	for (size_t i = out_size / 2; i > 0; i--, in += 2)
	  *out++ = (in[1] << 8) | in[0];
      }
      break;

    case AVT_AUDIO_S16BE:
      {
	uint_least8_t *restrict in;
	uint_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (uint_least16_t *) (snd->sound + old_size);

	for (size_t i = out_size / 2; i > 0; i--, in += 2)
	  *out++ = (in[0] << 8) | in[1];
      }
      break;

      // the following ones are all converted to 16 bits

    case AVT_AUDIO_S24LE:
      {
	uint_least8_t *restrict in;
	uint_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (uint_least16_t *) (snd->sound + old_size);

	for (size_t i = out_size / sizeof (*out); i > 0; i--, in += 3)
	  *out++ = (in[2] << 8) | in[1];
      }
      break;

    case AVT_AUDIO_S24BE:
      {
	uint_least8_t *restrict in;
	uint_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (uint_least16_t *) (snd->sound + old_size);

	for (size_t i = out_size / sizeof (*out); i > 0; i--, in += 3)
	  *out++ = (in[0] << 8) | in[1];
      }
      break;

    case AVT_AUDIO_S32LE:
      {
	uint_least8_t *restrict in;
	uint_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (uint_least16_t *) (snd->sound + old_size);

	for (size_t i = out_size / sizeof (*out); i > 0; i--, in += 4)
	  *out++ = (in[3] << 8) | in[2];
      }
      break;

    case AVT_AUDIO_S32BE:
      {
	uint_least8_t *restrict in;
	uint_least16_t *restrict out;

	in = (uint_least8_t *) data;
	out = (uint_least16_t *) (snd->sound + old_size);

	for (size_t i = out_size / sizeof (*out); i > 0; i--, in += 4)
	  *out++ = (in[0] << 8) | in[1];
      }
      break;

    default:
      avt_set_error ("Internal error");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  snd->length = new_size;

  if (active)
    avt_unlock_audio (snd);

  return avt_update ();
}

extern avt_audio *
avt_prepare_raw_audio (size_t capacity,
		       int samplingrate, int audio_type, int channels)
{
  struct avt_audio *s;

  if (channels < 1 or channels > 2)
    {
      avt_set_error ("only 1 or 2 channels supported");
      return NULL;
    }

  // adjustments for later optimizations
  if (AVT_LITTLE_ENDIAN == AVT_BYTE_ORDER)
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
  else if (AVT_BIG_ENDIAN == AVT_BYTE_ORDER)
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

  // get memory for struct
  s = (struct avt_audio *) malloc (sizeof (struct avt_audio));
  if (not s)
    {
      avt_set_error ("out of memory");
      return NULL;
    }

  s->length = 0;
  s->audio_type = audio_type;
  s->samplingrate = samplingrate;
  s->channels = channels;
  s->complete = false;

  // reserve memory
  unsigned char *sound_data = NULL;
  size_t real_capacity = 0;

  if (capacity > 0 and capacity < MAXIMUM_SIZE)
    {
      real_capacity = avt_required_audio_size (s, capacity);
      sound_data = (unsigned char *) malloc (real_capacity);

      if (not sound_data)
	{
	  avt_set_error ("out of memory");
	  free (s);
	  return NULL;
	}
    }

  s->sound = sound_data;
  s->capacity = real_capacity;

  return s;
}

extern void
avt_finalize_raw_audio (avt_audio * snd)
{
  bool active;

  active = avt_audio_playing (snd);
  if (active)
    avt_lock_audio ();

  // eventually free unneeded memory
  if (snd->capacity > snd->length)
    {
      void *new_sound;
      size_t new_capacity;

      new_capacity = snd->length;
      new_sound = realloc (snd->sound, new_capacity);

      if (new_sound)
	{
	  snd->sound = (unsigned char *) new_sound;
	  snd->capacity = new_capacity;
	}
    }

  snd->complete = true;

  if (active)
    avt_unlock_audio (snd);
}

extern void
avt_free_audio (avt_audio * snd)
{
  if (snd)
    {
      // Is this sound currently playing? Then stop it!
      if (avt_audio_playing (snd))
	avt_stop_audio ();

      // free the sound data
      free (snd->sound);

      // free the rest
      free (snd);
    }
}


// if size is unknown use 0 or MAXIMUM_SIZE for maxsize
static avt_audio *
avt_load_audio_block (avt_data * src, uint_least32_t maxsize,
		      int samplingrate, int audio_type, int channels,
		      int playmode)
{
  avt_audio *audio;
  int n;
  uint_least32_t rest;
  uint_least8_t data[24 * 1024];

  audio = avt_prepare_raw_audio (maxsize, samplingrate, audio_type, channels);

  if (not audio)
    return NULL;

  if (maxsize)
    rest = maxsize;
  else
    rest = MAXIMUM_SIZE;

  while ((n = avt_data_read (src, &data, 1,
			     avt_min (sizeof (data), rest))) > 0)
    {
      if (avt_add_raw_audio_data (audio, data, n) != AVT_NORMAL)
	{
	  avt_free_audio (audio);
	  return NULL;
	}

      if (playmode != AVT_LOAD)
	{
	  avt_play_audio (audio, playmode);
	  playmode = AVT_LOAD;
	}

      rest -= n;
    }

  avt_finalize_raw_audio (audio);

  return audio;
}


static avt_audio *
avt_load_au (avt_data * src, uint_least32_t maxsize, int playmode)
{
  uint_least32_t head_size, audio_size, encoding, samplingrate, channels;
  int audio_type;

  if (not src)
    return NULL;

  // check magic ".snd"
  if (avt_data_read32be (src) != 0x2e736e64)
    {
      avt_set_error ("Data is not an AU audio file"
		     " (maybe old raw data format?)");
      return NULL;
    }

  head_size = avt_data_read32be (src);
  audio_size = avt_data_read32be (src);
  encoding = avt_data_read32be (src);
  samplingrate = avt_data_read32be (src);
  channels = avt_data_read32be (src);

  // skip the rest of the header
  if (head_size > 24)
    avt_data_seek (src, head_size - 24, SEEK_CUR);

  if (maxsize < 0xFFFFFFFFU)
    {
      maxsize -= head_size;

      if (maxsize < audio_size)
	audio_size = maxsize;
    }

  // Note: linear PCM is always assumed to be signed and big endian
  switch (encoding)
    {
    case 1:			// mu-law
      audio_type = AVT_AUDIO_MULAW;
      break;

    case 2:			// 8Bit linear PCM
      audio_type = AVT_AUDIO_S8;	// signed!
      break;

    case 3:			// 16Bit linear PCM
      audio_type = AVT_AUDIO_S16BE;
      break;

    case 4:			// 24Bit linear PCM
      audio_type = AVT_AUDIO_S24BE;
      break;

    case 5:			// 32Bit linear PCM
      audio_type = AVT_AUDIO_S32BE;
      break;

    case 27:			// A-law
      audio_type = AVT_AUDIO_ALAW;
      break;

    default:
      avt_set_error ("unsupported encoding in AU file");
      return NULL;
    }

  /*
   * other encodings:
   *
   * 6: 32Bit float
   * 7: 64Bit float
   * 10-13: 8/16/24/32Bit fixed point
   * 23-26: ADPCM variants
   */

  return avt_load_audio_block (src, audio_size, samplingrate, audio_type,
			       channels, playmode);
}


// The Wave format is so stupid - don't ever use it!
static avt_audio *
avt_load_wave (avt_data * src, uint_least32_t maxsize, int playmode)
{
  int audio_type;
  char identifier[4];
  bool wrong_chunk;
  uint_least32_t chunk_size, chunk_end;
  uint_least32_t samplingrate;
  uint_least16_t encoding, channels, bits_per_sample;

  if (not src)
    return NULL;

  if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1
      or memcmp ("RIFF", identifier, sizeof (identifier)) != 0)
    return NULL;		// not a RIFF file

  /*
   * this chunk contains the rest,
   * so chunk_size should be the file size - 8
   */
  chunk_size = avt_data_read32le (src);

  if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1
      or memcmp ("WAVE", identifier, sizeof (identifier)) != 0)
    return NULL;		// not a Wave file

  // search format chunk
  do
    {
      if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1)
	return NULL;		// no format chunk found
      chunk_size = avt_data_read32le (src);
      chunk_end = avt_data_tell (src) + chunk_size;
      chunk_end += (chunk_end % 2);	// padding to even addresses
      wrong_chunk = (memcmp ("fmt ", identifier, sizeof (identifier)) != 0);
      if (wrong_chunk)
	avt_data_seek (src, chunk_end, SEEK_SET);
    }
  while (wrong_chunk);

  encoding = avt_data_read16le (src);
  channels = avt_data_read16le (src);
  samplingrate = avt_data_read32le (src);
  avt_data_read32le (src);	// bytes_per_second
  avt_data_read16le (src);	// block_align
  bits_per_sample = avt_data_read16le (src);	// just for PCM
  avt_data_seek (src, chunk_end, SEEK_SET);

  switch (encoding)
    {
    case 1:			// PCM
      // smaller numbers are already right-padded
      if (bits_per_sample <= 8)
	audio_type = AVT_AUDIO_U8;	// unsigned
      else if (bits_per_sample <= 16)
	audio_type = AVT_AUDIO_S16LE;	// signed
      else if (bits_per_sample <= 24)
	audio_type = AVT_AUDIO_S24LE;	// signed
      else if (bits_per_sample <= 32)
	audio_type = AVT_AUDIO_S32LE;	// signed
      else
	return NULL;
      break;

    case 6:			// A-law
      audio_type = AVT_AUDIO_ALAW;
      break;

    case 7:			// mu-law
      audio_type = AVT_AUDIO_MULAW;
      break;

    default:			// unsupported encoding
      return NULL;
    }

  // search data chunk - must be after format chunk
  do
    {
      if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1)
	return NULL;		// no data chunk found
      chunk_size = avt_data_read32le (src);
      chunk_end = avt_data_tell (src) + chunk_size;
      chunk_end += (chunk_end % 2);	// padding to even addresses
      wrong_chunk = (memcmp ("data", identifier, sizeof (identifier)) != 0);
      if (wrong_chunk)
	avt_data_seek (src, chunk_end, SEEK_SET);
    }
  while (wrong_chunk);

  if (chunk_size < maxsize)
    maxsize = chunk_size;

  return avt_load_audio_block (src, maxsize, samplingrate,
			       audio_type, channels, playmode);
}

// src gets always closed
static avt_audio *
avt_load_audio_general (avt_data * src, uint_least32_t maxsize, int playmode)
{
  struct avt_audio *s;
  int start;
  char head[16];

  if (not src)
    return NULL;

  if (not maxsize)
    maxsize = MAXIMUM_SIZE;

  start = avt_data_tell (src);

  if (avt_data_read (src, head, sizeof (head), 1) != 1)
    {
      avt_set_error ("cannot read head of audio data");
      avt_data_close (src);
      return NULL;
    }

  avt_data_seek (src, start, SEEK_SET);

  if (memcmp (&head[0], ".snd", 4) == 0)
    s = avt_load_au (src, maxsize, playmode);
  else if (memcmp (&head[0], "RIFF", 4) == 0
	   and memcmp (&head[8], "WAVE", 4) == 0)
    s = avt_load_wave (src, maxsize, playmode);
  else
    {
      s = NULL;
      avt_set_error ("audio data neither in AU nor WAVE format");
    }

  avt_data_close (src);
  return s;
}

extern int
avt_start_audio_common (void (*quit_backend) (void))
{
  if (not alert_sound)
    alert_sound = avt_load_audio_data (&avt_alert_data,
				       avt_alert_data_size, AVT_LOAD);

  if (alert_sound)
    avt_bell_function (audio_alert);

  quit_audio_backend = quit_backend;
  avt_quit_audio_function (avt_quit_audio);

  return _avt_STATUS;
}

#endif // not NO_AUDIO


extern avt_audio *
avt_load_audio_file (const char *file, int playmode)
{
  return avt_load_audio_general (avt_data_open_file (file),
				 MAXIMUM_SIZE, playmode);
}

extern avt_audio *
avt_load_audio_part (avt_stream * stream, size_t maxsize, int playmode)
{
  return
    avt_load_audio_general (avt_data_open_stream ((FILE *) stream, false),
			    maxsize, playmode);
}

extern avt_audio *
avt_load_audio_stream (avt_stream * stream, int playmode)
{
  return
    avt_load_audio_general (avt_data_open_stream ((FILE *) stream, false),
			    MAXIMUM_SIZE, playmode);
}

extern avt_audio *
avt_load_audio_data (const void *data, size_t datasize, int playmode)
{
  return avt_load_audio_general (avt_data_open_memory (data, datasize),
				 datasize, playmode);
}
