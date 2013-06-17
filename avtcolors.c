/*
 * color functions for AKFAvatar
 * Copyright (c) 2013 Andreas K. Foerster <info@akfoerster.de>
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

#include "akfavatar.h"
#include <iso646.h>

#define avt_max3(a,b,c)  ((a)>(b)?((a)>(c)?(a):(c)):(b)>(c)?(b):(c))

// returns the maximum color value (red, green or blue)
extern int
avt_brightness (int color)
{
  register int red, green, blue;

  red = avt_red (color);
  green = avt_green (color);
  blue = avt_blue (color);

  return avt_max3 (red, green, blue);
}


extern int
avt_darker (int color, int amount)
{
  register int red, green, blue;

  red = avt_red (color);
  green = avt_green (color);
  blue = avt_blue (color);

  red = red > amount ? red - amount : 0;
  green = green > amount ? green - amount : 0;
  blue = blue > amount ? blue - amount : 0;

  return avt_rgb (red, green, blue);
}


extern int
avt_brighter (int color, int amount)
{
  register int red, green, blue;

  red = avt_red (color) + amount;
  green = avt_green (color) + amount;
  blue = avt_blue (color) + amount;

  if (red > 0xFF)
    red = 0xFF;

  if (green > 0xFF)
    green = 0xFF;

  if (blue > 0xFF)
    blue = 0xFF;

  return avt_rgb(red, green, blue);
}
