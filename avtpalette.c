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
