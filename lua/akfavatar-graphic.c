/*
 * AKFAvatar graphic API
 * Copyright (c) 2011 Andreas K. Foerster <info@akfoerster.de>
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


/* ATTENTION: coordinates are 1-based externally, but internally 0-based */

#include "akfavatar.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef unsigned char uchar;

struct color
{
  uchar red, green, blue;
};

typedef struct graphic
{
  int width, height;
  int thickness;		/* thickness of pen */
  int htextalign, vtextalign;	/* alignment for text */
  double penx, peny;		/* position of pen */
  double heading;		/* heading of the turtle */
  struct color color;		/* drawing color */
  struct color background;	/* background color */
  struct color data[1];
} graphic;

/* Bytes per pixel (3=RGB) */
#define BPP 3

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

/* convert degree to radians */
#define RAD(x)  ((x) * M_PI / 180.0)

#define GRAPHICDATA "AKFAvatar-graphic"

#define get_graphic(L, idx) \
   ((graphic *) luaL_checkudata ((L), (idx), GRAPHICDATA))

#define graphic_bytes(width, height) \
  sizeof(graphic)-sizeof(struct color)+(width)*(height)*BPP

#define new_graphic(L, nbytes) \
  (graphic *) lua_newuserdata ((L), (nbytes)); \
  luaL_getmetatable ((L), GRAPHICDATA); \
  lua_setmetatable ((L), -2)

/* force value to be in range */
#define RANGE(v, min, max)  ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))

#define visible_x(gr, x)  ((x) >= 0 && (x) < (gr)->width)
#define visible_y(gr, y)  ((y) >= 0 && (y) < (gr)->height)
#define visible(gr, x, y)  (visible_x(gr, x) && visible_y(gr, y))

/* set pen position */
#define penpos(gr, x, y)  (gr)->penx = (x); (gr)->peny = (y)
#define center(gr) \
  do { \
    (gr)->penx = ((double) (gr)->width) / 2.0 - 1.0; \
    (gr)->peny = ((double) (gr)->height) / 2.0 - 1.0; \
  } while(0)

/* fast putpixel with color, no check */
#define putpixelcolor(gr, x, y, width, col) \
  *((gr)->data + ((int)(y) * (width)) + (int)(x)) = (col)

/* fast putpixel, no check */
#define putpixel(gr, x, y) \
  putpixelcolor ((gr), (x), (y), (gr)->width, (gr)->color)

#define HA_LEFT 0
#define HA_CENTER 1
#define HA_RIGHT 2
#define VA_TOP 0
#define VA_CENTER 1
#define VA_BOTTOM 2


static void
clear_graphic (graphic * gr)
{
  struct color *p;
  size_t i, pixels;
  struct color color;

  color = gr->background;
  pixels = gr->width * gr->height;
  p = gr->data;

  for (i = 0; i < pixels; i++)
    *p++ = color;
}


static void
bar (graphic * gr, int x1, int y1, int x2, int y2)
{
  int x, y, width, height;
  struct color *data, *p;
  struct color color;

  width = gr->width;
  height = gr->height;
  data = gr->data;
  color = gr->color;

  /* sanitize values */
  x1 = RANGE (x1, 0, width - 1);
  y1 = RANGE (y1, 0, height - 1);
  x2 = RANGE (x2, 0, width - 1);
  y2 = RANGE (y2, 0, height - 1);


  for (y = y1; y <= y2; y++)
    {
      p = data + (y * width) + x1;

      for (x = x1; x <= x2; x++)
	*p++ = color;
    }
}


#define putdot(gr, x, y) \
  do { \
    int s = (gr)->thickness; \
    bar ((gr), ((int) (x))-s, ((int)(y))-s, ((int)(x))+s, ((int)(y))+s); \
  } while(0)


/* local gr, width, height = graphic.new([width, height]) */
static int
lgraphic_new (lua_State * L)
{
  int width, height;
  int red, green, blue;
  graphic *gr;

  width = luaL_optint (L, 1, avt_image_max_width ());
  height = luaL_optint (L, 2, avt_image_max_height ());

  gr = new_graphic (L, graphic_bytes (width, height));
  gr->width = width;
  gr->height = height;

  /* black color */
  gr->color.red = gr->color.green = gr->color.blue = 0;

  /* pen in center */
  center (gr);
  gr->thickness = 1 - 1;

  gr->htextalign = HA_CENTER;
  gr->vtextalign = VA_CENTER;

  gr->heading = 0.0;

  avt_get_background_color (&red, &green, &blue);
  gr->background.red = (uchar) red;
  gr->background.green = (uchar) green;
  gr->background.blue = (uchar) blue;
  clear_graphic (gr);

  lua_pushinteger (L, width);
  lua_pushinteger (L, height);

  return 3;
}


