/*
 * AKFAvatar canvas
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

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Bytes per pixel (3=RGB) */
#define BPP 3

#define PI  3.14159265358979323846

#define CANVASDATA "AKFAvatar-canvas"

#define get_canvas(L,idx) \
   ((canvas *) luaL_checkudata ((L), (idx), CANVASDATA))

#define canvas_bytes(width, height) \
  sizeof(canvas)-sizeof(unsigned char)+(width)*(height)*BPP

#define new_canvas(L, nbytes) \
  (canvas *) lua_newuserdata ((L), (nbytes)); \
  luaL_getmetatable ((L), CANVASDATA); \
  lua_setmetatable ((L), -2)

/* force value to be in range */
#define RANGE(v, min, max)  ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))

#define visible_x(c, x)  ((int) (x) >= 0 && (int) (x) < (c)->width)
#define visible_y(c, y)  ((int) (y) >= 0 && (int) (y) < (c)->height)
#define visible(c, x, y)  (visible_x(c, x) && visible_y(c, y))

/* set pen position */
#define penpos(c, x, y)  (c)->penx = (int) (x); (c)->peny = (int) (y)

/* fast putpixel with rgb, no check */
#define putpixelrgb(c, x, y, width, r, g, b) \
  do { \
    unsigned char *p = \
      (c)->data+(((int)(y))*(width)*BPP)+(((int)(x))*BPP); \
    *p++=(r); *p++=(g); *p=(b); \
  } while(0)

/* fast putpixel, no check */
#define putpixel(c, x, y) \
  putpixelrgb ((c), (x), (y), (c)->width, (c)->r, (c)->g, (c)->b)

#define HA_LEFT 0
#define HA_CENTER 1
#define HA_RIGHT 2
#define VA_TOP 0
#define VA_CENTER 1
#define VA_BOTTOM 2

typedef struct canvas
{
  int width, height;
  int penx, peny;		/* position of pen */
  int thickness;		/* thickness of pen */
  unsigned char r, g, b;	/* current color */
  int htextalign, vtextalign;	/* alignment for text */
  unsigned char data[1];
} canvas;


static void
clear_canvas (canvas * c)
{
  unsigned char *p;
  int red, green, blue;
  unsigned char r, g, b;
  size_t i, pixels;

  avt_get_background_color (&red, &green, &blue);
  r = (unsigned char) red;
  g = (unsigned char) green;
  b = (unsigned char) blue;

  pixels = c->width * c->height;
  p = c->data;

  for (i = 0; i < pixels; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;
    }
}


static void
bar (canvas * c, int x1, int y1, int x2, int y2)
{
  int x, y, width, height;
  unsigned char r, g, b;
  unsigned char *data, *p;

  width = c->width;
  height = c->height;
  data = c->data;
  r = c->r;
  g = c->g;
  b = c->b;

  /* sanitize values */
  x1 = RANGE (x1, 0, width - 1);
  y1 = RANGE (y1, 0, height - 1);
  x2 = RANGE (x2, 0, width - 1);
  y2 = RANGE (y2, 0, height - 1);


  for (y = y1; y <= y2; y++)
    {
      p = data + (y * width * BPP) + (x1 * BPP);

      for (x = x1; x <= x2; x++)
	{
	  *p++ = r;
	  *p++ = g;
	  *p++ = b;
	}
    }
}


#define putdot(c, x, y) \
  do { \
    int s = (c)->thickness; \
    bar ((c), ((int) (x))-s, ((int)(y))-s, ((int)(x))+s, ((int)(y))+s); \
  } while(0)


/* local c, width, height = canvas.new([width, height]) */
static int
lcanvas_new (lua_State * L)
{
  int width, height;
  canvas *c;

  width = luaL_optint (L, 1, avt_image_max_width ());
  height = luaL_optint (L, 2, avt_image_max_height ());

  c = new_canvas (L, canvas_bytes (width, height));
  c->width = width;
  c->height = height;

  /* black color */
  c->r = c->g = c->b = 0;

  /* pen in center */
  penpos (c, width / 2 - 1, height / 2 - 1);
  c->thickness = 1 - 1;

  c->htextalign = HA_CENTER;
  c->vtextalign = VA_CENTER;

  clear_canvas (c);

  lua_pushinteger (L, width);
  lua_pushinteger (L, height);

  return 3;
}


