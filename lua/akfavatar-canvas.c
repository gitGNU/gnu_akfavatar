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
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Bytes per pixel (3=RGB) */
#define BPP 3

#define PI  3.14159265358979323846

#define CANVASDATA "AKFAvatar-canvas"
#define get_canvas()  ((canvas *) luaL_checkudata (L, 1, CANVASDATA))

/* force value to be in range */
#define RANGE(v, min, max)  ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))

#define visible_x(c, x)  ((int) (x) >= 0 && (int) (x) < (c)->width)
#define visible_y(c, y)  ((int) (y) >= 0 && (int) (y) < (c)->height)
#define visible(c, x, y)  (visible_x(c, x) && visible_y(c, y))

/* set pen position */
#define penpos(c, x, y)  (c)->penx = (int) (x); (c)->peny = (int) (y)

/* fast putpixel, no check */
#define putpixel(c, x, y) \
  do { \
    unsigned char *p = \
      (c)->data+(((int)(y))*(c)->width*BPP)+(((int)(x))*BPP); \
    *p++=(c)->r; *p++=(c)->g; *p=(c)->b; \
  } while(0)

/* fast putpixel with rgb (faster), no check */
#define putpixelrgb(c, x, y, r, g, b) \
  do { \
    unsigned char *p = \
      (c)->data+(((int)(y))*(c)->width*BPP)+(((int)(x))*BPP); \
    *p++=(r); *p++=(g); *p=(b); \
  } while(0)

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
  get_canvas ()->thickness = (s > 0) ? s : 0;

  return 0;
}


/* x, y = c:pen_position () */
static int
lcanvas_pen_position (lua_State * L)
{
  canvas *c = get_canvas ();

  lua_pushinteger (L, c->penx + 1);
  lua_pushinteger (L, c->peny + 1);

  return 2;
}


/* c:moveto (x, y) */
static int
lcanvas_moveto (lua_State * L)
{
  canvas *c = get_canvas ();

  /* a pen outside the field is allowed! */
  penpos (c, luaL_checkint (L, 2) - 1, luaL_checkint (L, 3) - 1);

  return 0;
}


/* c:moverel (x, y) */
static int
lcanvas_moverel (lua_State * L)
{
  canvas *c = get_canvas ();

  penpos (c, c->penx + luaL_checkint (L, 2), c->peny + luaL_checkint (L, 3));

  return 0;
}


/* c:center (x, y) */
static int
lcanvas_center (lua_State * L)
{
  canvas *c = get_canvas ();

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
	  for (y = y1; y <= y2; y++)
	    if (visible_y (c, y))
	      putpixelrgb (c, x, y, r, g, b);
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
sloped_line (canvas * c, double x1, double x2, double y1, double y2)
{
  double x, y;
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
      if (x1 < 0)
	{
	  y1 -= delta_y * x1;
	  x1 = 0;
	}

      if (x2 >= c->width)
	x2 = c->width - 1;

      if (c->thickness > 0)
	{
	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    putdot (c, x, y);
	}
      else
	{
	  unsigned char r = c->r, g = c->g, b = c->b;
	  for (x = x1, y = y1; x <= x2; x++, y += delta_y)
	    {
	      if (visible_y (c, y))
		putpixelrgb (c, x, y, r, g, b);
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
      if (y1 < 0)
	{
	  x1 -= delta_x * y1;
	  y1 = 0;
	}

      if (y2 >= c->height)
	y2 = c->height - 1;

      if (c->thickness > 0)
	{
	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    putdot (c, x, y);
	}
      else
	{
	  unsigned char r = c->r, g = c->g, b = c->b;
	  for (y = y1, x = x1; y <= y2; y++, x += delta_x)
	    {
	      if (visible_x (c, x))
		putpixelrgb (c, x, y, r, g, b);
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

  c = get_canvas ();
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

  c = get_canvas ();
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

  c = get_canvas ();
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

  c = get_canvas ();
  x = luaL_optint (L, 2, c->penx + 1) - 1;
  y = luaL_optint (L, 3, c->peny + 1) - 1;

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

  c = get_canvas ();

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

  c = get_canvas ();

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

  c = get_canvas ();

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

  c = get_canvas ();
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
		  putpixelrgb (c, x + lx, y + ly, r, g, b);
	      font_line++;
	    }
	}
    }
  else				/* fontwidth <= 8 */
    {
      unsigned char r = c->r, g = c->g, b = c->b;

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
		  putpixelrgb (c, x + lx, y + ly, r, g, b);
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
  const char *ha, *va;

  c = get_canvas ();
  ha = luaL_optstring (L, 2, "center");
  va = luaL_optstring (L, 3, "center");

  if (strcmp ("left", ha) == 0)
    c->htextalign = HA_LEFT;
  else if (strcmp ("center", ha) == 0)
    c->htextalign = HA_CENTER;
  else if (strcmp ("right", ha) == 0)
    c->htextalign = HA_RIGHT;
  else
    luaL_error (L, "\"%s\" unsupported,\n"
		"expected either of \"left\", \"center\", \"right\"", ha);

  if (strcmp ("top", va) == 0)
    c->vtextalign = VA_TOP;
  else if (strcmp ("center", va) == 0)
    c->vtextalign = VA_CENTER;
  else if (strcmp ("bottom", va) == 0)
    c->vtextalign = VA_BOTTOM;
  else
    luaL_error (L, "\"%s\" unsupported,\n"
		"expected either of \"top\", \"center\", \"bottom\"", va);

  return 0;
}


static const struct luaL_reg canvaslib[] = {
  {"new", lcanvas_new},
  {NULL, NULL}
};


static const struct luaL_reg canvaslib_methods[] = {
  {"clear", lcanvas_clear},
  {"color", lcanvas_color},
  {"thickness", lcanvas_thickness},
  {"putpixel", lcanvas_putpixel},
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