static int
lgraphic_fullsize (lua_State * L)
{
  lua_pushinteger (L, avt_image_max_width ());
  lua_pushinteger (L, avt_image_max_height ());

  return 2;
}


static int
lgraphic_clear (lua_State * L)
{
  graphic *gr;
  int red, green, blue;
  const char *color_name;

  gr = get_graphic (L, 1);
  color_name = lua_tostring (L, 2);

  if (color_name
      && avt_name_to_color (color_name, &red, &green, &blue) == AVT_NORMAL)
    {
      /* new background color */
      gr->background.red = (uchar) red;
      gr->background.green = (uchar) green;
      gr->background.blue = (uchar) blue;
    }

  clear_graphic (gr);

  return 0;
}


/* gr:color(colorname) */
static int
lgraphic_color (lua_State * L)
{
  graphic *gr;
  int red, green, blue;
  const char *name;

  name = luaL_checkstring (L, 2);
  if (avt_name_to_color (name, &red, &green, &blue) == AVT_NORMAL)
    {
      gr = get_graphic (L, 1);
      gr->color.red = (uchar) red;
      gr->color.green = (uchar) green;
      gr->color.blue = (uchar) blue;
    }

  return 0;
}


/* gr:rgb(red, green, blue) */
static int
lgraphic_rgb (lua_State * L)
{
  graphic *gr;
  int red, green, blue;

  red = luaL_checkint (L, 2);
  green = luaL_checkint (L, 3);
  blue = luaL_checkint (L, 4);

  luaL_argcheck (L, red >= 0 && red <= 255,
		 2, "value between 0 and 255 expected");

  luaL_argcheck (L, green >= 0 && green <= 255,
		 3, "value between 0 and 255 expected");

  luaL_argcheck (L, blue >= 0 && blue <= 255,
		 4, "value between 0 and 255 expected");

  gr = get_graphic (L, 1);
  gr->color.red = (uchar) red;
  gr->color.green = (uchar) green;
  gr->color.blue = (uchar) blue;

  return 0;
}


/* gr:eraser() */
static int
lgraphic_eraser (lua_State * L)
{
  graphic *gr;

  gr = get_graphic (L, 1);
  gr->color = gr->background;

  return 0;
}


/* gr:thickness(size) */
static int
lgraphic_thickness (lua_State * L)
{
  int s;

  s = luaL_checkint (L, 2) - 1;
  get_graphic (L, 1)->thickness = (s > 0) ? s : 0;

  return 0;
}


/* x, y = gr:pen_position () */
static int
lgraphic_pen_position (lua_State * L)
{
  graphic *gr = get_graphic (L, 1);

  lua_pushnumber (L, gr->penx + 1.0);
  lua_pushnumber (L, gr->peny + 1.0);

  return 2;
}


/* gr:moveto (x, y) */
static int
lgraphic_moveto (lua_State * L)
{
  graphic *gr = get_graphic (L, 1);

  /* a pen outside the field is allowed! */
  penpos (gr, luaL_checknumber (L, 2) - 1.0, luaL_checknumber (L, 3) - 1.0);

  return 0;
}


/* gr:moverel (x, y) */
static int
lgraphic_moverel (lua_State * L)
{
  graphic *gr = get_graphic (L, 1);

  penpos (gr,
	  gr->penx + luaL_checknumber (L, 2),
	  gr->peny + luaL_checknumber (L, 3));

  return 0;
}


/* gr:center () */
/* gr:home () */
static int
lgraphic_center (lua_State * L)
{
  graphic *gr = get_graphic (L, 1);

  center (gr);
  gr->heading = 0.0;

  return 0;
}


static void
vertical_line (graphic * gr, int x, int y1, int y2)
{
  int y;
  int height;

  if (visible_x (gr, x))
    {
      if (y1 > y2)		/* swap */
	{
	  int ty = y1;
	  y1 = y2;
	  y2 = ty;
	}

      height = gr->height;
      y1 = RANGE (y1, 0, height - 1);
      y2 = RANGE (y2, 0, height - 1);

      if (gr->thickness > 0)
	{
	  for (y = y1; y <= y2; y++)
	    putdot (gr, x, y);
	}
      else
	{
	  struct color color = gr->color;
	  int width = gr->width;

	  for (y = y1; y <= y2; y++)
	    if (visible_y (gr, y))
	      putpixelcolor (gr, x, y, width, color);
	}
    }
}


