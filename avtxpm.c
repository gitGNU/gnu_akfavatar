/*
 * X-Pixmap (XPM) support for AKFAvatar
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <iso646.h>

#define avt_isblank(c)  ((c) == ' ' or (c) == '\t')

// number of printable ASCII codes
#define XPM_NR_CODES (126 - 32 + 1)

// for xpm codes
union xpm_codes
{
  uint_least32_t nr;
  union xpm_codes *next;
};

static void
avt_free_xpm_tree (union xpm_codes *tree, int depth, int cpp)
{
  union xpm_codes *e;

  if (depth < cpp)
    {
      for (int i = 0; i < XPM_NR_CODES; i++)
	{
	  e = (tree + i)->next;
	  if (e)
	    avt_free_xpm_tree (e, depth + 1, cpp);
	}
    }

  free (tree);
}

extern avt_graphic *
avt_load_image_xpm (char **xpm)
{
  avt_graphic *img;
  int width, height, ncolors, cpp;
  union xpm_codes *codes;
  avt_color *colors;
  int code_nr;

  cpp = 0;
  codes = NULL;
  colors = NULL;
  img = NULL;

  // check if we actually have data to process
  if (not xpm or not * xpm)
    goto done;

  /*
   * read value line
   * there may be more values in the line, but we just
   * need the first four
   */
  {
    char *p = xpm[0];
    width = strtol (p, &p, 10);
    height = strtol (p, &p, 10);
    ncolors = strtol (p, &p, 10);	// number of colors
    cpp = strtol (p, &p, 10);	// characters per pixel
  }

  // check values and limit sizes to avoid exessive memory usage
  if (width < 1 or height < 1 or ncolors < 1 or cpp < 1
      or ncolors > 0xFFFFFF or cpp > 4 or width > 10000 or height > 10000)
    goto done;

  // create target surface
  img = avt_new_graphic (width, height);

  if (not img)
    goto done;

  // get memory for codes table
  if (cpp > 1)
    {
      codes = (union xpm_codes *) calloc (XPM_NR_CODES, sizeof (*codes));
      if (not codes)
	{
	  avt_free_graphic (img);
	  img = NULL;
	  goto done;
	}
    }

  // get memory for colors table (palette)
  // note: for 1 character per pixel we use the character-32 as index
  if (cpp == 1)
    colors = (avt_color *) malloc (XPM_NR_CODES * sizeof (avt_color));
  else
    colors = (avt_color *) malloc (ncolors * sizeof (avt_color));

  if (not colors)
    {
      avt_free_graphic (img);
      img = NULL;
      goto done;
    }

  code_nr = 0;

  // process colors
  for (int colornr = 1; colornr <= ncolors; colornr++, code_nr++)
    {
      char *p;			// pointer for scanning through the string

      if (xpm[colornr] == NULL)
	{
	  avt_free_graphic (img);
	  img = NULL;
	  goto done;
	}

      /*
       * if there is only one character per pixel,
       * the character is the palette number (simpler)
       */
      if (cpp == 1)
	code_nr = xpm[colornr][0] - 32;
      else			// store characters in codes table
	{
	  char c;
	  union xpm_codes *table;

	  c = '\0';
	  table = codes;
	  for (int i = 0; i < cpp - 1; i++)
	    {
	      c = xpm[colornr][i];

	      if (c < 32 or c > 126)
		break;

	      table = (table + (c - 32));

	      if (not table->next)
		table->next =
		  (union xpm_codes *) calloc (XPM_NR_CODES, sizeof (*codes));

	      table = table->next;
	    }

	  if (c < 32 or c > 126)
	    break;

	  c = xpm[colornr][cpp - 1];

	  if (c < 32 or c > 126)
	    break;

	  (table + (c - 32))->nr = colornr - 1;
	}

      // scan for color definition
      p = &xpm[colornr][cpp];	// skip color-characters
      while (*p and (*p != 'c' or not avt_isblank (*(p + 1))
		     or not avt_isblank (*(p - 1))))
	p++;

      // no color definition found? search for grayscale definition
      if (not * p)
	{
	  p = &xpm[colornr][cpp];	// skip color-characters
	  while (*p and (*p != 'g' or not avt_isblank (*(p + 1))
			 or not avt_isblank (*(p - 1))))
	    p++;
	}

      // no grayscale definition found? search for g4 definition
      if (not * p)
	{
	  p = &xpm[colornr][cpp];	// skip color-characters
	  while (*p
		 and (*p != '4' or * (p - 1) != 'g'
		      or not avt_isblank (*(p + 1))
		      or not avt_isblank (*(p - 2))))
	    p++;
	}

      // search for monochrome definition
      if (not * p)
	{
	  p = &xpm[colornr][cpp];	// skip color-characters
	  while (*p and (*p != 'm' or not avt_isblank (*(p + 1))
			 or not avt_isblank (*(p - 1))))
	    p++;
	}

      if (*p)
	{
	  int colornr;
	  size_t color_name_pos;
	  char color_name[80];

	  // skip to color name/definition
	  p++;
	  while (*p and avt_isblank (*p))
	    p++;

	  // copy colorname up to next space
	  color_name_pos = 0;
	  while (*p and not avt_isblank (*p)
		 and color_name_pos < sizeof (color_name) - 1)
	    color_name[color_name_pos++] = *p++;
	  color_name[color_name_pos] = '\0';

	  colornr = 0x000000;	// black

	  if (color_name[0] == '#')
	    colornr = strtol (&color_name[1], NULL, 16);
	  else if (strcasecmp (color_name, "None") == 0)
	    {
	      colornr = AVT_TRANSPARENT;
	      avt_set_color_key (img, colornr);
	    }
	  else if (strcasecmp (color_name, "black") == 0)
	    colornr = 0x000000;
	  else if (strcasecmp (color_name, "white") == 0)
	    colornr = 0xFFFFFF;

	  /*
	   * Note: don't use avt_colorname,
	   * or the palette is always needed
	   */

	  colors[code_nr] = colornr;
	}
    }

  // process pixeldata
  if (cpp == 1)			// the easiest case
    {
      avt_color *pix;
      uint_least8_t *xpm_data;

      for (int line = 0; line < height; line++)
	{
	  // point to beginning of the line
	  pix = avt_pixel (img, 0, line);
	  xpm_data = (uint_least8_t *) xpm[ncolors + 1 + line];

	  for (int pos = width; pos > 0; pos--, pix++, xpm_data++)
	    *pix = colors[*xpm_data - 32];
	}
    }
  else				// cpp != 1
    {
      avt_color *pix;
      uint_least8_t *xpm_line;

      for (int line = 0; line < height; line++)
	{
	  // point to beginning of the line
	  pix = avt_pixel (img, 0, line);
	  xpm_line = (uint_least8_t *) xpm[ncolors + 1 + line];

	  for (int pos = 0; pos < width; pos++, pix++)
	    {
	      union xpm_codes *table;
	      uint_least8_t c;

	      c = '\0';
	      // find code in codes table
	      table = codes;
	      for (int i = 0; i < cpp - 1; i++)
		{
		  c = xpm_line[pos * cpp + i];
		  if (c < 32 or c > 126)
		    break;
		  table = (table + (c - 32))->next;
		}

	      if (c < 32 or c > 126)
		break;

	      c = xpm_line[pos * cpp + cpp - 1];
	      if (c < 32 or c > 126)
		break;

	      code_nr = (table + (c - 32))->nr;

	      *pix = colors[code_nr];
	    }
	}
    }

