/*
 * X-Bitmap (XBM) support for AKFAvatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
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
#include <string.h>
#include <stdbool.h>
#include <iso646.h>

static inline int
avt_xbm_bytes_per_line (int width)
{
  return (width + 7) / 8;
}

extern void
avt_put_image_xbm (avt_graphic * gr, short x, short y,
		   const unsigned char *bits, int width, int height,
		   avt_color color)
{
  // if it doesn't fit horizontally, display nothing
  // use avt_load_image_xbm to get horizontal clipping
  if (x < 0 or x + width > gr->width)
    return;

  if (y < 0)
    {
      bits += (-y) * avt_xbm_bytes_per_line (width);
      height -= (-y);
      y = 0;
    }

  if (y + height > gr->height)
    height = gr->height - y;

  if (width <= 0 or height <= 0)
    return;

  for (int dy = 0; dy < height; dy++)
    {
      int dx = 0;
      avt_color *p = avt_pixel (gr, x, y + dy);

      while (dx < width)
	{
	  for (int bit = 1; bit <= 0x80 and dx < width; bit <<= 1, dx++, p++)
	    if (*bits bitand bit)
	      *p = color;

	  bits++;
	}
    }
}

extern void
avt_put_image_xbm_part (avt_graphic * gr, short x, short y, short y_offset,
			const unsigned char *bits, int width, int height,
			avt_color color)
{
  bits += avt_xbm_bytes_per_line (width) * y_offset;
  height -= y_offset;

  avt_put_image_xbm (gr, x, y, bits, width, height, color);
}

/*
 * loads an X-Bitmap (XBM) with a given color as foreground
 * and a transarent background
 */
extern avt_graphic *
avt_load_image_xbm (const unsigned char *bits, int width, int height,
		    avt_color color)
{
  avt_graphic *image;

  image = avt_new_graphic (width, height);

  if (image)
    {
      avt_fill (image, AVT_TRANSPARENT);
      avt_set_color_key (image, AVT_TRANSPARENT);
      avt_put_image_xbm (image, 0, 0, bits, width, height, color);
    }

  return image;
}

extern avt_graphic *
avt_load_image_xbm_data (avt_data * src, avt_color color)
{
  unsigned char *bits;
  int width, height;
  int start;
  unsigned int bytes, bmpos;
  char line[1024];
  avt_graphic *img;
  bool end, error;
  bool X10;

  if (not src)
    return NULL;

  img = NULL;
  bits = NULL;
  X10 = false;
  end = error = false;
  width = height = bytes = bmpos = 0;

  start = src->tell (src);

  // check if it starts with #define
  if (src->read (src, line, 1, sizeof (line) - 1) < 1
      or memcmp (line, "#define", 7) != 0)
    {
      src->seek (src, start, SEEK_SET);

      return NULL;
    }

  // make it usable as a string
  line[sizeof (line) - 1] = '\0';

  // search for width and height
  char *p;
  p = strstr (line, "_width ");
  if (p)
    width = strtol (p + 7, NULL, 0);
  else
    error = end = true;

  p = strstr (line, "_height ");
  if (p)
    height = strtol (p + 8, NULL, 0);
  else
    error = end = true;

  if (strstr (line, " short ") != NULL)
    X10 = true;

  if (error)
    goto done;

  if (width and height)
    {
      bytes = avt_xbm_bytes_per_line (width) * height;
      // one byte larger for safety with old X10 format
      bits = (unsigned char *) malloc (bytes + 1);
    }

  // this catches different errors
  if (not bits)
    {
      error = end = true;
      goto done;
    }

  // search start of bitmap part
  if (not end and not error)
    {
      char c;

      src->seek (src, start, SEEK_SET);

      do
	{
	  if (src->read (src, &c, sizeof (c), 1) < 1)
	    error = end = true;
	}
      while (c != '{' and not error);

      if (error)		// no '{' found
	goto done;

      // skip newline
      src->read (src, &c, sizeof (c), 1);
    }

  while (not end and not error)
    {
      char c;
      unsigned int linepos;

      // read line
      linepos = 0;
      c = '\0';
      while (not end and linepos < sizeof (line) and c != '\n')
	{
	  if (src->read (src, &c, sizeof (c), 1) < 1)
	    error = end = true;

	  if (c != '\n' and c != '}')
	    line[linepos++] = c;

	  if (c == '}')
	    end = true;
	}
      line[linepos] = '\0';

      // parse line
      if (line[0] != '\0')
	{
	  char *p;
	  char *endptr;
	  long value;
	  bool end_of_line;

	  p = line;
	  end_of_line = false;
	  while (not end_of_line and bmpos < bytes)
	    {
	      value = strtol (p, &endptr, 0);
	      if (endptr == p)
		end_of_line = true;
	      else
		{
		  if (not X10)
		    bits[bmpos++] = value;
		  else		// X10
		    {
		      unsigned short *v;
		      // image is assumed to be in native endianess
		      v = (unsigned short *) (bits + bmpos);
		      *v = value;
		      bmpos += sizeof (*v);
		    }

		  p = endptr + 1;	// skip comma
		}
	    }
	}
    }

  if (not error)
    img = avt_load_image_xbm (bits, width, height, color);

done:
  // free bits
  if (bits)
    free (bits);

  if (error)
    src->seek (src, start, SEEK_SET);

  return img;
}