static int
lcanvas_clear (lua_State * L)
{
  clear_canvas (get_canvas (L, 1));
  return 0;
}


/* c:color(colorname) */
static int
lcanvas_color (lua_State * L)
{
  canvas *c;
  int red, green, blue;
  const char *name;

  name = luaL_checkstring (L, 2);
  if (avt_name_to_color (name, &red, &green, &blue) == AVT_NORMAL)
    {
      c = get_canvas (L, 1);
      c->r = (unsigned char) red;
      c->g = (unsigned char) green;
      c->b = (unsigned char) blue;
    }

  return 0;
}


/* c:rgb(red, green, blue) */
static int
lcanvas_rgb (lua_State * L)
{
  canvas *c;
  int red, green, blue;

  red = luaL_checkint (L, 2);
  green = luaL_checkint (L, 3);
  blue = luaL_checkint (L, 4);

  luaL_argcheck (L, red > 0 && red < 256,
		 2, "value between 0 and 255 expected");

  luaL_argcheck (L, green > 0 && green < 256,
		 3, "value between 0 and 255 expected");

  luaL_argcheck (L, blue > 0 && blue < 256,
		 4, "value between 0 and 255 expected");

  c = get_canvas (L, 1);
  c->r = (unsigned char) red;
  c->g = (unsigned char) green;
  c->b = (unsigned char) blue;

  return 0;
}


/* c:eraser() */
static int
lcanvas_eraser (lua_State * L)
{
  canvas *c;
  int red, green, blue;

  c = get_canvas (L, 1);
  avt_get_background_color (&red, &green, &blue);
  c->r = (unsigned char) red;
  c->g = (unsigned char) green;
  c->b = (unsigned char) blue;

  return 0;
}


/* c:thickness(size) */
static int
lcanvas_thickness (lua_State * L)
{
  int s;

  s = luaL_checkint (L, 2) - 1;
  get_canvas (L, 1)->thickness = (s > 0) ? s : 0;

  return 0;
}


/* x, y = c:pen_position () */
static int
lcanvas_pen_position (lua_State * L)
{
  canvas *c = get_canvas (L, 1);

  lua_pushinteger (L, c->penx + 1);
  lua_pushinteger (L, c->peny + 1);

  return 2;
}


/* c:moveto (x, y) */
static int
lcanvas_moveto (lua_State * L)
{
  canvas *c = get_canvas (L, 1);

  /* a pen outside the field is allowed! */
  penpos (c, luaL_checkint (L, 2) - 1, luaL_checkint (L, 3) - 1);

  return 0;
}


/* c:moverel (x, y) */
static int
lcanvas_moverel (lua_State * L)
{
  canvas *c = get_canvas (L, 1);

  penpos (c, c->penx + luaL_checkint (L, 2), c->peny + luaL_checkint (L, 3));

  return 0;
}


/* c:center (x, y) */
static int
lcanvas_center (lua_State * L)
{
  canvas *c = get_canvas (L, 1);

  penpos (c, c->width / 2 - 1, c->height / 2 - 1);

  return 0;
}


static void
vertical_line (canvas * c, int x, int y1, int y2)
{
  int y;
  int height;

  if (visible_x (c, x))
    {
      if (y1 > y2)		/* swap */
	{
	  int ty = y1;
	  y1 = y2;
	  y2 = ty;
	}

      height = c->height;
      y1 = RANGE (y1, 0, height - 1);
      y2 = RANGE (y2, 0, height - 1);

      if (c->thickness > 0)
	{
	  for (y = y1; y <= y2; y++)
	    putdot (c, x, y);
	}
      else
	{
	  unsigned char r = c->r, g = c->g, b = c->b;
	  int width = c->width;
	  for (y = y1; y <= y2; y++)
	    if (visible_y (c, y))
	      putpixelrgb (c, x, y, width, r, g, b);
	}
    }
}