static void
horizontal_line (graphic * gr, int x1, int x2, int y)
{
  int x;
  int width;

  if (visible_y (gr, y))
    {
      if (x1 > x2)		/* swap */
	{
	  int tx = x1;
	  x1 = x2;
	  x2 = tx;
	}

      width = gr->width;
      x1 = RANGE (x1, 0, width - 1);
      x2 = RANGE (x2, 0, width - 1);

      if (gr->thickness > 0)
	{
	  for (x = x1; x <= x2; x++)
	    putdot (gr, x, y);
	}
      else
	{
	  struct color *p;
	  struct color color;

	  color = gr->color;
	  p = gr->data + (y * width) + x1;

	  for (x = x1; x <= x2; x++)
	    *p++ = color;
	}
    }
}


static void
sloped_line (graphic * gr, double x1, double x2, double y1, double y2)
{
  double dx, dy;

  dx = fabs (x2 - x1);
  dy = fabs (y2 - y1);

  if (dx > dy)			/* x steps 1 */
    {
      double delta_y;

      if (x1 > x2)		/* swap start and end point */
	{
	  double tx, ty;

	  tx = x1;
	  x1 = x2;
	  x2 = tx;

	  ty = y1;
	  y1 = y2;
	  y2 = ty;
	}

      dy = y2 - y1;
      delta_y = dy / dx;

      /* sanitize range of x */
      if (x1 < 0.0)
	{
	  y1 -= delta_y * x1;
	  x1 = 0.0;
	}

      if (x2 >= gr->width)
	x2 = gr->width - 1.0;

      if (gr->thickness > 0)
	{
	  double x, y;

	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    putdot (gr, x, y);
	}
      else
	{
	  double x, y;
	  struct color color = gr->color;
	  int width = gr->width;

	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    {
	      if (visible_y (gr, y))
		putpixelcolor (gr, x, y, width, color);
	    }
	}
    }
  else				/* y steps 1 */
    {
      double delta_x;

      if (y1 > y2)		/* swap start and end point */
	{
	  double tx, ty;

	  tx = x1;
	  x1 = x2;
	  x2 = tx;

	  ty = y1;
	  y1 = y2;
	  y2 = ty;
	}

      dx = x2 - x1;
      delta_x = dx / dy;

      /* sanitize range of y */
      if (y1 < 0.0)
	{
	  x1 -= delta_x * y1;
	  y1 = 0.0;
	}

      if (y2 >= gr->height)
	y2 = gr->height - 1.0;

      if (gr->thickness > 0)
	{
	  double x, y;

	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    putdot (gr, x, y);
	}
      else
	{
	  double x, y;
	  struct color color = gr->color;
	  int width = gr->width;

	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    {
	      if (visible_x (gr, x))
		putpixelcolor (gr, x, y, width, color);
	    }
	}
    }
}


static void
line (graphic * gr, double x1, double y1, double x2, double y2)
{
  int ix1, iy1, ix2, iy2;

  ix1 = (int) x1;
  iy1 = (int) y1;
  ix2 = (int) x2;
  iy2 = (int) y2;

  if (ix1 == ix2 && iy1 == iy2)	/* one dot */
    {
      if (visible (gr, ix1, iy1))
	putdot (gr, ix1, iy1);
    }
  else if (ix1 == ix2)
    vertical_line (gr, ix1, iy1, iy2);
  else if (iy1 == iy2)
    horizontal_line (gr, ix1, ix2, iy1);
  else
    sloped_line (gr, x1, x2, y1, y2);
}


/* gr:line (x1, y1, x2, y2) */
static int
lgraphic_line (lua_State * L)
{
  graphic *gr;
  double x1, y1, x2, y2;

  gr = get_graphic (L, 1);
  x1 = luaL_checknumber (L, 2) - 1.0;
  y1 = luaL_checknumber (L, 3) - 1.0;
  x2 = luaL_checknumber (L, 4) - 1.0;
  y2 = luaL_checknumber (L, 5) - 1.0;

  line (gr, x1, y1, x2, y2);
  penpos (gr, x2, y2);

  return 0;
}