done:
  if (colors)
    free (colors);

  // clean up codes table
  if (codes)
    avt_free_xpm_tree (codes, 1, cpp);

  return img;
}

extern avt_graphic *
avt_load_image_xpm_data (avt_data * src)
{
  int start;
  char head[9];
  char **xpm;
  char *line;
  unsigned int linepos, linenr, linecount, linecapacity;
  avt_graphic *img;
  char c;
  bool end, error;

  if (not src)
    return NULL;

  img = NULL;
  xpm = NULL;
  line = NULL;
  end = error = false;

  start = src->tell (src);

  // check if it has an XPM header
  if (src->read (src, head, sizeof (head), 1) < 1
      or memcmp (head, "/* XPM */", 9) != 0)
    {
      src->seek (src, start, SEEK_SET);

      return NULL;
    }

  linenr = linepos = 0;

  linecapacity = 100;
  line = (char *) malloc (linecapacity);
  if (not line)
    return NULL;

  linecount = 512;		// can be extended later
  xpm = (char **) malloc (linecount * sizeof (*xpm));
  if (not xpm)
    error = end = true;

  while (not end)
    {
      // skip to next quote
      do
	{
	  if (src->read (src, &c, sizeof (c), 1) < 1)
	    end = true;
	}
      while (not end and c != '"');

      // read line
      linepos = 0;
      c = '\0';
      while (not end and c != '"')
	{
	  if (src->read (src, &c, sizeof (c), 1) < 1)
	    error = end = true;	// shouldn't happen here

	  if (c != '"')
	    line[linepos++] = c;

	  if (linepos >= linecapacity)
	    {
	      register char *new_line;
	      linecapacity += 100;
	      new_line = (char *) realloc (line, linecapacity);
	      if (new_line)
		line = new_line;
	      else
		error = end = true;
	    }
	}

      // copy line
      if (not end)
	{
	  line[linepos++] = '\0';
	  xpm[linenr] = (char *) malloc (linepos);
	  memcpy (xpm[linenr], line, linepos);
	  linenr++;
	  if (linenr >= linecount)	// leave one line reserved
	    {
	      register char **new_xpm;
	      linecount += 512;
	      new_xpm = (char **) realloc (xpm, linecount * sizeof (*xpm));
	      if (new_xpm)
		xpm = new_xpm;
	      else
		error = end = true;
	    }
	}
    }

  /* last line must be NULL,
   * so premature end of data can be detected later
   */
  if (xpm)
    xpm[linenr] = NULL;

  if (not error)
    img = avt_load_image_xpm (xpm);

  // free xpm
  if (xpm)
    {
      // linenr points to next (uninitialized) line
      for (unsigned int i = 0; i < linenr; i++)
	free (xpm[i]);

      free (xpm);
    }

  if (line)
    free (line);

  return img;
}
