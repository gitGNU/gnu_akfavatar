#include "akfavatar.h"
#include "avtinternals.h"
#include "stdio.h"		// sscanf
#include "strings.h"		// strcasecmp
#include "rgb.h"
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
      unsigned int r, g, b;

      if (sscanf (name, " #%2x%2x%2x", &r, &g, &b) == 3)
	colornr = avt_rgb (r, g, b);
      else if (sscanf (name, " #%1x%1x%1x", &r, &g, &b) == 3)
	colornr = avt_rgb ((r << 4 | r), (g << 4 | g), (b << 4 | b));
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


#ifndef DISABLE_DEPRECATED

extern int
avt_name_to_color (const char *name, int *red, int *green, int *blue)
{
  int status;
  int colornr;

  if (not name or not * name or not red or not green or not blue)
    return -1;

  colornr = avt_colorname (name);

  if (colornr < 0)
    {
      status = -1;
      *red = *green = *blue = -1;
    }
  else
    {
      status = 0;
      *red = avt_red (colornr);
      *green = avt_green (colornr);
      *blue = avt_blue (colornr);
    }

  return status;
}

extern const char *
avt_get_color_name (int entry)
{
  const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

  if (entry >= 0 and entry < numcolors)
    return avt_colors[entry].name;
  else
    return NULL;
}

extern const char *
avt_get_color (int entry, int *red, int *green, int *blue)
{
  const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

  if (entry >= 0 and entry < numcolors)
    {
      int number = avt_colors[entry].number;
      if (red)
	*red = avt_red (number);
      if (green)
	*green = avt_green (number);
      if (blue)
	*blue = avt_blue (number);

      return avt_colors[entry].name;
    }
  else
    return NULL;
}

extern void
avt_set_balloon_color_name (const char *name)
{
  int c = avt_colorname (name);

  if (c >= 0)
    avt_set_balloon_color (c);
}

extern void
avt_set_background_color_name (const char *name)
{
  int c = avt_colorname (name);

  if (c >= 0)
    avt_set_background_color (c);
}

extern void
avt_set_text_color_name (const char *name)
{
  int c = avt_colorname (name);

  if (c >= 0)
    avt_set_text_color (c);
}

extern void
avt_set_text_background_color_name (const char *name)
{
  int c = avt_colorname (name);

  if (c >= 0)
    avt_set_text_background_color (c);
}

#endif // DISABLE_DEPRECATED