/* gr:lineto (x, y) */
static int
lgraphic_lineto (lua_State * L)
{
  graphic *gr;
  double x2, y2;

  gr = get_graphic (L, 1);
  x2 = luaL_checknumber (L, 2) - 1.0;
  y2 = luaL_checknumber (L, 3) - 1.0;

  line (gr, gr->penx, gr->peny, x2, y2);
  penpos (gr, x2, y2);

  return 0;
}


/* gr:linerel (x, y) */
static int
lgraphic_linerel (lua_State * L)
{
  graphic *gr;
  double x1, y1, x2, y2;

  gr = get_graphic (L, 1);
  x1 = gr->penx;
  y1 = gr->peny;
  x2 = x1 + luaL_checknumber (L, 2);
  y2 = y1 + luaL_checknumber (L, 3);

  line (gr, x1, y1, x2, y2);
  penpos (gr, x2, y2);

  return 0;
}


/* gr:putpixel ([x, y]) */
static int
lgraphic_putpixel (lua_State * L)
{
  graphic *gr;
  int x, y;

  gr = get_graphic (L, 1);
  x = luaL_optint (L, 2, gr->penx + 1) - 1;
  y = luaL_optint (L, 3, gr->peny + 1) - 1;

  if (visible (gr, x, y))
    putpixel (gr, x, y);

  return 0;
}


/* gr:getpixel ([x, y]) */
static int
lgraphic_getpixel (lua_State * L)
{
  graphic *gr;
  int x, y;

  gr = get_graphic (L, 1);
  x = luaL_optint (L, 2, gr->penx + 1) - 1;
  y = luaL_optint (L, 3, gr->peny + 1) - 1;

  if (visible (gr, x, y))
    {
      char color[8];
      struct color *p;
      p = gr->data + (y * gr->width) + x;
      sprintf (color, "#%02X%02X%02X", (unsigned int) p->red,
	       (unsigned int) p->green, (unsigned int) p->blue);
      lua_pushstring (L, color);
      return 1;
    }
  else				/* outside */
    {
      lua_pushnil (L);
      lua_pushliteral (L, "outside of graphic");
      return 2;
    }
}


/* gr:getpixelrgb ([x, y]) */
static int
lgraphic_getpixelrgb (lua_State * L)
{
  graphic *gr;
  int x, y;

  gr = get_graphic (L, 1);
  x = luaL_optint (L, 2, (int) gr->penx + 1) - 1;
  y = luaL_optint (L, 3, (int) gr->peny + 1) - 1;

  if (visible (gr, x, y))
    {
      struct color *p;
      p = gr->data + (y * gr->width) + x;
      lua_pushinteger (L, (int) p->red);
      lua_pushinteger (L, (int) p->green);
      lua_pushinteger (L, (int) p->blue);
      return 3;
    }
  else				/* outside */
    {
      lua_pushnil (L);
      lua_pushliteral (L, "outside of graphic");
      return 2;
    }
}


/* gr:putdot ([x, y]) */
static int
lgraphic_putdot (lua_State * L)
{
  graphic *gr;
  int x, y;

  gr = get_graphic (L, 1);
  x = luaL_optint (L, 2, (int) gr->penx + 1) - 1;
  y = luaL_optint (L, 3, (int) gr->peny + 1) - 1;

  /* macros evaluate the values more than once! */
  if (visible (gr, x, y))
    {
      if (gr->thickness > 0)
	putdot (gr, x, y);
      else
	putpixel (gr, x, y);
    }

  return 0;
}


/* gr:bar (x1, y1, x2, y2) */
static int
lgraphic_bar (lua_State * L)
{
  graphic *gr;
  int x1, y1, x2, y2;

  gr = get_graphic (L, 1);

  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  bar (gr, x1, y1, x2, y2);

  return 0;
}


/* gr:rectangle (x1, y1, x2, y2) */
static int
lgraphic_rectangle (lua_State * L)
{
  graphic *gr;
  int x1, y1, x2, y2;

  gr = get_graphic (L, 1);

  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  horizontal_line (gr, x1, x2, y1);
  vertical_line (gr, x2, y1, y2);
  horizontal_line (gr, x2, x1, y2);
  vertical_line (gr, x1, y2, y1);

  return 0;
}

