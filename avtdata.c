/*
 * data reading abstraction
 * Copyright (c) 2012,2013,2014,2015
 * Andreas K. Foerster <akf@akfoerster.de>
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>		// memcpy
#include <iso646.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(__GNUC__) \
  and ((__GNUC__ > 4) or ((__GNUC__ == 4) and (__GNUC_MINOR__ >= 8)))
#define avt_data_bswap16(x)  __builtin_bswap16(x)
#define avt_data_bswap32(x)  __builtin_bswap32(x)
#else
#define avt_data_bswap16(x)  (((x)<<8|(x)>>8)&0xFFFF)
#define avt_data_bswap32(x)  \
  (((x)<<24)|(((x)&0xFF00)<<8)|(((x)&0xFF0000)>>8)|((x)>>24))
#endif

// reset virtual methods
static inline void
reset (avt_data * d)
{
  d->seek = NULL;
  d->tell = NULL;
  d->read = NULL;
  d->read16 = NULL;
  d->read32 = NULL;
  d->filenumber = NULL;
}

static void
method_done_stream (avt_data * d)
{
  if (d->priv.stream.autoclose)
    {
      fclose (d->priv.stream.data);
      d->priv.stream.data = NULL;
      d->priv.stream.autoclose = false;
    }

  reset (d);
}


static void
method_done_memory (avt_data * d)
{
  d->priv.memory.data = NULL;
  d->priv.memory.position = 0;
  d->priv.memory.size = 0;

  reset (d);
}


static size_t
method_read_stream (avt_data * d, void *data, size_t size, size_t number)
{
  return fread (data, size, number, d->priv.stream.data);
}

// skip also works on nonseekable streams
static void
method_skip_stream (avt_data * d, size_t size)
{
  char buffer[BUFSIZ];

  while (size > sizeof (buffer))
    {
      fread (buffer, sizeof (buffer), 1, d->priv.stream.data);
      size -= sizeof (buffer);
    }

  // size <= sizeof(buffer)
  fread (buffer, 1, size, d->priv.stream.data);
}

static void
method_skip_memory (avt_data * d, size_t size)
{
  d->priv.memory.position += size;
}

static size_t
method_read_memory (avt_data * d, void *data, size_t size, size_t number)
{
  size_t result = 0;
  size_t all = size * number;
  size_t position = d->priv.memory.position;
  size_t datasize = d->priv.memory.size;

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

  memcpy (data, d->priv.memory.data + position, all);
  d->priv.memory.position += all;
  result = number;

  return result;
}


// read 8 bit value
uint_least8_t
avt_data_read8 (avt_data * d)
{
  uint_least8_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


// read 16 bit value
static uint_least16_t
method_read16 (avt_data * d)
{
  uint_least16_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


// read 16 bit value with bytes swapped
static uint_least16_t
method_read16swap (avt_data * d)
{
  uint_least16_t data;

  d->read (d, &data, sizeof (data), 1);

  return avt_data_bswap16 (data);
}


// read 32 bit value
static uint_least32_t
method_read32 (avt_data * d)
{
  uint_least32_t data;

  d->read (d, &data, sizeof (data), 1);

  return data;
}


// read 32 bit value with bytes swapped
static uint_least32_t
method_read32swap (avt_data * d)
{
  uint_least32_t data;

  d->read (d, &data, sizeof (data), 1);

  return avt_data_bswap32 (data);
}


static long
method_tell_stream (avt_data * d)
{
  return ftell (d->priv.stream.data);
}


static long
method_tell_memory (avt_data * d)
{
  if (d->priv.memory.position <= d->priv.memory.size)
    return d->priv.memory.position;
  else
    return -1;
}


static bool
method_seek_stream (avt_data * d, long offset, int whence)
{
  return (fseek (d->priv.stream.data, offset, whence) > -1);
}


static bool
method_seek_memory (avt_data * d, long offset, int whence)
{
  switch (whence)
    {
    case SEEK_SET:
      d->priv.memory.position = offset;
      break;

    case SEEK_CUR:
      d->priv.memory.position += offset;
      break;

    case SEEK_END:
      d->priv.memory.position = d->priv.memory.size - offset;
      break;
    }

  return (d->priv.memory.position <= d->priv.memory.size);
}


static int
method_filenumber_stream (avt_data * d)
{
  return fileno (d->priv.stream.data);
}


static int
method_filenumber_memory (avt_data * d)
{
  (void) d;
  return -1;
}


void
avt_data_big_endian (avt_data * d, bool big_endian_data)
{
  if (d)
    {
      if ((AVT_BIG_ENDIAN == AVT_BYTE_ORDER and big_endian_data)
	  or (AVT_LITTLE_ENDIAN == AVT_BYTE_ORDER and not big_endian_data))
	{
	  d->read16 = method_read16;
	  d->read32 = method_read32;
	}
      else
	{
	  d->read16 = method_read16swap;
	  d->read32 = method_read32swap;
	}
    }
}


bool
avt_data_open_stream (avt_data * d, FILE * stream, bool autoclose)
{
  if (not d or not stream or d->seek)
    return false;

  d->done = method_done_stream;
  d->read = method_read_stream;
  d->skip = method_skip_stream;
  d->tell = method_tell_stream;
  d->seek = method_seek_stream;
  d->filenumber = method_filenumber_stream;

  d->priv.stream.data = stream;
  d->priv.stream.autoclose = autoclose;

  return true;
}


bool
avt_data_open_file (avt_data * d, const char *filename)
{
  if (not d or not filename or d->seek)
    return false;

  return avt_data_open_stream (d, fopen (filename, "rb"), true);
}


bool
avt_data_open_memory (avt_data * d, const void *memory, size_t size)
{
  if (not d or not memory or not size or d->seek)
    return false;

  d->done = method_done_memory;
  d->read = method_read_memory;
  d->skip = method_skip_memory;
  d->tell = method_tell_memory;
  d->seek = method_seek_memory;
  d->filenumber = method_filenumber_memory;

  d->priv.memory.data = memory;
  d->priv.memory.position = 0;
  d->priv.memory.size = size;

  return true;
}

extern void
avt_data_init (avt_data * d)
{
  if (d)
    {
      reset (d);
      d->done = method_done_memory;
    }
}

extern avt_data *
avt_data_dup (avt_data * d)
{
  avt_data *n = malloc (sizeof (*d));

  if (n)
    memcpy (n, d, sizeof (*d));

  return n;
}
