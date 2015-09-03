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

  // open a stream
  // if autoclose is true avt_data_close closes the stream with fclose
  bool (*open_stream) (avt_data * self, FILE * stream, bool autoclose);

  // open a file for reading in binary mode
  bool (*open_file) (avt_data * self, const char *filename);

  // read data from memory
  // the memory area must be kept available until closed
  bool (*open_memory) (avt_data * self, const void *memory, size_t size);

  void (*big_endian) (avt_data * self, bool big_endian);

  void (*done) (avt_data * self);	// destructor

  bool (*seek) (avt_data * self, long offset, int whence);

  long (*tell) (avt_data * self);

  size_t (*read) (avt_data * self, void *data, size_t size, size_t number);

  // skip also works on nonseekable streams
  void (*skip) (avt_data * self, size_t size);

  uint_least8_t (*read8) (avt_data * self);

  uint_least16_t (*read16) (avt_data * self);

  uint_least32_t (*read32) (avt_data * self);

  int (*filenumber) (avt_data * self);

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
      const unsigned char *data;
      size_t size;
      size_t position;
    } memory;
  } priv;
};

void avt_data_init (avt_data *);	// constructor

// duplicate the data element
// the result must be freed by the caller after calling done
avt_data *avt_data_dup (avt_data * d);

#endif // AVTDATA_H
