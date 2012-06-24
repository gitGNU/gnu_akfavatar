/*
 * AKFAvatar Ogg Vorbis decoder
 * based on stb_vorbis
 * ATTENTION: this is work in progress, ie. not finished yet
 * Copyright (c) 2011,2012 Andreas K. Foerster <info@akfoerster.de>
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
#include "avtaddons.h"
#include <stdio.h>
#include <stdlib.h>

#define STB_VORBIS_HEADER_ONLY 1
#define STB_VORBIS_NO_PUSHDATA_API 1
#include "stb_vorbis.c"

/*
 * SDL-1.2 doesn't support more than 2 channels
 * but you may want to set this to AVT_AUDIO_MONO to save memory
 */
#define MAX_CHANNELS  AVT_AUDIO_STEREO

static avt_audio *
load_vorbis (stb_vorbis * vorbis, int playmode)
{
  int data_len, offset, total, limit, n;
  unsigned int samples;
  stb_vorbis_info info;
  avt_audio *audio;
  short data[12 * 1024];

  info = stb_vorbis_get_info (vorbis);
  samples = stb_vorbis_stream_length_in_samples (vorbis);

  limit = info.channels * 4096;
  total = sizeof (data) / sizeof (data[0]);
  offset = data_len = 0;

  if (info.channels > MAX_CHANNELS)
    info.channels = MAX_CHANNELS;

  audio = avt_load_raw_audio_data (NULL, 0, info.sample_rate,
				   AVT_AUDIO_S16SYS, info.channels);

  if (samples > 0)
    avt_set_raw_audio_capacity (audio,
				samples * sizeof (data[0]) * info.channels);

  while ((n = stb_vorbis_get_frame_short_interleaved (vorbis,
						      info.channels,
						      &data[offset],
						      total - offset)) != 0)
    {
      data_len += n;
      offset += n * info.channels;

      // buffer full?
      if (offset + limit > total)
	{
	  if (avt_add_raw_audio_data (audio, data,
				      data_len * sizeof (data[0]) *
				      info.channels) != AVT_NORMAL)
	    {
	      avt_free_audio (audio);
	      return NULL;
	    }

	  if (playmode != AVT_LOAD)
	    {
	      avt_play_audio (audio, playmode);
	      playmode = AVT_LOAD;
	    }

	  offset = data_len = 0;
	}
    }

  if (data_len > 0 && avt_add_raw_audio_data (audio, data,
					      data_len * sizeof (*data) *
					      info.channels) != AVT_NORMAL)
    {
      avt_free_audio (audio);
      return NULL;
    }

  // if not started yet, start it
  if (playmode != AVT_LOAD)
    avt_play_audio (audio, playmode);

  avt_finalize_raw_audio (audio);

  return audio;
}

extern avt_audio *
avta_load_vorbis_stream (avt_stream * stream, size_t size, int playmode)
{
  FILE *f;
  int error;
  long start;
  stb_vorbis *vorbis;
  avt_audio *audio_data;
  char buf[40];

  f = (FILE *) stream;
  start = ftell (f);

  // check content, must be plain vorbis with no other streams
  if (fread (&buf, sizeof (buf), 1, f) < 1
      || memcmp ("OggS", buf, 4) != 0
      || memcmp ("\x01vorbis", buf + 28, 7) != 0)
    {
      fseek (f, start, SEEK_SET);
      return NULL;
    }

  if (size == 0)
    {
      // get the size
      // ugly, but stb_vorbis does roughly the same
      fseek (f, 0, SEEK_END);
      size = ftell (f) - start;
    }

  fseek (f, start, SEEK_SET);
  vorbis = stb_vorbis_open_file_section (f, 0, &error, NULL, size);

  if (vorbis)
    {
      audio_data = load_vorbis (vorbis, playmode);
      stb_vorbis_close (vorbis);
    }
  else				// error
    {
      fseek (f, start, SEEK_SET);
      audio_data = NULL;
    }

  return audio_data;
}

extern avt_audio *
avta_load_vorbis_file (char *filename, int playmode)
{
  FILE *f;
  avt_audio *audio_data;

  if (!filename || !*filename)
    return NULL;

  f = fopen (filename, "rb");

  if (!f)
    return NULL;

  audio_data = avta_load_vorbis_stream (f, 0, playmode);
  fclose (f);

  return audio_data;
}

extern avt_audio *
avta_load_vorbis_data (void *data, int datasize, int playmode)
{
  int error;
  stb_vorbis *vorbis;
  avt_audio *audio_data;

  // check content, must be plain vorbis with no other streams
  if (!data || datasize <= 0
      || memcmp ("OggS", data, 4) != 0
      || memcmp ("\x01vorbis", ((char *) data) + 28, 7) != 0)
    return NULL;

  vorbis =
    stb_vorbis_open_memory ((unsigned char *) data, datasize, &error, NULL);
  if (vorbis == NULL)
    return NULL;

  audio_data = load_vorbis (vorbis, playmode);
  stb_vorbis_close (vorbis);

  return audio_data;
}