static void
horizontal_line (canvas * c, int x1, int x2, int y)
{
  int x;
  int width;

  if (visible_y (c, y))
    {
      if (x1 > x2)		/* swap */
	{
	  int tx = x1;
	  x1 = x2;
	  x2 = tx;
	}

      width = c->width;
      x1 = RANGE (x1, 0, width - 1);
      x2 = RANGE (x2, 0, width - 1);

      if (c->thickness > 0)
	{
	  for (x = x1; x <= x2; x++)
	    putdot (c, x, y);
	}
      else
	{
	  unsigned char *p;
	  unsigned char r, g, b;

	  r = c->r;
	  g = c->g;
	  b = c->b;

	  p = c->data + (y * width * BPP) + (x1 * BPP);

	  for (x = x1; x <= x2; x++)
	    {
	      *p++ = r;
	      *p++ = g;
	      *p++ = b;
	    }
	}
    }
}


/* TODO: optimize */
static void
sloped_line (canvas * c, int x1, int x2, int y1, int y2)
{
  int dx, dy;

  dx = abs (x2 - x1);
  dy = abs (y2 - y1);

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
      delta_y = (double) dy / (double) dx;

      /* sanitize range of x */
      if (x1 < 0)
	{
	  y1 -= delta_y * x1;
	  x1 = 0;
	}

      if (x2 >= c->width)
	x2 = c->width - 1;

      if (c->thickness > 0)
	{
	  int x;
	  double y;

	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    putdot (c, x, y);
	}
      else
	{
	  int x;
	  double y;
	  unsigned char r = c->r, g = c->g, b = c->b;
	  int width = c->width;

	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    {
	      if (visible_y (c, y))
		putpixelrgb (c, x, y, width, r, g, b);
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
      delta_x = (double) dx / (double) dy;

      /* sanitize range of y */
      if (y1 < 0)
	{
	  x1 -= delta_x * y1;
	  y1 = 0;
	}

      if (y2 >= c->height)
	y2 = c->height - 1;

      if (c->thickness > 0)
	{
	  int y;
	  double x;

	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    putdot (c, x, y);
	}
      else
	{
	  int y;
	  double x;
	  unsigned char r = c->r, g = c->g, b = c->b;
	  int width = c->width;

	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    {
	      if (visible_x (c, x))
		putpixelrgb (c, x, y, width, r, g, b);
	    }
	}
    }
}


static void
line (canvas * c, int x1, int y1, int x2, int y2)
{
  if (x1 == x2 && y1 == y2)	/* one dot */
    {
      if (visible (c, x1, y1))
	putdot (c, x1, y1);
    }
  else if (x1 == x2)
    vertical_line (c, x1, y1, y2);
  else if (y1 == y2)
    horizontal_line (c, x1, x2, y1);
  else
    sloped_line (c, x1, x2, y1, y2);
}


/* c:line (x1, y1, x2, y2) */
static int
lcanvas_line (lua_State * L)
{
  canvas *c;
  int x1, y1, x2, y2;

  c = get_canvas (L, 1);
  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  line (c, x1, y1, x2, y2);
  penpos (c, x2, y2);

  return 0;
}


/* c:lineto (x, y) */
static int
lcanvas_lineto (lua_State * L)
{
  canvas *c;
  int x2, y2;

  c = get_canvas (L, 1);
  x2 = luaL_checkint (L, 2) - 1;
  y2 = luaL_checkint (L, 3) - 1;

  line (c, c->penx, c->peny, x2, y2);
  penpos (c, x2, y2);

  return 0;
}


/* c:linerel (x, y) */
static int
lcanvas_linerel (lua_State * L)
{
  canvas *c;
  int x1, y1, x2, y2;

  c = get_canvas (L, 1);
  x1 = c->penx;
  y1 = c->peny;
  x2 = x1 + luaL_checkint (L, 2);
  y2 = y1 + luaL_checkint (L, 3);

  line (c, x1, y1, x2, y2);
  penpos (c, x2, y2);

  return 0;
}


/* c:putpixel ([x, y]) */
static int
lcanvas_putpixel (lua_State * L)
{
  canvas *c;
  int x, y;

  c = get_canvas (L, 1);
  x = luaL_optint (L, 2, c->penx + 1) - 1;
  y = luaL_optint (L, 3, c->peny + 1) - 1;

  if (visible (c, x, y))
    putpixel (c, x, y);

  return 0;
}


/* c:getpixel ([x, y]) */
static int
lcanvas_getpixel (lua_State * L)
{
  canvas *c;
  int x, y;

  c = get_canvas (L, 1);
  x = luaL_optint (L, 2, c->penx + 1) - 1;
  y = luaL_optint (L, 3, c->peny + 1) - 1;

  if (visible (c, x, y))
    {
      char color[8];
      unsigned char *p = c->data + (y * c->width * BPP) + x * BPP;
      sprintf (color, "#%02X%02X%02X", (unsigned int) *p,
	       (unsigned int) *(p + 1), (unsigned int) *(p + 2));
      lua_pushstring (L, color);
      return 1;
    }
  else				/* outside */
    {
      lua_pushnil (L);
      lua_pushliteral (L, "outside of canvas");
      return 2;
    }
}


/* c:getpixelrgb ([x, y]) */
static int
lcanvas_getpixelrgb (lua_State * L)
{
  canvas *c;
  int x, y;

  c = get_canvas (L, 1);
  x = luaL_optint (L, 2, c->penx + 1) - 1;
  y = luaL_optint (L, 3, c->peny + 1) - 1;

  if (visible (c, x, y))
    {
      unsigned char *p = c->data + (y * c->width * BPP) + x * BPP;
      lua_pushinteger (L, (int) *p);	/* red */
      lua_pushinteger (L, (int) *(p + 1));	/* green */
      lua_pushinteger (L, (int) *(p + 2));	/* blue */
      return 3;
    }
  else				/* outside */
    {
      lua_pushnil (L);
      lua_pushliteral (L, "outside of canvas");
      return 2;
    }
}


/* c:putdot ([x, y]) */
static int
lcanvas_putdot (lua_State * L)
{
  canvas *c;
  int x, y;

  c = get_canvas (L, 1);
  x = luaL_optint (L, 2, c->penx + 1) - 1;
  y = luaL_optint (L, 3, c->peny + 1) - 1;

  /* macros evaluate the values more than once! */
  if (visible (c, x, y))
    {
      if (c->thickness > 0)
	putdot (c, x, y);
      else
	putpixel (c, x, y);
    }

  return 0;
}


/* c:bar (x1, y1, x2, y2) */
static int
lcanvas_bar (lua_State * L)
{
  canvas *c;
  int x1, y1, x2, y2;

  c = get_canvas (L, 1);

  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  bar (c, x1, y1, x2, y2);

  return 0;
}


/* c:rectangle (x1, y1, x2, y2) */
static int
lcanvas_rectangle (lua_State * L)
{
  canvas *c;
  int x1, y1, x2, y2;

  c = get_canvas (L, 1);

  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  horizontal_line (c, x1, x2, y1);
  vertical_line (c, x2, y1, y2);
  horizontal_line (c, x2, x1, y2);
  vertical_line (c, x1, y2, y1);

  return 0;
}

/* c:circle (xcenter, ycenter, radius [,startangle] [,endangle]) */
static int
lcanvas_circle (lua_State * L)
{
  canvas *c;
  double xcenter, ycenter, radius, startangle, endangle;
  double x, y, i;

  c = get_canvas (L, 1);

  xcenter = luaL_checknumber (L, 2) - 1;
  ycenter = luaL_checknumber (L, 3) - 1;
  radius = luaL_checknumber (L, 4);
  startangle = luaL_optnumber (L, 5, 0);
  endangle = luaL_optnumber (L, 6, 360);

  penpos (c, xcenter, ycenter);

  while (startangle > endangle)
    endangle += 360;

  x = xcenter + radius * sin (2 * PI * startangle / 360);
  y = ycenter - radius * cos (2 * PI * startangle / 360);

  for (i = startangle; i <= endangle; i++)
    {
      double newx, newy;
      newx = xcenter + radius * sin (2 * PI * i / 360);
      newy = ycenter - radius * cos (2 * PI * i / 360);
      line (c, x, y, newx, newy);
      x = newx;
      y = newy;
    }

  return 0;
}


static int
lcanvas_show (lua_State * L)
{
  canvas *c;
  int status;

  c = get_canvas (L, 1);
  avt_show_raw_image (&c->data, c->width, c->height, BPP);

  status = avt_update();

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
lcanvas_size (lua_State * L)
{
  canvas *c;

  c = get_canvas (L, 1);
  lua_pushinteger (L, c->width);
  lua_pushinteger (L, c->height);

  return 2;
}


static int
lcanvas_width (lua_State * L)
{
  lua_pushinteger (L, get_canvas (L, 1)->width);
  return 1;
}


static int
lcanvas_height (lua_State * L)
{
  lua_pushinteger (L, get_canvas (L, 1)->height);
  return 1;
}


static int
lcanvas_font_size (lua_State * L)
{
  int fontwidth, fontheight;

  avt_get_font_size (&fontwidth, &fontheight);

  lua_pushinteger (L, fontwidth);
  lua_pushinteger (L, fontheight);

  return 2;
}


/* c:text (string [,x ,y]) */
static int
lcanvas_text (lua_State * L)
{
  canvas *c;
  const char *s;
  size_t len;
  wchar_t *wctext, *wc;
  int wclen, i;
  int x, y;
  int fontwidth, fontheight;

  c = get_canvas (L, 1);
  s = luaL_checklstring (L, 2, &len);
  x = luaL_optint (L, 3, c->penx + 1) - 1;
  y = luaL_optint (L, 4, c->peny + 1) - 1;

  avt_get_font_size (&fontwidth, &fontheight);

  switch (c->vtextalign)
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
  if (y < 0 || y >= c->height - fontheight)
    return 0;

  wclen = avt_mb_decode (&wctext, s, (int) len);
  if (!wctext)
    return 0;

  switch (c->htextalign)
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
  if (wclen <= 0 || x >= c->width - fontwidth || x + (wclen * fontwidth) < 0)
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

  if (wclen > (c->width - x) / fontwidth)
    wclen = (c->width - x) / fontwidth;

  if (fontwidth > 8)		/* 2 bytes per character */
    {
      unsigned char r = c->r, g = c->g, b = c->b;
      int width = c->width;

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
		  putpixelrgb (c, x + lx, y + ly, width, r, g, b);
	      font_line++;
	    }
	}
    }
  else				/* fontwidth <= 8 */
    {
      unsigned char r = c->r, g = c->g, b = c->b;
      int width = c->width;

      for (i = 0; i < wclen; i++, wc++, x += fontwidth)
	{
	  const unsigned char *font_line;
	  int lx, ly;

	  font_line = (const unsigned char *) get_font_char ((int) *wc);
	  if (!font_line)
	    font_line = (const unsigned char *) get_font_char (0);

	  for (ly = 0; ly < fontheight; ly++)
	    {
	      for (lx = 0; lx < fontwidth; lx++)
		if (*font_line & (1 << (7 - lx)))
		  putpixelrgb (c, x + lx, y + ly, width, r, g, b);
	      font_line++;
	    }
	}
    }

  avt_free (wctext);

  return 0;
}