/* gr:arc (radius [,angle1] [,angle2]) */
static int
lgraphic_arc (lua_State * L)
{
  graphic *gr;
  double xcenter, ycenter, radius, startangle, endangle;
  double x, y, i;

  gr = get_graphic (L, 1);

  radius = luaL_checknumber (L, 2);

  if (lua_isnoneornil (L, 3))	/* no angles given */
    {
      startangle = 0;
      endangle = 360;		/* full circle */
    }
  else if (lua_isnoneornil (L, 4))	/* one angle given */
    {
      startangle = gr->heading;
      endangle = luaL_checknumber (L, 3);
    }
  else				/* two angles given */
    {
      startangle = luaL_checknumber (L, 3);
      endangle = luaL_checknumber (L, 4);
    }

  while (startangle > endangle)
    endangle += 360;

  xcenter = gr->penx;
  ycenter = gr->peny;

  x = xcenter + radius * sin (RAD (startangle));
  y = ycenter - radius * cos (RAD (startangle));

  for (i = startangle; i <= endangle; i++)
    {
      double newx, newy;
      newx = xcenter + radius * sin (RAD (i));
      newy = ycenter - radius * cos (RAD (i));
      line (gr, x, y, newx, newy);
      x = newx;
      y = newy;
    }

  return 0;
}


static int
lgraphic_show (lua_State * L)
{
  graphic *gr;
  int status;

  gr = get_graphic (L, 1);
  avt_show_raw_image (&gr->data, gr->width, gr->height, BPP);

  status = avt_update ();

  if (status <= AVT_ERROR)
    {
      return luaL_error (L, "%s", avt_get_error ());
    }
  else if (status == AVT_QUIT)
    {
      /* no actual error, so no error message */
      /* this is handled by the calling program */
      lua_pushnil (L);
      return lua_error (L);
    }

  return 0;
}


static int
lgraphic_size (lua_State * L)
{
  graphic *gr;

  gr = get_graphic (L, 1);
  lua_pushinteger (L, gr->width);
  lua_pushinteger (L, gr->height);

  return 2;
}


static int
lgraphic_width (lua_State * L)
{
  lua_pushinteger (L, get_graphic (L, 1)->width);
  return 1;
}


static int
lgraphic_height (lua_State * L)
{
  lua_pushinteger (L, get_graphic (L, 1)->height);
  return 1;
}


static int
lgraphic_font_size (lua_State * L)
{
  int fontwidth, fontheight;

  avt_get_font_size (&fontwidth, &fontheight);

  lua_pushinteger (L, fontwidth);
  lua_pushinteger (L, fontheight);

  return 2;
}


/* gr:text (string [,x ,y]) */
static int
lgraphic_text (lua_State * L)
{
  graphic *gr;
  const char *s;
  size_t len;
  wchar_t *wctext, *wc;
  int wclen, i;
  int x, y;
  int fontwidth, fontheight;

  gr = get_graphic (L, 1);
  s = luaL_checklstring (L, 2, &len);
  x = luaL_optint (L, 3, (int) gr->penx + 1) - 1;
  y = luaL_optint (L, 4, (int) gr->peny + 1) - 1;

  avt_get_font_size (&fontwidth, &fontheight);

  switch (gr->vtextalign)
    {
    case VA_TOP:
      break;

    case VA_CENTER:
      y -= fontheight / 2;
      break;

    case VA_BOTTOM:
      y -= fontheight;
      break;
    }

  /* vertically outside visible area? (cannot show partly) */
  if (y < 0 || y >= gr->height - fontheight)
    return 0;

  wclen = avt_mb_decode (&wctext, s, (int) len);
  if (!wctext)
    return 0;

  switch (gr->htextalign)
    {
    case HA_LEFT:
      break;

    case HA_CENTER:
      x -= wclen * fontwidth / 2;
      break;

    case HA_RIGHT:
      x -= wclen * fontwidth;
      break;
    }

  /* horizontally outside visible area? (cannot show partly) */
  if (wclen <= 0 || x >= gr->width - fontwidth || x + (wclen * fontwidth) < 0)
    {
      avt_free (wctext);
      return 0;
    }

  wc = wctext;

  /* crop text as neccessary */
  if (x < 0)
    {
      int pixels = fontwidth - x + 1;
      int crop = pixels / fontwidth;
      wc += crop;
      wclen -= crop;
      x = fontwidth - (pixels % fontwidth);
    }

  if (wclen > (gr->width - x) / fontwidth)
    wclen = (gr->width - x) / fontwidth;

  if (fontwidth > 8)		/* 2 bytes per character */
    {
      struct color color = gr->color;
      int width = gr->width;

      for (i = 0; i < wclen; i++, wc++, x += fontwidth)
	{
	  const unsigned short *font_line;
	  int lx, ly;

	  font_line = (const unsigned short *) get_font_char ((int) *wc);
	  if (!font_line)
	    font_line = (const unsigned short *) get_font_char (0);

	  for (ly = 0; ly < fontheight; ly++)
	    {
	      for (lx = 0; lx < fontwidth; lx++)
		if (*font_line & (1 << (15 - lx)))
		  putpixelcolor (gr, x + lx, y + ly, width, color);
	      font_line++;
	    }
	}
    }
  else				/* fontwidth <= 8 */
    {
      struct color color = gr->color;
      int width = gr->width;

      for (i = 0; i < wclen; i++, wc++, x += fontwidth)
	{
	  const uchar *font_line;
	  int lx, ly;

	  font_line = (const uchar *) get_font_char ((int) *wc);
	  if (!font_line)
	    font_line = (const uchar *) get_font_char (0);

	  for (ly = 0; ly < fontheight; ly++)
	    {
	      for (lx = 0; lx < fontwidth; lx++)
		if (*font_line & (1 << (7 - lx)))
		  putpixelcolor (gr, x + lx, y + ly, width, color);
	      font_line++;
	    }
	}
    }

  avt_free (wctext);

  return 0;
}


