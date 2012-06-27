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

#include "avtdata.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>		// memcpy

#ifndef __cplusplus
#include <stdbool.h>
#endif

#define AVT_DATA_STREAM 1
#define AVT_DATA_MEMORY 2


union avt_data
{
  int type;

  struct
  {
    int type;			// overlays type
    FILE *data;
    size_t start;
    bool autoclose;
  } stream;

  struct
  {
    int type;			// overlays type
    const unsigned char *data;
    size_t size;
    size_t position;
  } memory;
};


extern void
avt_data_close (avt_data * d)
{
  if (d)
    {
      if (AVT_DATA_STREAM == d->type && d->stream.autoclose)
	fclose (d->stream.data);

      free (d);
    }
}


extern size_t
avt_data_read (avt_data * d, void *data, size_t size, size_t number)
{
  size_t result = 0;

  if (!d)
    return 0;

  switch (d->type)
    {
    case AVT_DATA_STREAM:
      result = fread (data, size, number, d->stream.data);
      break;

    case AVT_DATA_MEMORY:
      {
	size_t all = size * number;
	size_t position = d->memory.position;
	size_t datasize = d->memory.size;

	if (position + all > datasize)
	  {
	    // at least 1 element readable?
	    if (position < datasize && (datasize - position) >= size)
	      {
		// integer division ignores the rest
		number = (datasize - position) / size;
		all = size * number;
	      }
	    else
	      break;		// nothing readable
	  }

	memcpy (data, d->memory.data + position, all);
	d->memory.position += all;
	result = number;
      }
      break;
    }

  return result;
}


// read 8 bit value
extern uint8_t
avt_data_read8 (avt_data * d)
{
  uint8_t data;

  avt_data_read (d, &data, sizeof (data), 1);

  return data;
}


// read little endian 16 bit value
extern uint16_t
avt_data_read16le (avt_data * d)
{
  uint8_t data[2];

  avt_data_read (d, &data, sizeof (data), 1);

  return data[1] << 8 | data[0];
}


// read big endian 16 bit value
extern uint16_t
avt_data_read16be (avt_data * d)
{
  uint8_t data[2];

  avt_data_read (d, &data, sizeof (data), 1);

  return data[0] << 8 | data[1];
}


// read little endian 32 bit value
extern uint32_t
avt_data_read32le (avt_data * d)
{
  uint8_t data[4];

  avt_data_read (d, &data, sizeof (data), 1);

  return data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
}


// read big endian 32 bit value
extern uint32_t
avt_data_read32be (avt_data * d)
{
  uint8_t data[4];

  avt_data_read (d, &data, sizeof (data), 1);

  return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
}


extern long
avt_data_tell (avt_data * d)
{
  long result = -1;

  if (!d)
    return -1;

  switch (d->type)
    {
    case AVT_DATA_STREAM:
      result = ftell (d->stream.data) - d->stream.start;
      break;

    case AVT_DATA_MEMORY:
      if (d->memory.position <= d->memory.size)
	result = d->memory.position;
      break;
    }

  return result;
}


extern bool
avt_data_seek (avt_data * d, long offset, int whence)
{
  bool okay = false;

  if (!d)
    return false;

  switch (d->type)
    {
    case AVT_DATA_STREAM:
      if (SEEK_SET == whence)
	offset += d->stream.start;

      okay = (fseek (d->stream.data, offset, whence) > -1);
      break;

    case AVT_DATA_MEMORY:
      if (SEEK_SET == whence)
	d->memory.position = offset;
      else if (SEEK_CUR == whence)
	d->memory.position += offset;
      else if (SEEK_END == whence)
	d->memory.position = d->memory.size - offset;

      okay = (d->memory.position <= d->memory.size);
      break;
    }

  return okay;
}


extern avt_data *
avt_data_open_stream (FILE * stream, bool autoclose)
{
  avt_data *d;

  if (!stream)
    return NULL;

  d = (avt_data *) malloc (sizeof (avt_data));

  if (d)
    {
      d->type = AVT_DATA_STREAM;
      d->stream.data = stream;
      d->stream.start = ftell (stream);
      d->stream.autoclose = autoclose;
    }

  return d;
}


extern avt_data *
avt_data_open_file (const char *filename)
{
  return avt_data_open_stream (fopen (filename, "rb"), true);
}


extern avt_data *
avt_data_open_memory (const void *memory, size_t size)
{
  avt_data *d;

  if (memory == NULL || size == 0)
    return NULL;

  d = (avt_data *) malloc (sizeof (avt_data));

  if (d)
    {
      d->type = AVT_DATA_MEMORY;
      d->memory.data = (const unsigned char *) memory;
      d->memory.position = 0;
      d->memory.size = size;
    }

  return d;
}