/* c:textalign (horizontal, vertical) */
static int
lcanvas_textalign (lua_State * L)
{
  canvas *c;
  const char *const hoptions[] = { "left", "center", "right", NULL };
  const char *const voptions[] = { "top", "center", "bottom", NULL };

  c = get_canvas (L, 1);
  c->htextalign = luaL_checkoption (L, 2, "center", hoptions);
  c->vtextalign = luaL_checkoption (L, 3, "center", voptions);

  return 0;
}


/* c:put(canvas, xoffset, yoffset) */
static int
lcanvas_put (lua_State * L)
{
  canvas *c, *c2;
  int xoffset, yoffset, y;
  int lines, bytes;
  int xstart, show_width;	/* for horizontal cropping */
  int source_width, target_width, source_height, target_height;
  unsigned char *source, *target;

  c = get_canvas (L, 1);
  c2 = get_canvas (L, 2);

  if (c == c2)
    return luaL_error (L, "cannot put a canvas onto itself");

  xoffset = luaL_checkint (L, 3) - 1;
  yoffset = luaL_checkint (L, 4) - 1;

  source = c2->data;
  target = c->data;
  source_width = c2->width;
  target_width = c->width;
  source_height = c2->height;
  target_height = c->height;
  xstart = 0;
  show_width = source_width;

  /* which line to start with? */
  if (yoffset < 0)
    {
      source += abs (yoffset) * source_width * BPP;
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
      xstart = abs (xoffset) * BPP;
      show_width -= abs (xoffset);
      xoffset = 0;
    }

  /* how many bytes per line? */
  if (target_width > show_width + xoffset)
    bytes = show_width * BPP;
  else
    bytes = (target_width - xoffset) * BPP;

  if (bytes <= 0)
    return 0;			/* nothing to copy */

  for (y = 0; y < lines; y++)
    memcpy (target + ((y + yoffset) * target_width * BPP) + xoffset * BPP,
	    source + y * source_width * BPP + xstart, bytes);

  return 0;
}