/* gr:textalign (horizontal, vertical) */
static int
lgraphic_textalign (lua_State * L)
{
  graphic *gr;
  const char *const hoptions[] = { "left", "center", "right", NULL };
  const char *const voptions[] = { "top", "center", "bottom", NULL };

  gr = get_graphic (L, 1);
  gr->htextalign = luaL_checkoption (L, 2, "center", hoptions);
  gr->vtextalign = luaL_checkoption (L, 3, "center", voptions);

  return 0;
}

/* gr:heading(degree) */
static int
lgraphic_heading (lua_State * L)
{
  graphic *gr;
  double degree;

  gr = get_graphic (L, 1);
  degree = (double) luaL_checknumber (L, 2);

  while (degree < 0.0)
    degree += 360.0;

  while (degree > 360.0)
    degree -= 360.0;

  gr->heading = degree;

  return 0;
}


/* degree = gr:get_heading() */
static int
lgraphic_get_heading (lua_State * L)
{
  graphic *gr;

  gr = get_graphic (L, 1);
  lua_pushnumber (L, (lua_Number) gr->heading);

  return 1;
}


/* gr:right(degree) */
static int
lgraphic_right (lua_State * L)
{
  graphic *gr;
  double degree, heading;

  gr = get_graphic (L, 1);
  degree = (double) luaL_checknumber (L, 2);

  heading = gr->heading + degree;

  while (heading > 360.0)
    heading -= 360.0;

  gr->heading = heading;

  return 0;
}


/* gr:left(degree) */
static int
lgraphic_left (lua_State * L)
{
  graphic *gr;
  double degree, heading;

  gr = get_graphic (L, 1);
  degree = (double) luaL_checknumber (L, 2);

  heading = gr->heading - degree;

  while (heading < 0.0)
    heading += 360.0;

  gr->heading = heading;

  return 0;
}


/* gr:draw(steps) */
static int
lgraphic_draw (lua_State * L)
{
  graphic *gr;
  double penx, peny, x, y;
  double steps, value;

  gr = get_graphic (L, 1);
  steps = (double) luaL_checknumber (L, 2);

  penx = gr->penx;
  peny = gr->peny;
  value = RAD (gr->heading);

  x = penx + steps * sin (value);
  y = peny - steps * cos (value);

  line (gr, penx, peny, x, y);
  penpos (gr, x, y);

  return 0;
}


/* gr:move(steps) */
static int
lgraphic_move (lua_State * L)
{
  graphic *gr;
  double steps, value;

  gr = get_graphic (L, 1);
  steps = (double) luaL_checknumber (L, 2);

  value = RAD (gr->heading);
  penpos (gr, gr->penx + steps * sin (value), gr->peny - steps * cos (value));

  return 0;
}


