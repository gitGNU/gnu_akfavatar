/*
 * data handling
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
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

typedef union avt_data avt_data;

// open a stream
// if autoclose is true avt_data_close closes the stream with fclose
avt_data *avt_data_open_stream (FILE * stream, bool autoclose);

// open a file for reading in binary mode
avt_data *avt_data_open_file (const char *filename);

// read data from memory
// the memory area must be kept available until closed
avt_data *avt_data_open_memory (const void *memory, size_t size);

// closes the data construct
// eventually also closes the stream with fclose
void avt_data_close (avt_data *d);

// read data
size_t avt_data_read (avt_data *d, void *data, size_t size, size_t number);

bool avt_data_seek (avt_data *d, long offset, int whence);
long avt_data_tell (avt_data *d);

// read values of specific size and endianness 
// le = Little Endian, be = Big Endian
uint8_t  avt_data_read8    (avt_data *d);
uint16_t avt_data_read16le (avt_data *d);
uint16_t avt_data_read16be (avt_data *d);
uint32_t avt_data_read32le (avt_data *d);
uint32_t avt_data_read32be (avt_data *d);

#define avt_data_rewind(d)  avt_data_seek((d), 0, SEEK_SET)

#endif // AVTDATA_H