/* c:get(x1, y1, x2, y2) */
static int
lcanvas_get (lua_State * L)
{
  canvas *c, *c2;
  int x1, y1, x2, y2;
  int source_width, target_width, source_height, target_height;
  int bytes, y;
  unsigned char *source, *target;

  c = get_canvas (L, 1);

  x1 = luaL_checkint (L, 2) - 1;
  y1 = luaL_checkint (L, 3) - 1;
  x2 = luaL_checkint (L, 4) - 1;
  y2 = luaL_checkint (L, 5) - 1;

  source_width = c->width;
  source_height = c->height;

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

  c2 = new_canvas (L, canvas_bytes (target_width, target_height));
  c2->width = target_width;
  c2->height = target_height;
  c2->r = c->r;
  c2->g = c->g;
  c2->b = c->b;

  /* pen in center */
  penpos (c2, target_width / 2 - 1, target_height / 2 - 1);
  c2->thickness = c->thickness;

  c2->htextalign = c->htextalign;
  c2->vtextalign = c->vtextalign;

  source = c->data + y1 * source_width * BPP;
  target = c2->data;
  bytes = target_width * BPP;

  for (y = 0; y < target_height; y++)
    memcpy (target + (y * target_width * BPP),
	    source + y * source_width * BPP + x1 * BPP, bytes);

  return 1;
}