/* gr:put(graphic, xoffset, yoffset) */
static int
lgraphic_put (lua_State * L)
{
  graphic *gr, *gr2;
  int xoffset, yoffset, y;
  int lines, bytes;
  int xstart, show_width;	/* for horizontal cropping */
  int source_width, target_width, source_height, target_height;
  struct color *source, *target;

  gr = get_graphic (L, 1);
  gr2 = get_graphic (L, 2);

  if (gr == gr2)
    return luaL_error (L, "cannot put a graphic onto itself");

  xoffset = luaL_checkint (L, 3) - 1;
  yoffset = luaL_checkint (L, 4) - 1;

  source = gr2->data;
  target = gr->data;
  source_width = gr2->width;
  target_width = gr->width;
  source_height = gr2->height;
  target_height = gr->height;
  xstart = 0;
  show_width = source_width;

  /* which line to start with? */
  if (yoffset < 0)
    {
      source += abs (yoffset) * source_width;
      source_height -= abs (yoffset);
      yoffset = 0;
    }

  /* how many lines to copy? */
  if (target_height > source_height + yoffset)
    lines = source_height;
  else
    lines = target_height - yoffset;

  if (lines <= 0)
    return 0;			/* nothing to copy */

  if (xoffset < 0)
    {
      xstart = abs (xoffset);
      show_width -= abs (xoffset);
      xoffset = 0;
    }

  /* how many bytes per line? */
  if (target_width > show_width + xoffset)
    bytes = show_width * BPP;
  else
    bytes = (target_width - xoffset) * BPP;

  if (bytes > 0)
    {
      for (y = 0; y < lines; y++)
	memcpy (target + ((y + yoffset) * target_width) + xoffset,
		source + y * source_width + xstart, bytes);
    }

  return 0;
}


/* gr:get(x1, y1, x2, y2) */
static int
lgraphic_get (lua_State * L)
{
  graphic *gr, *gr2;
  int x1, y1, x2, y2;
  int source_width, target_width, source_height, target_height;
  int bytes, y;
  struct color *source, *target;

  gr = get_graphic (L, 1);

  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  source_width = gr->width;
  source_height = gr->height;

  luaL_argcheck (L, x1 >= 0 && x1 < source_width, 2, "value out of range");
  luaL_argcheck (L, y1 >= 0 && y1 < source_height, 3, "value out of range");
  luaL_argcheck (L, x2 >= 0 && x2 < source_width, 4, "value out of range");
  luaL_argcheck (L, y2 >= 0 && y2 < source_height, 5, "value out of range");

  if (x1 > x2)			/* swap */
    {
      int tx = x1;
      x1 = x2;
      x2 = tx;
    }

  if (y1 > y2)			/* swap */
    {
      int ty = y1;
      y1 = y2;
      y2 = ty;
    }

  target_width = x2 - x1 + 1;
  target_height = y2 - y1 + 1;

  gr2 = new_graphic (L, graphic_bytes (target_width, target_height));
  gr2->width = target_width;
  gr2->height = target_height;
  gr2->color = gr->color;
  gr2->background = gr->background;

  /* pen in center */
  penpos (gr2, target_width / 2 - 1, target_height / 2 - 1);
  gr2->thickness = gr->thickness;

  gr2->htextalign = gr->htextalign;
  gr2->vtextalign = gr->vtextalign;
  gr2->heading = 0.0;

  source = gr->data + y1 * source_width;
  target = gr2->data;
  bytes = target_width * BPP;

  for (y = 0; y < target_height; y++)
    memcpy (target + (y * target_width),
	    source + y * source_width + x1, bytes);

  return 1;
}


static int
lgraphic_duplicate (lua_State * L)
{
  size_t nbytes;
  graphic *gr, *gr2;

  gr = get_graphic (L, 1);
  nbytes = graphic_bytes (gr->width, gr->height);
  gr2 = new_graphic (L, nbytes);
  memcpy (gr2, gr, nbytes);

  return 1;
}


static int
lgraphic_shift_vertically (lua_State * L)
{
  graphic *gr;
  struct color *data, *area;
  int lines;
  int width, height;

  area = NULL;
  gr = get_graphic (L, 1);
  data = gr->data;
  width = gr->width;
  height = gr->height;
  lines = luaL_checkint (L, 2);

  /* move pen position */
  gr->peny += (double) lines;

  if (abs (lines) >= height)	/* clear all */
    {
      lines = height;
      area = data;
    }
  else if (lines > 0)		/* move down */
    {
      memmove (data + (lines * width), data, (height - lines) * width * BPP);
      area = data;
    }
  else if (lines < 0)		/* move up */
    {
      lines = -lines;		/* make lines positive */
      memmove (data, data + (lines * width), (height - lines) * width * BPP);
      area = data + ((height - lines) * width);
    }
  /* do nothing if lines == 0 */

  /* clear the area */
  if (area)
    {
      int i;
      struct color color = gr->background;

      for (i = 0; i < lines * width; i++)
	*area++ = color;
    }

  return 0;
}


