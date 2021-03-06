/*
 * data handling
 * Copyright (c) 2012,2013,2014,2015
 * Andreas K. Foerster <akf@akfoerster.de>
 *
 * required standards: C99
 *
 * This file is part of AKFAvatar
 * This file is not part of the official API
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

#ifndef AVTDATA_H
#define AVTDATA_H

#include <stdio.h>		// FILE
#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef struct avt_data avt_data;

struct avt_data
{
  // public

  void (*done) (avt_data *);	// destructor

  int (*filenumber) (avt_data *);

  size_t (*read) (avt_data *, void *, size_t size, size_t number);

  long (*tell) (avt_data *);

  bool (*seek) (avt_data *, long offset, int whence);

  // skip also works on nonseekable streams
  void (*skip) (avt_data *, size_t);

  // use avt_data_big_endian() before calling one of these
  uint_least16_t (*read16) (avt_data *);
  uint_least32_t (*read32) (avt_data *);


  // private
  union
  {
    struct
    {
      FILE *data;
      bool autoclose;
    } stream;

    struct
    {
      const uint_least8_t *data;
      size_t size, position;
    } memory;
  } priv;
};

void avt_data_init (avt_data *);	// constructor
#define avt_data_done(d) (d)->done(d)
#define avt_data_filenumber(d) (d)->filenumber(d)

// duplicate the data element
// the result must be freed by the caller after calling done
avt_data *avt_data_dup (avt_data *);

// open a stream
// if autoclose is true done closes the stream with fclose
bool avt_data_open_stream (avt_data *, FILE *, bool autoclose);

// open a file for reading in binary mode
bool avt_data_open_file (avt_data *, const char *);

// read data from memory
// the memory area must be kept available until closed
bool avt_data_open_memory (avt_data *, const void *, size_t);

#define avt_data_read(d,p,size,number) (d)->read((d),(p),(size),(number))
uint_least8_t avt_data_read8 (avt_data *);
#define avt_data_read16(d) (d)->read16(d)
#define avt_data_read32(d) (d)->read32(d)

#define avt_data_tell(d) (d)->tell(d)
#define avt_data_seek(d,offset,whence) (d)->seek((d),(offset),(whence))
#define avt_data_skip(d,size) (d)->skip((d),(size))

// set this before using read16 or read32
void avt_data_big_endian (avt_data *, bool);

#endif // AVTDATA_H