static int
lcanvas_duplicate (lua_State * L)
{
  size_t nbytes;
  canvas *c, *c2;

  c = get_canvas (L, 1);
  nbytes = canvas_bytes (c->width, c->height);
  c2 = new_canvas (L, nbytes);
  memcpy (c2, c, nbytes);

  return 1;
}


static const struct luaL_reg canvaslib[] = {
  {"new", lcanvas_new},
  {NULL, NULL}
};


static const struct luaL_reg canvaslib_methods[] = {
  {"clear", lcanvas_clear},
  {"color", lcanvas_color},
  {"rgb", lcanvas_rgb},
  {"eraser", lcanvas_eraser},
  {"thickness", lcanvas_thickness},
  {"putpixel", lcanvas_putpixel},
  {"getpixel", lcanvas_getpixel},
  {"getpixelrgb", lcanvas_getpixelrgb},
  {"line", lcanvas_line},
  {"pen_position", lcanvas_pen_position},
  {"center", lcanvas_center},
  {"moveto", lcanvas_moveto},
  {"moverel", lcanvas_moverel},
  {"lineto", lcanvas_lineto},
  {"linerel", lcanvas_linerel},
  {"putdot", lcanvas_putdot},
  {"bar", lcanvas_bar},
  {"rectangle", lcanvas_rectangle},
  {"circle", lcanvas_circle},
  {"text", lcanvas_text},
  {"textalign", lcanvas_textalign},
  {"font_size", lcanvas_font_size},
  {"show", lcanvas_show},
  {"size", lcanvas_size},
  {"width", lcanvas_width},
  {"height", lcanvas_height},
  {"put", lcanvas_put},
  {"get", lcanvas_get},
  {"duplicate", lcanvas_duplicate},
  {NULL, NULL}
};


int
luaopen_canvas (lua_State * L)
{
  luaL_register (L, "canvas", canvaslib);

  luaL_newmetatable (L, CANVASDATA);
  lua_pushvalue (L, -1);
  lua_setfield (L, -2, "__index");
  luaL_register (L, NULL, canvaslib_methods);
  lua_pop (L, 1);

  return 1;
}
