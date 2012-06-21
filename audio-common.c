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

/* don't make functions deprecated for this file */
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "avtdata.h"
#include <stdlib.h>		/* malloc / realloc /free */
#include <stdint.h>
#include <string.h>		/* memcmp */

/* absolute maximum size for audio data */
#define MAXIMUM_SIZE  0xFFFFFFFFU


#ifdef NO_AUDIO

#define avt_load_audio_general(a,b,c)  NULL

#else /* not NO_AUDIO */

/* if size is unknown use 0 or MAXIMUM_SIZE for maxsize */
static avt_audio *
avt_load_audio_block (avt_data * src, uint32_t maxsize,
		      int samplingrate, int audio_type, int channels,
		      int playmode)
{
  avt_audio *audio;
  int n;
  uint32_t rest;
  uint8_t data[24 * 1024];

  audio = avt_load_raw_audio_data (NULL, 0, samplingrate,
				   audio_type, channels);

  if (!audio)
    return NULL;

  if (maxsize != 0)
    rest = maxsize;
  else
    rest = MAXIMUM_SIZE;

  /* if size is known, pre-allocate enough memory */
  if (rest < MAXIMUM_SIZE)
    {
      if (avt_set_raw_audio_capacity (audio, rest) != AVT_NORMAL)
	return NULL;
    }

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
avt_load_au (avt_data * src, uint32_t maxsize, int playmode)
{
  uint32_t head_size, audio_size, encoding, samplingrate, channels;
  int audio_type;

  if (!src)
    return NULL;

  /* check magic ".snd" */
  if (avt_read32be (src) != 0x2e736e64)
    {
      avt_set_error ("Data is not an AU audio file"
		     " (maybe old raw data format?)");
      return NULL;
    }

  head_size = avt_read32be (src);
  audio_size = avt_read32be (src);
  encoding = avt_read32be (src);
  samplingrate = avt_read32be (src);
  channels = avt_read32be (src);

  /* skip the rest of the header */
  if (head_size > 24)
    avt_data_seek (src, head_size - 24, SEEK_CUR);

  if (maxsize != MAXIMUM_SIZE)
    {
      maxsize -= head_size;

      if (maxsize < audio_size)
	audio_size = maxsize;
    }

  /* Note: linear PCM is always assumed to be signed and big endian */
  switch (encoding)
    {
    case 1:			/* mu-law */
      audio_type = AVT_AUDIO_MULAW;
      break;

    case 2:			/* 8Bit linear PCM */
      audio_type = AVT_AUDIO_S8;	/* signed! */
      break;

    case 3:			/* 16Bit linear PCM */
      audio_type = AVT_AUDIO_S16BE;
      break;

    case 4:			/* 24Bit linear PCM */
      audio_type = AVT_AUDIO_S24BE;
      break;

    case 5:			/* 32Bit linear PCM */
      audio_type = AVT_AUDIO_S32BE;
      break;

    case 27:			/* A-law */
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


/* The Wave format is so stupid - don't ever use it! */
static avt_audio *
avt_load_wave (avt_data * src, uint32_t maxsize, int playmode)
{
  int start;
  int audio_type;
  char identifier[4];
  bool wrong_chunk;
  uint32_t chunk_size, chunk_end;
  uint32_t samplingrate;
  uint16_t encoding, channels, bits_per_sample;

  if (!src)
    return NULL;

  start = avt_data_tell (src);

  if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1
      || memcmp ("RIFF", identifier, sizeof (identifier)) != 0)
    return NULL;		/* not a RIFF file */

  /*
   * this chunk contains the rest,
   * so chunk_size should be the file size - 8
   */
  chunk_size = avt_read32le (src);

  if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1
      || memcmp ("WAVE", identifier, sizeof (identifier)) != 0)
    return NULL;		/* not a Wave file */

  /* search format chunk */
  do
    {
      if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1)
	return NULL;		/* no format chunk found */
      chunk_size = avt_read32le (src);
      chunk_end = avt_data_tell (src) + chunk_size;
      if (chunk_end % 2 != 0)
	chunk_end++;		/* padding to even addresses */
      wrong_chunk = (memcmp ("fmt ", identifier, sizeof (identifier)) != 0);
      if (wrong_chunk)
	avt_data_seek (src, chunk_end, SEEK_SET);
    }
  while (wrong_chunk);

  encoding = avt_read16le (src);
  channels = avt_read16le (src);
  samplingrate = avt_read32le (src);
  avt_read32le (src);		/* bytes_per_second */
  avt_read16le (src);		/* block_align */
  bits_per_sample = avt_read16le (src);	/* just for PCM */
  avt_data_seek (src, chunk_end, SEEK_SET);

  switch (encoding)
    {
    case 1:			/* PCM */
      /* smaller numbers are already right-padded */
      if (bits_per_sample <= 8)
	audio_type = AVT_AUDIO_U8;	/* unsigned */
      else if (bits_per_sample <= 16)
	audio_type = AVT_AUDIO_S16LE;	/* signed */
      else if (bits_per_sample <= 24)
	audio_type = AVT_AUDIO_S24LE;	/* signed */
      else if (bits_per_sample <= 32)
	audio_type = AVT_AUDIO_S32LE;	/* signed */
      else
	return NULL;
      break;

    case 6:			/* A-law */
      audio_type = AVT_AUDIO_ALAW;
      break;

    case 7:			/* mu-law */
      audio_type = AVT_AUDIO_MULAW;
      break;

    default:			/* unsupported encoding */
      return NULL;
    }

  /* search data chunk - must be after format chunk */
  do
    {
      if (avt_data_read (src, &identifier, sizeof (identifier), 1) != 1)
	return NULL;		/* no data chunk found */
      chunk_size = avt_read32le (src);
      chunk_end = avt_data_tell (src) + chunk_size;
      if (chunk_end % 2 != 0)
	chunk_end++;		/* padding to even addresses */
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

/* src gets always closed */
static avt_audio *
avt_load_audio_general (avt_data * src, uint32_t maxsize, int playmode)
{
  struct avt_audio *s;
  int start;
  char head[16];

  if (src == NULL)
    return NULL;

  if (maxsize == 0)
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
	   && memcmp (&head[8], "WAVE", 4) == 0)
    s = avt_load_wave (src, maxsize, playmode);
  else
    {
      s = NULL;
      avt_set_error ("audio data neither in AU nor WAVE format");
    }

  avt_data_close (src);
  return s;
}

#endif /* not NO_AUDIO */


extern avt_audio *
avt_load_audio_file (const char *file, int playmode)
{
  return avt_load_audio_general (avt_data_open_file (file, "rb"), MAXIMUM_SIZE,
				 playmode);
}

extern avt_audio *
avt_load_audio_part (avt_stream * stream, size_t maxsize, int playmode)
{
  return avt_load_audio_general (avt_data_open_stream ((FILE *) stream),
				 maxsize, playmode);
}

extern avt_audio *
avt_load_audio_stream (avt_stream * stream, int playmode)
{
  return avt_load_audio_general (avt_data_open_stream ((FILE *) stream),
				 MAXIMUM_SIZE, playmode);
}

extern avt_audio *
avt_load_audio_data (void *data, size_t datasize, int playmode)
{
  return avt_load_audio_general (avt_data_open_memory (data, datasize),
				 datasize, playmode);
}
