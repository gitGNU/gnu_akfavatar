/*
 * graphic functions for AKFAvatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
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

#include "avtgraphic.h"

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <iso646.h>

extern void
avt_free_graphic (avt_graphic * gr)
{
  if (gr)
    {
      if (gr->free_pixels)
	free (gr->pixels);

      free (gr);
    }
}

// use data for pixels
// data may olny be freed after avt_free_graphic is called on this
extern avt_graphic *
avt_data_to_graphic (void *data, short width, short height)
{
  avt_graphic *gr;

  if (width <= 0 or height <= 0)
    return NULL;

  // data may be NULL (see avt_new_graphic)

  gr = (avt_graphic *) malloc (sizeof (*gr));

  if (gr)
    {
      gr->width = width;
      gr->height = height;
      gr->transparent = false;
      gr->free_pixels = false;
      gr->color_key = AVT_TRANSPARENT;
      gr->pixels = (avt_color *) data;
    }

  return gr;
}

extern avt_graphic *
avt_new_graphic (short width, short height)
{
  avt_graphic *gr;

  gr = avt_data_to_graphic (NULL, width, height);

  if (gr)
    {
      gr->pixels = (avt_color *) malloc (width * height * sizeof (avt_color));

      if (not gr->pixels)
	{
	  avt_free_graphic (gr);
	  return NULL;
	}

      gr->free_pixels = true;
    }

  return gr;
}

extern avt_graphic *
avt_copy_graphic (avt_graphic * gr)
{
  avt_graphic *result;

  if (not gr)
    return NULL;

  result = avt_new_graphic (gr->width, gr->height);

  if (result)
    {
      memcpy (result->pixels, gr->pixels,
	      gr->width * gr->height * sizeof (avt_color));

      result->color_key = gr->color_key;
      result->transparent = gr->transparent;
    }

  return result;
}

// secure
extern void
avt_bar (avt_graphic * gr, int x, int y, int width, int height,
	 avt_color color)
{
  if (x > gr->width or y > gr->height)
    return;

  if (x < 0)
    {
      width += x;
      x = 0;
    }

  if (y < 0)
    {
      height += y;
      y = 0;
    }

  if (x + width > gr->width)
    width = gr->width - x;

  if (y + height > gr->height)
    height = gr->height - y;

  if (width <= 0 or height <= 0)
    return;

  for (int ny = 0; ny < height; ny++)
    {
      avt_color *p;

      p = avt_pixel (gr, x, y + ny);

      for (int nx = width; nx > 0; nx--, p++)
	*p = color;
    }
}

// secure
extern void
avt_fill (avt_graphic * gr, avt_color color)
{
  avt_color *p;

  p = gr->pixels;
  for (int i = (gr->width * gr->height); i > 0; --i, p++)
    *p = color;
}

// secure
static void
avt_horizontal_line (avt_graphic * gr, int x1, int x2, int y1,
		     avt_color color)
{
  if (x1 > gr->width)
    return;

  if (x1 < 0)
    x1 = 0;

  if (x2 >= gr->width)
    x2 = gr->width - 1;

  avt_color *p;
  p = avt_pixel (gr, x1, y1);

  for (int nx = x1; nx <= x2; ++nx, ++p)
    *p = color;
}

// secure
static void
avt_vertical_line (avt_graphic * gr, int x1, int y1, int y2, avt_color color)
{
  if (y1 >= gr->height)
    return;

  if (y1 < 0)
    y1 = 0;

  if (y2 >= gr->height)
    y2 = gr->height - 1;

  for (int ny = y1; ny <= y2; ny++)
    *avt_pixel (gr, x1, ny) = color;
}

// border with 3d effect
static void
avt_border3d (avt_graphic * gr, int x, int y, int width, int height,
	      avt_color color, bool pressed)
{
  avt_color c1, c2;
  int x2, y2;

  if (not pressed)
    {
      c1 = avt_brighter (color, BORDER_3D_INTENSITY);
      c2 = avt_darker (color, BORDER_3D_INTENSITY);
    }
  else				// pressed
    {
      c1 = avt_darker (color, BORDER_3D_INTENSITY);
      c2 = avt_brighter (color, BORDER_3D_INTENSITY);
    }

  x2 = x + width - 1;
  y2 = y + height - 1;

  for (int i = 0; i < BORDER_3D_WIDTH; ++i)
    {
      // lower right
      avt_horizontal_line (gr, x + i, x2 - i, y + height - 1 - i, c2);
      avt_vertical_line (gr, x + width - 1 - i, y + i, y2 - i, c2);

      // upper left
      // defined later, so it's dominant when overlapping
      avt_horizontal_line (gr, x + i, x2 - i, y + i, c1);
      avt_vertical_line (gr, x + i, y + i, y2 - i, c1);
    }
}

// like avt_bar bit with 3d effect
extern void
avt_bar3d (avt_graphic * gr, int x, int y, int width, int height,
	   avt_color color, bool pressed)
{
  avt_bar (gr, x + BORDER_3D_WIDTH, y + BORDER_3D_WIDTH,
	   width - (2 * BORDER_3D_WIDTH), height - (2 * BORDER_3D_WIDTH),
	   color);

  avt_border3d (gr, x, y, width, height, color, pressed);
}

// move a line up or down
static inline void
avt_line_move (avt_color * d, avt_color * s, int width, avt_color color_key)
{
  if (s > d)
    {
      for (int i = width; i > 0; i--, s++, d++)
	if (*s != color_key)
	  *d = *s;
    }
  else
    {
      s += width - 1;
      d += width - 1;
      for (int i = width; i > 0; i--, s--, d--)
	if (*s != color_key)
	  *d = *s;
    }
}

extern void
avt_graphic_segment (avt_graphic * source, int xoffset, int yoffset,
		     int width, int height, avt_graphic * destination,
		     int x, int y)
{
  if (not source or not destination
      or x > destination->width or y > destination->height)
    return;

  if (width > source->width)
    width = source->width;

  if (height > source->height)
    height = source->height;

  if (y < 0)
    {
      yoffset += (-y);
      height -= (-y);
      y = 0;
    }

  if (x < 0)
    {
      xoffset += (-x);
      width -= (-x);
      x = 0;
    }

  if (x + width > destination->width)
    width = destination->width - x;

  if (y + height > destination->height)
    height = destination->height - y;

  if (width <= 0 or height <= 0)
    return;

  bool opaque = not source->transparent;

  // overlap allowed, so we must take care about the direction we go

  if (yoffset >= y)
    {
      for (int line = 0; line < height; line++)
	{
	  avt_color *s, *d;

	  s = avt_pixel (source, xoffset, line + yoffset);
	  d = avt_pixel (destination, x, y + line);

	  if (opaque)
	    memmove (d, s, width * sizeof (avt_color));
	  else			// transparent
	    avt_line_move (d, s, width, source->color_key);
	}
    }
  else				// yoffset < y
    {
      for (int line = height - 1; line >= 0; line--)
	{
	  avt_color *s, *d;

	  s = avt_pixel (source, xoffset, line + yoffset);
	  d = avt_pixel (destination, x, y + line);

	  if (opaque)
	    memmove (d, s, width * sizeof (avt_color));
	  else			// transparent
	    avt_line_move (d, s, width, source->color_key);
	}
    }
}

extern void
avt_put_graphic (avt_graphic * source, avt_graphic * destination,
		 int x, int y)
{
  avt_graphic_segment (source, 0, 0, INT_MAX, INT_MAX, destination, x, y);
}

// saves the area into a new graphic
// the result should be freed with avt_free_graphic
extern avt_graphic *
avt_get_area (avt_graphic * gr, int x, int y, int width, int height)
{
  avt_graphic *result;

  result = avt_new_graphic (width, height);

  if (result)
    avt_graphic_segment (gr, x, y, width, height, result, 0, 0);

  return result;
}


// secure
extern void
avt_darker_area (avt_graphic * gr, int x, int y, int width, int height,
		 int amount)
{
  if (x > gr->width or y > gr->height)
    return;

  if (x < 0)
    {
      width -= (-x);
      x = 0;
    }

  if (y < 0)
    {
      height -= (-y);
      y = 0;
    }

  if (x + width > gr->width)
    width = gr->width - x;

  if (y + height > gr->height)
    height = gr->height - y;

  if (width <= 0 or height <= 0)
    return;

  for (int dy = height - 1; dy >= 0; dy--)
    {
      avt_color *p = avt_pixel (gr, x, y + dy);

      for (int dx = width - 1; dx >= 0; dx--, p++)
	*p = avt_darker (*p, amount);
    }
}
