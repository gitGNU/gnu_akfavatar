/*
 * BMP support for AKFAvatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013,2014,2015
 * Andreas K. Foerster <akf@akfoerster.de>
 *
 * required standards: C99, POSIX.1-2001
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

#include "avtgraphic.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <iso646.h>

static inline short
get_right_shift (uint32_t mask)
{
  register short shift;

  shift = 0;

  while ((mask bitand 1) == 0)
    {
      shift++;
      mask >>= 1;
    }

  return shift;
}

static inline short
get_left_shift (uint32_t mask)
{
  register short shift;

  shift = 0;

  while ((mask bitand 0x80) == 0)
    {
      shift++;
      mask <<= 1;
    }

  return shift;
}

extern avt_graphic *
avt_load_image_bmp_data (avt_data * src)
{
  avt_graphic *image;
  long start;
  uint32_t bits_offset, info_size, compression;
  uint32_t colors_used;
  int32_t width, height;
  uint32_t red_mask, green_mask, blue_mask;
  uint16_t bits_per_pixel;
  avt_color palette[256];

  image = NULL;
  red_mask = green_mask = blue_mask = 0;

  start = src->tell (src);

  avt_data_big_endian (src, false);

  // check magic number ("BM")
  if (src->read16 (src) != 0x4D42)
    goto done;

  // skip filesize (4) and reserved (4)
  src->seek (src, 2 * 4, SEEK_CUR);

  bits_offset = src->read32 (src);

  info_size = src->read32 (src);

  if (info_size == 12)
    {
      // old OS/2 format
      width = src->read16 (src);
      height = src->read16 (src);	// negative for top-down

      // skip planes (unused for BMP)
      src->seek (src, 2, SEEK_CUR);

      bits_per_pixel = src->read16 (src);
      compression = 0;
      colors_used = 0;
    }
  else				// info_size > 12
    {
      width = src->read32 (src);
      height = src->read32 (src);	// negative for top-down

      // skip planes (unused for BMP)
      src->seek (src, 2, SEEK_CUR);

      bits_per_pixel = src->read16 (src);
      compression = src->read32 (src);

      // skip image data size and pixels per meter (x an y)
      src->seek (src, 3 * 4, SEEK_CUR);

      colors_used = src->read32 (src);
      src->seek (src, 4, SEEK_CUR);	// important colors
    }

  // just uncompressed allowed for now
  if (compression != 0 and compression != 3)
    goto done;

  if (compression == 3)
    {
      // masks, just for 16 or 32 bit per pixel allowed
      red_mask = src->read32 (src);
      green_mask = src->read32 (src);
      blue_mask = src->read32 (src);
    }
  else if (bits_per_pixel <= 8)
    {
      // read palette
      // a palette is required when bits per pixel <= 8

      //  skip end of header
      src->seek (src, start + 14 + info_size, SEEK_SET);

      if (colors_used == 0)
	colors_used = 1 << bits_per_pixel;

      memset (palette, 0, sizeof (palette));

      if (info_size != 12)
	{
	  for (uint16_t color = 0; color < colors_used; color++)
	    palette[color] = src->read32 (src) bitand 0xFFFFFF;
	}
      else			// old OS/2 format
	{
	  uint8_t red, green, blue;

	  for (uint16_t color = 0; color < colors_used; color++)
	    {
	      blue = avt_data_read8 (src);
	      green = avt_data_read8 (src);
	      red = avt_data_read8 (src);
	      palette[color] = (red << 16) | (green << 8) | blue;
	    }
	}
    }

  // go to image data
  src->seek (src, start + bits_offset, SEEK_SET);

  /*
   * An image can be stored from bottom to top or from top to bottom.
   * This makes absolutely no technical sense, as far as I can tell.
   * I think it's only there to discourage reimplementations.
   */

  int y, direction;

  // if height is positive it's bottom-up
  if (height > 0)
    {
      y = height - 1;
      direction = -1;
    }
  else				// else top-down
    {
      y = 0;
      direction = 1;
    }

  height = abs (height);

  switch (bits_per_pixel)
    {
    case 1:
      {
	image = avt_new_graphic (width, height);
	if (not image)
	  goto done;

	int remainder = ((width + 7) / 8) % 4;

	while (y >= 0 and y < height)
	  {
	    avt_color *p = image->pixels + y * image->width;

	    for (int x = 0; x < width; x += 8)
	      {
		uint8_t value = avt_data_read8 (src);
		uint8_t bitmask = 0x80;
		for (int bit = 0; bit < 8; bit++, bitmask >>= 1)
		  if (x + bit < width)
		    *p++ = palette[(value bitand bitmask) != 0];
	      }

	    if (remainder)
	      src->seek (src, 4 - remainder, SEEK_CUR);

	    y += direction;
	  }
      }
      break;

    case 4:
      {
	image = avt_new_graphic (width, height);
	if (not image)
	  goto done;

	int remainder = (width / 2 + width % 2) % 4;

	while (y >= 0 and y < height)
	  {
	    avt_color *p = image->pixels + y * image->width;

	    for (int x = 0; x < width; x += 2)
	      {
		uint16_t value = avt_data_read8 (src);
		*p++ = palette[value >> 4 bitand 0xF];
		if (x < width - 1)
		  *p++ = palette[value bitand 0xF];
	      }

	    if (remainder)
	      src->seek (src, 4 - remainder, SEEK_CUR);

	    y += direction;
	  }
      }
      break;

    case 8:
      {
	image = avt_new_graphic (width, height);
	if (not image)
	  goto done;

	int remainder = width % 4;

	while (y >= 0 and y < height)
	  {
	    avt_color *p = image->pixels + y * image->width;

	    for (int x = 0; x < width; x++, p++)
	      {
		uint16_t color = avt_data_read8 (src);
		*p = palette[color];
	      }

	    if (remainder)
	      src->seek (src, 4 - remainder, SEEK_CUR);

	    y += direction;
	  }
      }
      break;

    case 16:
      {
	short red_right, green_right, blue_right;
	short red_left, green_left, blue_left;

	if (compression != 3)
	  {
	    red_mask = 0x7C00;
	    green_mask = 0x03E0;
	    blue_mask = 0x001F;
	  }

	// colors get shifted right and then left again
	red_right = get_right_shift (red_mask);
	red_left = get_left_shift (red_mask >> red_right);
	green_right = get_right_shift (green_mask);
	green_left = get_left_shift (green_mask >> green_right);
	blue_right = get_right_shift (blue_mask);
	blue_left = get_left_shift (blue_mask >> blue_right);

	image = avt_new_graphic (width, height);
	if (not image)
	  goto done;

	int remainder = (2 * width) % 4;

	while (y >= 0 and y < height)
	  {
	    avt_color *p = image->pixels + y * image->width;

	    for (int x = 0; x < width; x++, p++)
	      {
		register uint16_t color = src->read16 (src);

		*p =
		  ((color & red_mask) >> red_right << red_left) << 16
		  | ((color & green_mask) >> green_right << green_left) << 8
		  | (color & blue_mask) >> blue_right << blue_left;
	      }

	    if (remainder)
	      src->seek (src, 4 - remainder, SEEK_CUR);

	    y += direction;
	  }
      }
      break;

    case 24:
      {
	uint8_t red, green, blue;

	image = avt_new_graphic (width, height);
	if (not image)
	  goto done;

	int remainder = (3 * width) % 4;

	while (y >= 0 and y < height)
	  {
	    avt_color *p = image->pixels + y * image->width;

	    for (int x = 0; x < width; x++, p++)
	      {
		blue = avt_data_read8 (src);
		green = avt_data_read8 (src);
		red = avt_data_read8 (src);
		*p = (red << 16) | (green << 8) | blue;
	      }

	    if (remainder)
	      src->seek (src, 4 - remainder, SEEK_CUR);

	    y += direction;
	  }
      }
      break;

    case 32:
      {
	short red_shift, green_shift, blue_shift;

	if (compression != 3)
	  {
	    red_mask = 0x00FF0000;
	    green_mask = 0x0000FF00;
	    blue_mask = 0x000000FF;
	  }

	red_shift = get_right_shift (red_mask);
	green_shift = get_right_shift (green_mask);
	blue_shift = get_right_shift (blue_mask);

	image = avt_new_graphic (width, height);
	if (not image)
	  goto done;

	while (y >= 0 and y < height)
	  {
	    avt_color *p = image->pixels + y * image->width;

	    for (int x = 0; x < width; x++, p++)
	      {
		register uint32_t color = src->read32 (src);
		*p = ((color & red_mask) >> red_shift) << 16
		  | ((color & green_mask) >> green_shift) << 8
		  | (color & blue_mask) >> blue_shift;
	      }

	    y += direction;
	  }
      }
      break;
    }

done:
  if (not image)
    src->seek (src, start, SEEK_SET);

  return image;
}
