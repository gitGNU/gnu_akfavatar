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

#ifndef AVTGRAPHIC_H
#define AVTGRAPHIC_H

#include "avtdata.h"
#include <stdint.h>
#include <stdbool.h>

/* special value for the color_key */
#define AVT_TRANSPARENT  0xFFFFFFFF

/* border width for 3d bars */
#define BORDER_3D_WIDTH 3
#define BORDER_3D_INTENSITY 0x37

/* avtgraphic.c */
typedef uint_least32_t avt_color;

typedef struct avt_graphic
{
  short width, height;
  bool transparent;
  bool free_pixels;
  avt_color color_key;
  avt_color *pixels;
} avt_graphic;

avt_graphic *avt_new_graphic (short width, short height);

avt_graphic *avt_data_to_graphic (void *data, short width, short height);

void avt_free_graphic (avt_graphic *gr);

void avt_bar (avt_graphic *gr, int x, int y, int width, int height, 
              avt_color color);

void avt_put_graphic (avt_graphic *source, avt_graphic *destination,
                      int x, int y);

void avt_graphic_segment (avt_graphic *source, int xoffset, int yoffset,
                          int width, int height, avt_graphic *destination,
                          int x, int y);

avt_graphic *avt_copy_graphic (avt_graphic *gr);

// saves the area into a new graphic
// the result should be freed with avt_free_graphic
avt_graphic *avt_get_area (avt_graphic * gr, int x, int y,
                           int width, int height);

void avt_bar3d (avt_graphic *s, int x, int y, int width, int height,
                avt_color color, bool pressed);

void avt_fill (avt_graphic *gr, avt_color color);

void avt_darker_area (avt_graphic *gr, int x, int y,
                      int width, int height, int amount);

void avt_brighter_area (avt_graphic *gr, int x, int y,
                        int width, int height, int amount);

/* inline functions */

/* return a brighter color */
/* returns the pixel position, no checks */
/* INSECURE */
static inline avt_color *
avt_pixel (avt_graphic *s, int x, int y)
{
  return s->pixels + y * s->width + x;
}

/*
 * the color_key is a color, which should be transparent
 * it can be AVT_TRANSPARENT, which doesn't conflict with real colors
 */
static inline void
avt_set_color_key (avt_graphic * gr, avt_color color_key)
{
  if (gr)
    {
      gr->color_key = color_key;
      gr->transparent = true;
    }
}


/* avtxbm.c */
void avt_put_image_xbm (avt_graphic *gr, short x, short y,
                        const unsigned char *bits, int width, int height,
                        avt_color color);

void avt_put_image_xbm_part (avt_graphic *gr, short x, short y,
                             short y_offset,
                             const unsigned char *bits, int width, int height,
                             avt_color color);

avt_graphic *avt_load_image_xbm (const unsigned char *bits,
                                 int width, int height, avt_color color);

avt_graphic *avt_load_image_xbm_data (avt_data *src, avt_color color);

/* avtxpm.c */
avt_graphic *avt_load_image_xpm (char **xpm);

avt_graphic *avt_load_image_xpm_data (avt_data *src);

/* avtbmp.c */
avt_graphic *avt_load_image_bmp_data (avt_data *src);

#endif
