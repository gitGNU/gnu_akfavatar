/*
 * data reading abstraction
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
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

#include "avtdata.h"
#include "avtinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>		// memcpy
#include <iso646.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif


static void
method_close_stream (avt_data * d)
{
  if (d->stream.autoclose)
    fclose (d->stream.data);

  memset (d, 0, sizeof (avt_data));
  free (d);
}


static void
method_close_memory (avt_data * d)
{
  memset (d, 0, sizeof (avt_data));
  free (d);
}


static size_t
method_read_stream (avt_data * d, void *data, size_t size, size_t number)
{
  return fread (data, size, number, d->stream.data);
}


static size_t
method_read_memory (avt_data * d, void *data, size_t size, size_t number)
{
  size_t result = 0;
  size_t all = size * number;
  size_t position = d->memory.position;
  size_t datasize = d->memory.size;

  // not all readable?
  if (position + all > datasize)
    {
      // at least 1 element readable?
      if (position < datasize and (datasize - position) >= size)
	{
	  // integer division ignores the rest
	  number = (datasize - position) / size;
	  all = size * number;
	}
      else
	return 0;		// nothing readable
    }

  memcpy (data, d->memory.data + position, all);
  d->memory.position += all;
  result = number;

  return result;
}


// read 8 bit value
static uint_least8_t
method_read8 (avt_data * d)
{
  uint_least8_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


#if AVT_BIG_ENDIAN == AVT_BYTE_ORDER

// read little endian 16 bit value
static uint_least16_t
method_read16le (avt_data * d)
{
  uint_least8_t data[2];

  d->read (d, &data, sizeof (data), 1);

  return data[1] << 8 bitor data[0];
}


// read little endian 32 bit value
static uint_least32_t
method_read32le (avt_data * d)
{
  uint_least8_t data[4];

  d->read (d, &data, sizeof (data), 1);

  return data[3] << 24 bitor data[2] << 16 bitor data[1] << 8 bitor data[0];
}


// read big endian 16 bit value
static uint_least16_t
method_read16be (avt_data * d)
{
  uint_least16_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


// read big endian 32 bit value
static uint_least32_t
method_read32be (avt_data * d)
{
  uint_least32_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


#else // little endian

// read little endian 16 bit value
static uint_least16_t
method_read16le (avt_data * d)
{
  uint_least16_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


// read little endian 32 bit value
static uint_least32_t
method_read32le (avt_data * d)
{
  uint_least32_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}

// read big endian 16 bit value
static uint_least16_t
method_read16be (avt_data * d)
{
  uint_least8_t data[2];

  d->read (d, &data, sizeof (data), 1);

  return data[0] << 8 bitor data[1];
}


// read big endian 32 bit value
static uint_least32_t
method_read32be (avt_data * d)
{
  uint_least8_t data[4];

  d->read (d, &data, sizeof (data), 1);

  return data[0] << 24 bitor data[1] << 16 bitor data[2] << 8 bitor data[3];
}

#endif // little endian


static long
method_tell_stream (avt_data * d)
{
  return ftell (d->stream.data) - d->stream.start;
}


static long
method_tell_memory (avt_data * d)
{
  if (d->memory.position <= d->memory.size)
    return d->memory.position;
  else
    return -1;
}


static bool
method_seek_stream (avt_data * d, long offset, int whence)
{
  if (SEEK_SET == whence)
    offset += d->stream.start;

  return (fseek (d->stream.data, offset, whence) > -1);
}


static bool
method_seek_memory (avt_data * d, long offset, int whence)
{
  if (SEEK_SET == whence)
    d->memory.position = offset;
  else if (SEEK_CUR == whence)
    d->memory.position += offset;
  else if (SEEK_END == whence)
    d->memory.position = d->memory.size - offset;

  return (d->memory.position <= d->memory.size);
}


static void
method_big_endian (avt_data * d, bool big_endian)
{
  if (d)
    {
      if (big_endian)
	{
	  d->read16 = method_read16be;
	  d->read32 = method_read32be;
	}
      else
	{
	  d->read16 = method_read16le;
	  d->read32 = method_read32le;
	}
    }
}

static void
method_open_stream (avt_data * d, FILE * stream, bool autoclose)
{
  if (not d or not stream)
    return;

  d->close = method_close_stream;
  d->read = method_read_stream;
  d->tell = method_tell_stream;
  d->seek = method_seek_stream;

  d->stream.data = stream;
  d->stream.start = ftell (stream);
  d->stream.autoclose = autoclose;

  d->open_stream = NULL;
  d->open_file = NULL;
  d->open_memory = NULL;
}


static void
method_open_file (avt_data * d, const char *filename)
{
  d->open_stream (d, fopen (filename, "rb"), true);
}


static void
method_open_memory (avt_data * d, const void *memory, size_t size)
{
  if (not memory or not size)
    return;

  d->close = method_close_memory;
  d->read = method_read_memory;
  d->tell = method_tell_memory;
  d->seek = method_seek_memory;

  d->memory.data = memory;
  d->memory.position = 0;
  d->memory.size = size;

  d->open_stream = NULL;
  d->open_file = NULL;
  d->open_memory = NULL;
}

extern avt_data *
avt_data_new (void)
{
  avt_data *d;

  d = malloc (sizeof (avt_data));

  if (d)
    {
      memset (d, 0, sizeof (avt_data));
      d->open_stream = method_open_stream;
      d->open_file = method_open_file;
      d->open_memory = method_open_memory;
      d->close = method_close_memory;
      d->big_endian = method_big_endian;
      d->read8 = method_read8;
    }

  return d;
}
