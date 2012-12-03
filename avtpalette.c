/*
 * access to a palette of color definitions
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
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

#include "akfavatar.h"
#include "avtinternals.h"
#include "rgb.h"
#include <stdlib.h>
#include <strings.h>		// strcasecmp
#include <iso646.h>

extern int
avt_colorname (const char *name)
{
  int colornr;

  if (not name or not * name)
    return -1;

  colornr = -1;

  // skip space
  while (avt_isblank (*name))
    name++;

  if (name[0] == '#')		// hexadecimal values
    {
      char *p;

      colornr = strtol (name + 1, &p, 16);

      // only 3 hex digits?
      if (p - name <= 4)
	{
	  unsigned int r, g, b;

	  r = (colornr >> 8) bitand 0xF;
	  g = (colornr >> 4) bitand 0xF;
	  b = colornr bitand 0xF;

	  colornr = avt_rgb ((r << 4 | r), (g << 4 | g), (b << 4 | b));
	}
    }
  else if (name[0] == '%')	// HSV values not supported
    colornr = -1;
  else				// look up color table
    {
      int i;
      const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

      for (i = 0; i < numcolors and colornr == -1; i++)
	if (strcasecmp (avt_colors[i].name, name) == 0)
	  colornr = avt_colors[i].number;
    }

  return colornr;
}

extern const char *
avt_palette (int entry, int *colornr)
{
  const char *name = NULL;
  const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

  if (entry >= 0 and entry < numcolors)
    {
      name = avt_colors[entry].name;

      if (colornr)
	*colornr = avt_colors[entry].number;
    }

  return name;
}
