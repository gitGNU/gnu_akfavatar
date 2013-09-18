/*
 * data reading abstraction
 * Copyright (c) 2012,2013 Andreas K. Foerster <info@akfoerster.de>
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
#define _XOPEN_SOURCE 600

#include "avtinternals.h"
#include "avtdata.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>		// memcpy
#include <iso646.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

// reset virtual methods
static inline void
reset (avt_data * d)
{
  d->seek = NULL;
  d->tell = NULL;
  d->read = NULL;
  d->read8 = NULL;
  d->read16 = NULL;
  d->read32 = NULL;
  d->fileno = NULL;
}

static void
method_done_stream (avt_data * d)
{
  if (d->field.stream.autoclose)
    {
      fclose (d->field.stream.data);
      d->field.stream.data = NULL;
      d->field.stream.autoclose = false;
    }

  reset (d);
}


static void
method_done_memory (avt_data * d)
{
  d->field.memory.data = NULL;
  d->field.memory.position = 0;
  d->field.memory.size = 0;

  reset (d);
}


static size_t
method_read_stream (avt_data * d, void *data, size_t size, size_t number)
{
  return fread (data, size, number, d->field.stream.data);
}


static size_t
method_read_memory (avt_data * d, void *data, size_t size, size_t number)
{
  size_t result = 0;
  size_t all = size * number;
  size_t position = d->field.memory.position;
  size_t datasize = d->field.memory.size;

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

  memcpy (data, d->field.memory.data + position, all);
  d->field.memory.position += all;
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


// read little endian 24 bit value
static uint_least32_t
method_read24le (avt_data * d)
{
  uint_least8_t data[3];

  d->read (d, &data, sizeof (data), 1);

  return data[2] << 16 bitor data[1] << 8 bitor data[0];
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


// read big endian 24 bit value
static uint_least32_t
method_read24be (avt_data * d)
{
  uint_least32_t data;

  d->read (d, &data, 3, 1);

  return (data >> 8);
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


// read little endian 24 bit value
static uint_least32_t
method_read24le (avt_data * d)
{
  uint_least32_t data;

  d->read (d, &data, 3, 1);

  return (data bitand 0xFFFFFF00);
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


// read big endian 24 bit value
static uint_least32_t
method_read24be (avt_data * d)
{
  uint_least8_t data[3];

  d->read (d, &data, sizeof (data), 1);

  return data[0] << 16 bitor data[1] << 8 bitor data[2];
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
  return ftell (d->field.stream.data);
}


static long
method_tell_memory (avt_data * d)
{
  if (d->field.memory.position <= d->field.memory.size)
    return d->field.memory.position;
  else
    return -1;
}


static bool
method_seek_stream (avt_data * d, long offset, int whence)
{
  return (fseek (d->field.stream.data, offset, whence) > -1);
}


static bool
method_seek_memory (avt_data * d, long offset, int whence)
{
  if (SEEK_SET == whence)
    d->field.memory.position = offset;
  else if (SEEK_CUR == whence)
    d->field.memory.position += offset;
  else if (SEEK_END == whence)
    d->field.memory.position = d->field.memory.size - offset;

  return (d->field.memory.position <= d->field.memory.size);
}


static int
method_fileno_stream (avt_data * d)
{
  return fileno (d->field.stream.data);
}


static int
method_fileno_memory (avt_data * d)
{
  (void) d;
  return -1;
}


static void
method_big_endian (avt_data * d, bool big_endian)
{
  if (d)
    {
      if (big_endian)
	{
	  d->read16 = method_read16be;
	  d->read24 = method_read24be;
	  d->read32 = method_read32be;
	}
      else
	{
	  d->read16 = method_read16le;
	  d->read24 = method_read24le;
	  d->read32 = method_read32le;
	}
    }
}

static bool
method_open_stream (avt_data * d, FILE * stream, bool autoclose)
{
  if (not d or not stream or d->seek)
    return false;

  d->done = method_done_stream;
  d->read = method_read_stream;
  d->tell = method_tell_stream;
  d->seek = method_seek_stream;
  d->fileno = method_fileno_stream;

  d->field.stream.data = stream;
  d->field.stream.autoclose = autoclose;

  d->open_stream = NULL;
  d->open_file = NULL;
  d->open_memory = NULL;

  return true;
}


static bool
method_open_file (avt_data * d, const char *filename)
{
  if (not d or not filename or d->seek)
    return false;

  return d->open_stream (d, fopen (filename, "rb"), true);
}


static bool
method_open_memory (avt_data * d, const void *memory, size_t size)
{
  if (not d or not memory or not size or d->seek)
    return false;

  d->done = method_done_memory;
  d->read = method_read_memory;
  d->tell = method_tell_memory;
  d->seek = method_seek_memory;
  d->fileno = method_fileno_memory;

  d->field.memory.data = memory;
  d->field.memory.position = 0;
  d->field.memory.size = size;

  d->open_stream = NULL;
  d->open_file = NULL;
  d->open_memory = NULL;

  return true;
}

extern void
avt_data_init (avt_data * d)
{
  if (d)
    {
      reset (d);
      d->open_stream = method_open_stream;
      d->open_file = method_open_file;
      d->open_memory = method_open_memory;
      d->done = method_done_memory;
      d->big_endian = method_big_endian;
      d->read8 = method_read8;
    }
}