static int
lgraphic_shift_horizontally (lua_State * L)
{
  graphic *gr;
  struct color *data, *area;
  int columns;
  int width, height;

  area = NULL;
  gr = get_graphic (L, 1);
  data = gr->data;
  width = gr->width;
  height = gr->height;
  columns = luaL_checkint (L, 2);

  /* move pen position */
  gr->penx += (double) columns;

  if (abs (columns) >= width)	/* clear all */
    {
      columns = width;
      area = data;
    }
  else if (columns > 0)		/* move right */
    {
      memmove (data + columns, data, ((width * height) - columns) * BPP);
      area = data;
    }
  else if (columns < 0)		/* move left */
    {
      columns = -columns;	/* make columns positive */
      memmove (data, data + columns, ((width * height) - columns) * BPP);
      area = data + width - columns;
    }
  /* do nothing if columns == 0 */

  /* clear the area */
  if (area)
    {
      int x, y;
      struct color *p;
      struct color color = gr->background;

      for (y = 0; y < height; y++)
	{
	  p = area + (y * width);
	  for (x = 0; x < columns; x++)
	    *p++ = color;
	}
    }

  return 0;
}


static int
lgraphic_export_ppm (lua_State * L)
{
  graphic *gr;
  const char *fname;
  FILE *f;

  if (BPP != 3)
    return luaL_error (L, "%s", strerror (ENOSYS));

  gr = get_graphic (L, 1);
  fname = luaL_checkstring (L, 2);

  f = fopen (fname, "wb");

  if (!f)
    return luaL_error (L, "\"%s\": %s", fname, strerror (errno));

  fprintf (f, "P6\n%d %d\n255\n", gr->width, gr->height);
  fwrite (gr->data, 1, gr->height * gr->width * BPP, f);

  if (fclose (f) != 0)
    return luaL_error (L, "\"%s\": %s", fname, strerror (errno));

  return 0;
}


static const struct luaL_reg graphiclib[] = {
  {"new", lgraphic_new},
  {"fullsize", lgraphic_fullsize},
  {"font_size", lgraphic_font_size},
  {NULL, NULL}
};


static const struct luaL_reg graphiclib_methods[] = {
  {"clear", lgraphic_clear},
  {"color", lgraphic_color},
  {"rgb", lgraphic_rgb},
  {"eraser", lgraphic_eraser},
  {"thickness", lgraphic_thickness},
  {"putpixel", lgraphic_putpixel},
  {"getpixel", lgraphic_getpixel},
  {"getpixelrgb", lgraphic_getpixelrgb},
  {"line", lgraphic_line},
  {"pen_position", lgraphic_pen_position},
  {"center", lgraphic_center},
  {"home", lgraphic_center},
  {"moveto", lgraphic_moveto},
  {"moverel", lgraphic_moverel},
  {"lineto", lgraphic_lineto},
  {"linerel", lgraphic_linerel},
  {"putdot", lgraphic_putdot},
  {"bar", lgraphic_bar},
  {"rectangle", lgraphic_rectangle},
  {"arc", lgraphic_arc},
  {"circle", lgraphic_arc},
  {"text", lgraphic_text},
  {"textalign", lgraphic_textalign},
  {"font_size", lgraphic_font_size},
  {"show", lgraphic_show},
  {"size", lgraphic_size},
  {"width", lgraphic_width},
  {"height", lgraphic_height},
  {"put", lgraphic_put},
  {"get", lgraphic_get},
  {"duplicate", lgraphic_duplicate},
  {"heading", lgraphic_heading},
  {"get_heading", lgraphic_get_heading},
  {"right", lgraphic_right},
  {"left", lgraphic_left},
  {"draw", lgraphic_draw},
  {"move", lgraphic_move},
  {"shift_vertically", lgraphic_shift_vertically},
  {"shift_horizontally", lgraphic_shift_horizontally},
  {"export_ppm", lgraphic_export_ppm},
  {NULL, NULL}
};


int
luaopen_graphic (lua_State * L)
{
  luaL_register (L, "graphic", graphiclib);

  luaL_newmetatable (L, GRAPHICDATA);
  lua_pushvalue (L, -1);
  lua_setfield (L, -2, "__index");
  luaL_register (L, NULL, graphiclib_methods);
  lua_pop (L, 1);

  return 1;
}
