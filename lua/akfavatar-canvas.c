/*
 * AKFAvatar simple canvas
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

#include "akfavatar.h"
#include <stdlib.h>
#include <string.h>		/* memset */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Bytes per pixel (3=RGB) */
#define BPP 3

#define CANVASDATA "AKFAvatar-canvas"
#define get_canvas()  ((canvas *) luaL_checkudata (L, 1, CANVASDATA))

/* force value to be in range */
#define RANGE(v, min, max)  ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))

#define visible_x(c, x)  ((int) (x) >= 1 && (int) (x) <= (c)->width)
#define visible_y(c, y)  ((int) (y) >= 1 && (int) (y) <= (c)->height)
#define visible(c, x, y)  (visible_x(c, x) && visible_y(c, y))

#define abs(x)  ((x) > 0 ? (x) : -(x))

/* set pen position */
#define penpos(c, x, y)  (c)->penx = (int) (x); (c)->peny = (int) (y)

/* fast putpixel, no check */
#define putpixel(c, x, y) \
  do { \
    unsigned char *p = \
      (c)->data+(((int)(y)-1)*(c)->width*BPP)+(((int)(x)-1)*BPP); \
    *p++=(c)->r; *p++=(c)->g; *p++=(c)->b; \
  } while(0)


typedef struct canvas
{
  int width, height;
  int penx, peny;		/* position of pen */
  int thickness;		/* thickness of pen */
  unsigned char r, g, b;	/* current color */
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
  r = (unsigned char) c->r;
  g = (unsigned char) c->g;
  b = (unsigned char) c->b;

  /* sanitize values */
  x1 = RANGE (x1, 1, width);
  y1 = RANGE (y1, 1, height);
  x2 = RANGE (x2, 1, width);
  y2 = RANGE (y2, 1, height);


  for (y = y1 - 1; y < y2; y++)
    {
      p = data + (y * width * BPP) + ((x1 - 1) * BPP);

      for (x = x1 - 1; x < x2; x++)
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
  size_t nbytes;
  canvas *c;

  width = luaL_optint (L, 1, avt_image_max_width ());
  height = luaL_optint (L, 2, avt_image_max_height ());

  nbytes = sizeof (canvas) - sizeof (unsigned char) + (width * height * BPP);

  c = (canvas *) lua_newuserdata (L, nbytes);
  luaL_getmetatable (L, CANVASDATA);
  lua_setmetatable (L, -2);

  c->width = width;
  c->height = height;

  /* black color */
  c->r = c->g = c->b = 0;

  /* pen in center */
  penpos (c, width / 2, height / 2);
  c->thickness = 1 - 1;

  clear_canvas (c);

  lua_pushinteger (L, width);
  lua_pushinteger (L, height);

  return 3;
}


static int
lcanvas_clear (lua_State * L)
{
  clear_canvas (get_canvas ());
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
      c = get_canvas ();
      c->r = (unsigned char) red;
      c->g = (unsigned char) green;
      c->b = (unsigned char) blue;
    }

  return 0;
}


/* c:thickness(size) */
static int
lcanvas_thickness (lua_State * L)
{
  int s;

  s = luaL_checkint (L, 2) - 1;
  get_canvas ()->thickness = (s < 0) ? 0 : s;

  return 0;
}


/* c:moveto (x, y) */
static int
lcanvas_moveto (lua_State * L)
{
  /* a pen outside the field is allowed! */
  penpos (get_canvas (), luaL_checkint (L, 2), luaL_checkint (L, 3));

  return 0;
}


static void
vertical_line (canvas * c, double x, double y1, double y2)
{
  double y;
  int height;

  if (visible_x (c, x))
    {
      if (y1 > y2)		/* swap */
	{
	  double ty = y1;
	  y1 = y2;
	  y2 = ty;
	}

      height = c->height;
      y1 = RANGE (y1, 1, height);
      y2 = RANGE (y2, 1, height);

      if (c->thickness > 0)
	{
	  for (y = y1; y <= y2; y++)
	    putdot (c, x, y);
	}
      else
	{
	  for (y = y1; y <= y2; y++)
	    if (visible_y (c, y))
	      putpixel (c, x, y);
	}
    }
}


static void
horizontal_line (canvas * c, double x1, double x2, double y)
{
  double x;
  int width;
  unsigned char *p;
  unsigned char r, g, b;

  if (visible_y (c, y))
    {
      if (x1 > x2)		/* swap */
	{
	  double tx = x1;
	  x1 = x2;
	  x2 = tx;
	}

      width = c->width;
      x1 = RANGE (x1, 1, width);
      x2 = RANGE (x2, 1, width);

      if (c->thickness > 0)
	{
	  for (x = x1; x <= x2; x++)
	    putdot (c, (int) x, (int) y);
	}
      else
	{
	  r = c->r;
	  g = c->g;
	  b = c->b;

	  p =
	    c->data + (((int) y - 1) * width * BPP) + (((int) x1 - 1) * BPP);

	  for (x = x1 - 1; x < x2; x++)
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
sloped_line (canvas * c, double x1, double x2, double y1, double y2)
{
  double x, y;
  double dx, dy;

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
      delta_y = dy / dx;

      /* sanitize range of x */
      if (x1 < 1)
	{
	  y1 += delta_y * (1 - x1);
	  x1 = 1;
	}

      if (x2 > c->width)
	x2 = c->width;

      if (c->thickness > 0)
	{
	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    putdot (c, x, y);
	}
      else
	{
	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    {
	      if (visible_y (c, y))
		putpixel (c, x, y);
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
      if (y1 < 1)
	{
	  x1 += delta_x * (1 - y1);
	  y1 = 1;
	}

      if (y2 > c->height)
	y2 = c->height;

      if (c->thickness > 0)
	{
	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    putdot (c, x, y);
	}
      else
	{
	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    {
	      if (visible_x (c, x))
		putpixel (c, x, y);
	    }
	}
    }
}


static void
line (canvas * c, double x1, double y1, double x2, double y2)
{
  if (x1 == x2 && y1 == y2)	/* one pixel */
    {
      if (visible (c, x1, y1))
	putpixel (c, x1, y1);
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
  double x1, y1, x2, y2;

  c = get_canvas ();
  x1 = luaL_checknumber (L, 2);
  y1 = luaL_checknumber (L, 3);
  x2 = luaL_checknumber (L, 4);
  y2 = luaL_checknumber (L, 5);

  line (c, x1, y1, x2, y2);
  penpos (c, x2, y2);

  return 0;
}


/* c:lineto (x, y) */
static int
lcanvas_lineto (lua_State * L)
{
  canvas *c;
  double x2, y2;

  c = get_canvas ();
  x2 = luaL_checknumber (L, 2);
  y2 = luaL_checknumber (L, 3);

  line (c, (double) c->penx, (double) c->peny, x2, y2);
  penpos (c, x2, y2);

  return 0;
}


/* c:putpixel ([x, y]) */
static int
lcanvas_putpixel (lua_State * L)
{
  canvas *c;
  int x, y;

  c = get_canvas ();
  x = luaL_optint (L, 2, c->penx);
  y = luaL_optint (L, 3, c->peny);

  if (visible (c, x, y))
    putpixel (c, x, y);

  return 0;
}


/* c:putdot ([x, y]) */
static int
lcanvas_putdot (lua_State * L)
{
  canvas *c;
  int x, y;

  c = get_canvas ();
  x = luaL_optint (L, 2, c->penx);
  y = luaL_optint (L, 3, c->peny);

  /* macros evaluate the values more than once! */
  if (c->thickness > 0)
    putdot (c, x, y);
  else if (visible (c, x, y))
    putpixel (c, x, y);

  return 0;
}


/* c:bar (x, y, width, height) */
static int
lcanvas_bar (lua_State * L)
{
  canvas *c;
  int x1, y1, x2, y2;

  c = get_canvas ();

  x1 = luaL_checkint (L, 2);
  y1 = luaL_checkint (L, 3);
  x2 = x1 + luaL_checkint (L, 4) - 1;
  y2 = y1 + luaL_checkint (L, 5) - 1;

  bar (c, x1, y1, x2, y2);

  return 0;
}


/* c:rectangle (x, y, width, height) */
static int
lcanvas_rectangle (lua_State * L)
{
  canvas *c;
  double x1, y1, x2, y2;

  c = get_canvas ();

  x1 = luaL_checknumber (L, 2);
  y1 = luaL_checknumber (L, 3);
  x2 = x1 + luaL_checknumber (L, 4) - 1;
  y2 = y1 + luaL_checknumber (L, 5) - 1;

  horizontal_line (c, x1, x2, y1);
  vertical_line (c, x2, y1, y2);
  horizontal_line (c, x2, x1, y2);
  vertical_line (c, x1, y2, y1);

  return 0;
}


static int
lcanvas_show (lua_State * L)
{
  canvas *c = get_canvas ();
  avt_show_raw_image (&c->data, c->width, c->height, BPP);

  return 0;
}


static int
lcanvas_width (lua_State * L)
{
  lua_pushinteger (L, get_canvas ()->width);
  return 1;
}


static int
lcanvas_height (lua_State * L)
{
  lua_pushinteger (L, get_canvas ()->height);
  return 1;
}


static const struct luaL_reg canvaslib[] = {
  {"new", lcanvas_new},
  {NULL, NULL}
};


static const struct luaL_reg canvaslib_methods[] = {
  {"clear", lcanvas_clear},
  {"color", lcanvas_color},
  {"thickness", lcanvas_thickness},
  {"line", lcanvas_line},
  {"moveto", lcanvas_moveto},
  {"lineto", lcanvas_lineto},
  {"putpixel", lcanvas_putpixel},
  {"putdot", lcanvas_putdot},
  {"bar", lcanvas_bar},
  {"rectangle", lcanvas_rectangle},
  {"show", lcanvas_show},
  {"width", lcanvas_width},
  {"height", lcanvas_height},
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
