/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99 or C++
 *
 * other software:
 * required:
 *  SDL1.2 (recommended: SDL1.2.11 or later (but not 1.3!))
 * optional:
 *  SDL_image1.2
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

/* don't make functions deprecated for this file */
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "SDL.h"
#include "version.h"
#include "rgb.h"

/* include images */
#include "akfavatar.xpm"
#include "btn.xpm"
#include "balloonpointer.xbm"
#include "circle.xbm"
#include "btn_yes.xbm"
#include "btn_no.xbm"
#include "btn_right.xbm"
#include "btn_left.xbm"
#include "btn_up.xbm"
#include "btn_down.xbm"
#include "btn_cancel.xbm"
#include "btn_ff.xbm"
#include "btn_fb.xbm"
#include "btn_stop.xbm"
#include "btn_pause.xbm"
#include "btn_help.xbm"
#include "btn_eject.xbm"
#include "btn_circle.xbm"
#include "mpointer.xbm"
#include "mpointer_mask.xbm"

#ifdef LINK_SDL_IMAGE
#  include "SDL_image.h"
#endif

#define COPYRIGHTYEAR "2012"

/*
 * Most iconv implementations support "" for the systems encoding.
 * You should redefine this macro if and only if it is not supported.
 */
#ifndef SYSTEMENCODING
#  define SYSTEMENCODING  ""
#endif

#define BUTTON_DISTANCE 10

/* normal color of what's printed on the button */
#define BUTTON_COLOR  0x66, 0x55, 0x33
#define XBM_DEFAULT_COLOR  0x00, 0x00, 0x00

#define AVT_XBM_INFO(img)  img##_bits, img##_width, img##_height

#if defined(VGA)
#  define FONTWIDTH 7
#  define FONTHEIGHT 14
#  define UNDERLINE 13
#  define LINEHEIGHT FONTHEIGHT	/* + something, if you want */
#  define NOT_BOLD 0
#else
#  define FONTWIDTH 9
#  define FONTHEIGHT 18
#  define UNDERLINE 15
#  define LINEHEIGHT FONTHEIGHT	/* + something, if you want */
#  define NOT_BOLD 0
#endif

#define SHADOWOFFSET 5

/*
 * newer vesions of SDL have some fallback implementations
 * for libc functionality - I try to use it, if available
 * This is the fallback for older SDL versions.
 */
#ifndef _SDL_stdinc_h
#  define OLD_SDL 1
#endif

#ifdef OLD_SDL
#  warning "compiling for old SDL - using libc directly"
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <ctype.h>

#  undef SDL_malloc
#  define SDL_malloc              malloc
#  undef SDL_calloc
#  define SDL_calloc              calloc
#  undef SDL_free
#  define SDL_free                free
#  undef SDL_strlen
#  define SDL_strlen              strlen
#  undef SDL_strstr
#  define SDL_strstr              strstr
#  undef SDL_strdup
#  define SDL_strdup              strdup
#  undef SDL_atoi
#  define SDL_atoi                atoi
#  undef SDL_strtol
#  define SDL_strtol              strtol
#  undef SDL_memcpy
#  define SDL_memcpy              memcpy
#  undef SDL_memset
#  define SDL_memset              memset
#  undef SDL_memcmp
#  define SDL_memcmp              memcmp
#  undef SDL_strcasecmp
#  define SDL_strcasecmp          strcasecmp
#  undef SDL_strncasecmp
#  define SDL_strncasecmp         strncasecmp
#  undef SDL_putenv
#  define SDL_putenv              putenv
#  undef SDL_sscanf
#  define SDL_sscanf              sscanf
#endif /* OLD_SDL */

/* Note errno is only used for iconv and may not be the external errno! */

#ifndef USE_SDL_ICONV
#  include <errno.h>
#  include <iconv.h>
#  define avt_iconv_t             iconv_t
#  define avt_iconv_open          iconv_open
#  define avt_iconv_close         iconv_close
#  define avt_iconv               iconv
#else /* USE_SDL_ICONV */
static int errno;
#  define avt_iconv_t             SDL_iconv_t
#  define avt_iconv_open          SDL_iconv_open
#  define avt_iconv_close         SDL_iconv_close
   /* avt_iconv implemented below */
#endif /* USE_SDL_ICONV */

/* don't use any libc commands directly! */
#pragma GCC poison  malloc calloc free strlen memcpy memset getenv putenv
#pragma GCC poison  strstr atoi atol strtol
/* do not poison the iconv stuff, it causes problems with external libiconv */


#define COLORDEPTH 24

#if defined(VGA)
#  define MINIMALWIDTH 640
#  define MINIMALHEIGHT 480
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 10
#  define AVATAR_MARGIN 10
   /* Delay for moving in or out - the higher, the slower */
#  define MOVE_DELAY 2.5
#else
#  define MINIMALWIDTH 800
#  define MINIMALHEIGHT 600
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 15
#  define AVATAR_MARGIN 20
   /* Delay for moving in or out - the higher, the slower */
#  define MOVE_DELAY 1.8
#endif /* !VGA */

#define BALLOONPOINTER_OFFSET 20

#define ICONV_UNINITIALIZED   (avt_iconv_t)(-1)

/* try to guess WCHAR_ENCODING,
 * based on WCHAR_MAX or __WCHAR_MAX__ if it is available
 * note: newer SDL versions include stdint.h if available
 */
#if !defined(WCHAR_MAX) && defined(__WCHAR_MAX__)
#  define WCHAR_MAX __WCHAR_MAX__
#endif

#ifndef WCHAR_ENCODING

#  ifndef WCHAR_MAX
#    error "please define WCHAR_ENCODING (no autodetection possible)"
#  endif

#  if (WCHAR_MAX <= 255)
#    define WCHAR_ENCODING "ISO-8859-1"
#  elif (WCHAR_MAX <= 65535U)
#    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#      define WCHAR_ENCODING "UTF-16BE"
#    else /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#      define WCHAR_ENCODING "UTF-16LE"
#    endif /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#  else	/* (WCHAR_MAX > 65535U) */
#    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#      define WCHAR_ENCODING "UTF-32BE"
#    else /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#      define WCHAR_ENCODING "UTF-32LE"
#    endif /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#  endif /* (WCHAR_MAX > 65535U) */

#endif /* not WCHAR_ENCODING */

/*
 * this will be used, when somebody forgets to set the
 * encoding
 */
#define MB_DEFAULT_ENCODING "UTF-8"

/* only defined in later SDL versions */
#ifndef SDL_BUTTON_WHEELUP
#  define SDL_BUTTON_WHEELUP 4
#endif

#ifndef SDL_BUTTON_WHEELDOWN
#  define SDL_BUTTON_WHEELDOWN 5
#endif

/* shorthand */
#define bell(void)  if (avt_alert_func) (*avt_alert_func)()

#define avt_isblank(c)  ((c) == ' ' || (c) == '\t')


/* type for gimp images */
#ifndef DISABLE_DEPRECATED
typedef struct
{
  unsigned int width;
  unsigned int height;
  unsigned int bytes_per_pixel;	/* 3:RGB, 4:RGBA */
  unsigned char pixel_data;	/* handle as startpoint */
} gimp_img_t;
#endif

/* for an external keyboard/mouse handlers */
static avt_keyhandler avt_ext_keyhandler = NULL;
static avt_mousehandler avt_ext_mousehandler = NULL;

static SDL_Surface *screen, *avatar_image, *avt_character;
static SDL_Surface *avt_text_cursor, *avt_cursor_character;
static SDL_Surface *circle, *pointer;
static SDL_Cursor *mpointer;
static wchar_t *avt_name;
static Uint32 background_color;
static Uint32 text_background_color;
static bool newline_mode;	/* when off, you need an extra CR */
static bool underlined, bold, inverse;	/* text underlined, bold? */
static bool auto_margin;	/* automatic new lines? */
static Uint32 screenflags;	/* flags for the screen */
static int avt_mode;		/* whether fullscreen or window or ... */
static SDL_Rect window;		/* if screen is in fact larger */
static SDL_Rect windowmode_size;	/* size of the whole window (screen) */
static bool avt_visible;	/* avatar visible? */
static bool text_cursor_visible;	/* shall the text cursor be visible? */
static bool text_cursor_actually_visible;	/* is it actually visible? */
static bool reserve_single_keys;	/* reserve single keys? */
static bool markup;		/* markup-syntax activated? */
static int scroll_mode = 1;
static SDL_Rect textfield;
static SDL_Rect viewport;	/* sub-window in textfield */
static bool avt_tab_stops[AVT_LINELENGTH];
static char avt_encoding[100];

/* origin mode */
/* Home: textfield (false) or viewport (true) */
/* avt_initialize sets it to true for backwards compatibility */
static bool origin_mode;
static int textdir_rtl = AVT_LEFT_TO_RIGHT;

/* beginning of line - depending on text direction */
static int linestart;
static int balloonheight, balloonmaxheight, balloonwidth;

/* delay values for printing text and flipping the page */
static int text_delay = 0;	/* AVT_DEFAULT_TEXT_DELAY */
static int flip_page_delay = AVT_DEFAULT_FLIP_PAGE_DELAY;

/* holding updates back? */
static bool hold_updates;

/* color independent from the screen mode */

/* floral white */
static SDL_Color ballooncolor_RGB = { 255, 250, 240, 0 };

/* default (pale brown) */
static SDL_Color backgroundcolor_RGB = { 0xE0, 0xD5, 0xC5, 0 };

/* grey */
/* static SDL_Color backgroundcolor_RGB = { 0xCC, 0xCC, 0xCC, 0 }; */

/* color for cursor and menu-bar */
static SDL_Color cursor_color = { 0xF2, 0x89, 0x19, 0 };

/* conversion descriptors for text input and output */
static avt_iconv_t output_cd = ICONV_UNINITIALIZED;
static avt_iconv_t input_cd = ICONV_UNINITIALIZED;

struct pos
{
  int x, y;
};

static struct pos cursor, saved_position;


#if defined(__GNUC__) && !defined(__WIN32__)
#  define AVT_HIDDEN __attribute__((__visibility__("hidden")))
#else
#  define AVT_HIDDEN
#endif /* __GNUC__ */

/* 0 = normal; 1 = quit-request; -1 = error */
int _avt_STATUS AVT_HIDDEN;

void (*avt_alert_func) (void) AVT_HIDDEN = NULL;
void (*avt_quit_audio_func) (void) AVT_HIDDEN = NULL;

/* forward declaration */
static int avt_pause (void);
static void avt_drawchar (avt_char ch, SDL_Surface * surface);


/* color selector */
extern int
avt_name_to_color (const char *name, int *red, int *green, int *blue)
{
  int status;

  if (!name || !*name || !red || !green || !blue)
    return -1;

  status = -1;
  *red = *green = *blue = -1;

  /* skip space */
  while (avt_isblank (*name))
    name++;

  if (name[0] == '#')		/* hexadecimal values */
    {
      unsigned int r, g, b;

      if (SDL_sscanf (name, " #%2x%2x%2x", &r, &g, &b) == 3)
	{
	  *red = r;
	  *green = g;
	  *blue = b;
	  status = 0;
	}
      else if (SDL_sscanf (name, " #%1x%1x%1x", &r, &g, &b) == 3)
	{
	  *red = r << 4 | r;
	  *green = g << 4 | g;
	  *blue = b << 4 | b;
	  status = 0;
	}
    }
  else if (name[0] == '%')	/* HSV values not supported */
    status = -1;
  else				/* look up color table */
    {
      int i;
      const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

      for (i = 0; i < numcolors && status != 0; i++)
	{
	  if (SDL_strcasecmp (avt_colors[i].color_name, name) == 0)
	    {
	      *red = avt_colors[i].red;
	      *green = avt_colors[i].green;
	      *blue = avt_colors[i].blue;
	      status = 0;
	    }
	}
    }

  return status;
}

extern const char *
avt_get_color_name (int nr)
{
  const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

  if (nr >= 0 && nr < numcolors)
    return avt_colors[nr].color_name;
  else
    return NULL;
}

extern const char *
avt_get_color (int nr, int *red, int *green, int *blue)
{
  const int numcolors = sizeof (avt_colors) / sizeof (avt_colors[0]);

  if (nr >= 0 && nr < numcolors)
    {
      if (red)
	*red = avt_colors[nr].red;
      if (green)
	*green = avt_colors[nr].green;
      if (blue)
	*blue = avt_colors[nr].blue;

      return avt_colors[nr].color_name;
    }
  else
    return NULL;
}

/* for dynamically loading SDL_image */
#ifndef AVT_SDL_IMAGE_LIB
#  if defined (__WIN32__)
#    define AVT_SDL_IMAGE_LIB "SDL_image.dll"
#  else	/* not Windows */
#    define AVT_SDL_IMAGE_LIB "libSDL_image-1.2.so.0"
#  endif /* not Windows */
#endif /* not AVT_SDL_IMAGE_LIB */

/*
 * object for image-loading
 * SDL_image can be dynamically loaded (SDL-1.2.6 or better)
 */
static struct
{
  bool initialized;
  void *handle;			/* handle for dynamically loaded SDL_image */
  SDL_Surface *(*rw) (SDL_RWops * src, int freesrc);
} load_image;

#define AVT_UPDATE_RECT(rect) \
  SDL_UpdateRect(screen, rect.x, rect.y, rect.w, rect.h)

#define AVT_UPDATE_TRECT(rect) \
  if (!hold_updates) AVT_UPDATE_RECT(rect)

#define AVT_UPDATE_ALL(void) SDL_UpdateRect(screen, 0, 0, 0, 0)


/* X-Pixmap (XPM) support */

/* number of printable ASCII codes */
#define XPM_NR_CODES (126 - 32 + 1)

/* for xpm codes */
union xpm_codes
{
  Uint32 nr;
  union xpm_codes *next;
};

static void
avt_free_xpm_tree (union xpm_codes *tree, int depth, int cpp)
{
  int i;
  union xpm_codes *e;

  if (depth < cpp)
    {
      for (i = 0; i < XPM_NR_CODES; i++)
	{
	  e = (tree + i)->next;
	  if (e != NULL)
	    avt_free_xpm_tree (e, depth + 1, cpp);
	}
    }

  SDL_free (tree);
}

/* use this for internal stuff! */
static SDL_Surface *
avt_load_image_xpm (char **xpm)
{
  SDL_Surface *img;
  int width, height, ncolors, cpp;
  int colornr;
  union xpm_codes *codes;
  Uint32 *colors;
  SDL_Color *colors256;
  int code_nr;

  codes = NULL;
  colors = NULL;
  colors256 = NULL;
  img = NULL;

  /* check if we actually have data to process */
  if (!xpm || !*xpm)
    goto done;

  /* read value line
   * there may be more values in the line, but we just
   * need the first four
   */
  if (SDL_sscanf (xpm[0], "%d %d %d %d", &width, &height, &ncolors, &cpp) < 4
      || width < 1 || height < 1 || ncolors < 1)
    {
      SDL_SetError ("error in XPM data");
      goto done;
    }

  /* create target surface */
  if (ncolors <= 256)
    img = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);
  else
    {
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	img = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 32,
				    0xFF0000, 0x00FF00, 0x0000FF, 0);
      else
	img = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 32,
				    0x0000FF, 0x00FF00, 0xFF0000, 0);
    }

  if (!img)
    {
      SDL_SetError ("out of memory");
      goto done;
    }

  /* get memory for codes table */
  if (cpp > 1)
    {
      codes = (union xpm_codes *) SDL_calloc (XPM_NR_CODES, sizeof (codes));
      if (!codes)
	{
	  SDL_SetError ("out of memory");
	  SDL_free (img);
	  img = NULL;
	  goto done;
	}
    }

  /* get memory for colors table (palette) */
  if (ncolors <= 256)
    colors256 = (SDL_Color *) SDL_calloc (256, sizeof (SDL_Color));
  else
    colors = (Uint32 *) SDL_calloc (ncolors, sizeof (Uint32));

  /*
   * note: for colors256 the colors will be scattered around the palette
   * so we need a full sized palette
   *
   * colors is a different type so we have to call SDL_MapRGB only once
   * for each color
   */

  if (!colors && !colors256)
    {
      SDL_SetError ("out of memory");
      SDL_free (img);
      img = NULL;
      goto done;
    }

  code_nr = 0;

  /* process colors */
  for (colornr = 1; colornr <= ncolors; colornr++, code_nr++)
    {
      char *p;			/* pointer for scanning through the string */

      if (xpm[colornr] == NULL)
	{
	  SDL_SetError ("error in XPM data");
	  SDL_free (img);
	  img = NULL;
	  goto done;
	}

      /* if there is only one character per pixel,
       * the character is the palette number
       */
      if (cpp == 1)
	code_nr = xpm[colornr][0];
      else			/* store characters in codes table */
	{
	  int i;
	  char c;
	  union xpm_codes *table;

	  c = '\0';
	  table = codes;
	  for (i = 0; i < cpp - 1; i++)
	    {
	      c = xpm[colornr][i];

	      if (c < 32 || c > 126)
		break;

	      table = (table + (c - 32));

	      if (!table->next)
		table->next =
		  (union xpm_codes *) SDL_calloc (XPM_NR_CODES,
						  sizeof (*codes));

	      table = table->next;
	    }

	  if (c < 32 || c > 126)
	    break;

	  c = xpm[colornr][cpp - 1];

	  if (c < 32 || c > 126)
	    break;

	  (table + (c - 32))->nr = colornr - 1;
	}

      /* scan for color definition */
      p = &xpm[colornr][cpp];	/* skip color-characters */
      while (*p && (*p != 'c' || !avt_isblank (*(p + 1))
		    || !avt_isblank (*(p - 1))))
	p++;

      /* no color definition found? search for grayscale definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p && (*p != 'g' || !avt_isblank (*(p + 1))
			|| !avt_isblank (*(p - 1))))
	    p++;
	}

      /* no grayscale definition found? search for g4 definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p
		 && (*p != '4' || *(p - 1) != 'g' || !avt_isblank (*(p + 1))
		     || !avt_isblank (*(p - 2))))
	    p++;
	}

      /* search for monochrome definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p && (*p != 'm' || !avt_isblank (*(p + 1))
			|| !avt_isblank (*(p - 1))))
	    p++;
	}

      if (*p)
	{
	  int red, green, blue;
	  int color_name_pos;
	  char color_name[80];

	  /* skip to color name/definition */
	  p++;
	  while (*p && avt_isblank (*p))
	    p++;

	  /* copy colorname up to next space */
	  color_name_pos = 0;
	  while (*p && !avt_isblank (*p)
		 && color_name_pos < (int) sizeof (color_name) - 1)
	    color_name[color_name_pos++] = *p++;
	  color_name[color_name_pos] = '\0';

	  if (SDL_strcasecmp (color_name, "None") == 0)
	    {
	      SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL, code_nr);

	      /* some weird color, that hopefully doesn't conflict (#1A2A3A) */
	      red = 0x1A;
	      green = 0x2A;
	      blue = 0x3A;
	    }
	  else
	    {
	      avt_name_to_color (color_name, &red, &green, &blue);
	      /* no check, because it couldn't do anything usefull anyway */
	      /* and it shoudn't break on broken image-files */
	    }

	  if (ncolors <= 256)
	    {
	      colors256[code_nr].r = red;
	      colors256[code_nr].g = green;
	      colors256[code_nr].b = blue;
	    }
	  else			/* ncolors > 256 */
	    {
	      *(colors + colornr - 1) =
		SDL_MapRGB (img->format, red, green, blue);
	    }
	}
    }

  /* put colormap into the image */
  if (ncolors <= 256)
    {
      SDL_SetPalette (img, SDL_LOGPAL, colors256, 0, 256);
      SDL_free (colors256);
    }

  /* process pixeldata */
  if (SDL_MUSTLOCK (img))
    SDL_LockSurface (img);
  if (cpp == 1)			/* the easiest case */
    {
      int line;

      for (line = 0; line < height; line++)
	{
	  /* check for premture end of data */
	  if (xpm[ncolors + 1 + line] == NULL)
	    break;

	  SDL_memcpy ((Uint8 *) img->pixels + (line * img->pitch),
		      xpm[ncolors + 1 + line], width);
	}
    }
  else				/* cpp != 1 */
    {
      Uint8 *pix;
      char *xpm_line;
      int line;
      int bpp;
      int pos;
      int i;

      /* Bytes per Pixel of img */
      bpp = img->format->BytesPerPixel;

      for (line = 0; line < height; line++)
	{
	  /* point to beginning of the line */
	  pix = (Uint8 *) img->pixels + (line * img->pitch);
	  xpm_line = xpm[ncolors + 1 + line];

	  /* check for premture end of data */
	  if (xpm_line == NULL)
	    break;

	  for (pos = 0; pos < width; pos++, pix += bpp)
	    {
	      union xpm_codes *table;
	      char c;

	      c = '\0';
	      /* find code in codes table */
	      table = codes;
	      for (i = 0; i < cpp - 1; i++)
		{
		  c = xpm_line[pos * cpp + i];
		  if (c < 32 || c > 126)
		    break;
		  table = (table + (c - 32))->next;
		}

	      if (c < 32 || c > 126)
		break;

	      c = xpm_line[pos * cpp + cpp - 1];
	      if (c < 32 || c > 126)
		break;

	      code_nr = (table + (c - 32))->nr;

	      if (ncolors <= 256)
		*pix = code_nr;
	      else
		*(Uint32 *) pix = *(colors + code_nr);
	    }
	}
    }
  if (SDL_MUSTLOCK (img))
    SDL_UnlockSurface (img);

done:
  if (colors)
    SDL_free (colors);

  /* clean up codes table */
  if (codes)
    avt_free_xpm_tree (codes, 1, cpp);

  return img;
}

static SDL_Surface *
avt_load_image_xpm_RW (SDL_RWops * src, int freesrc)
{
  int start;
  char head[9];
  char **xpm;
  char *line;
  unsigned int linepos, linenr, linecount, linecapacity;
  SDL_Surface *img;
  char c;
  bool end, error;

  if (!src)
    return NULL;

  img = NULL;
  xpm = NULL;
  line = NULL;
  end = error = false;

  start = SDL_RWtell (src);

  /* check if it has an XPM header */
  if (SDL_RWread (src, head, sizeof (head), 1) < 1
      || SDL_memcmp (head, "/* XPM */", 9) != 0)
    {
      if (freesrc)
	SDL_RWclose (src);
      else
	SDL_RWseek (src, start, RW_SEEK_SET);

      return NULL;
    }

  linenr = linepos = 0;

  linecapacity = 100;
  line = (char *) SDL_malloc (linecapacity);
  if (!line)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  linecount = 512;		/* can be extended later */
  xpm = (char **) SDL_malloc (linecount * sizeof (*xpm));
  if (!xpm)
    {
      SDL_SetError ("out of memory");
      error = end = true;
    }

  while (!end)
    {
      /* skip to next quote */
      do
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    end = true;
	}
      while (!end && c != '"');

      /* read line */
      linepos = 0;
      c = '\0';
      while (!end && c != '"')
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = true;	/* shouldn't happen here */

	  if (c != '"')
	    line[linepos++] = c;

	  if (linepos >= linecapacity)
	    {
	      linecapacity += 100;
	      line = (char *) SDL_realloc (line, linecapacity);
	      if (!line)
		error = end = true;
	    }
	}

      /* copy line */
      if (!end)
	{
	  line[linepos++] = '\0';
	  xpm[linenr] = (char *) SDL_malloc (linepos);
	  SDL_memcpy (xpm[linenr], line, linepos);
	  linenr++;
	  if (linenr >= linecount)	/* leave one line reserved */
	    {
	      linecount += 512;
	      xpm = (char **) SDL_realloc (xpm, linecount * sizeof (*xpm));
	      if (!xpm)
		error = end = true;
	    }
	}
    }

  /* last line must be NULL,
   * so premature end of data can be detected later
   */
  if (xpm)
    xpm[linenr] = NULL;

  if (!error)
    img = avt_load_image_xpm (xpm);

  /* free xpm */
  if (xpm)
    {
      unsigned int i;

      /* linenr points to next (uninitialized) line */
      for (i = 0; i < linenr; i++)
	SDL_free (xpm[i]);

      SDL_free (xpm);
    }

  if (line)
    SDL_free (line);

  if (freesrc)
    SDL_RWclose (src);

  return img;
}

/*
 * loads an X-Bitmap (XBM) with a given color as foreground
 * and a transarent background
 */
static SDL_Surface *
avt_load_image_xbm (const unsigned char *bits, int width, int height,
		    int red, int green, int blue)
{
  SDL_Surface *img;
  SDL_Color color[2];
  int y;
  int bpl;			/* Bytes per line */
  Uint8 *line;

  img = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 1, 0, 0, 0, 0);

  if (!img)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  /* Bytes per line */
  bpl = (width + 7) / 8;

  if (SDL_MUSTLOCK (img))
    SDL_LockSurface (img);

  line = (Uint8 *) img->pixels;

  for (y = 0; y < height; y++)
    {
      int byte;

      for (byte = 0; byte < bpl; byte++)
	{
	  Uint8 val, res;
	  int bit;

	  val = *bits;
	  res = 0;

	  /* bits must be reversed */
	  for (bit = 7; bit >= 0; bit--)
	    {
	      res |= (val & 1) << bit;
	      val >>= 1;
	    }

	  line[byte] = res;
	  bits++;
	}

      line += img->pitch;
    }

  if (SDL_MUSTLOCK (img))
    SDL_UnlockSurface (img);

  color[0].r = ~red;
  color[0].g = ~green;
  color[0].b = ~blue;
  color[1].r = red;
  color[1].g = green;
  color[1].b = blue;
  SDL_SetPalette (img, SDL_LOGPAL, color, 0, 2);
  SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);

  return img;
}

static SDL_Surface *
avt_load_image_xbm_RW (SDL_RWops * src, int freesrc,
		       int red, int green, int blue)
{
  unsigned char *bits;
  int width, height;
  int start;
  unsigned int bytes, bmpos;
  char line[1024];
  SDL_Surface *img;
  bool end, error;
  bool X10;

  if (!src)
    return NULL;

  img = NULL;
  bits = NULL;
  X10 = false;
  end = error = false;
  width = height = bytes = bmpos = 0;

  start = SDL_RWtell (src);

  /* check if it starts with #define */
  if (SDL_RWread (src, line, 1, sizeof (line) - 1) < 1
      || SDL_memcmp (line, "#define", 7) != 0)
    {
      if (freesrc)
	SDL_RWclose (src);
      else
	SDL_RWseek (src, start, RW_SEEK_SET);

      return NULL;
    }

  /* make it usable as a string */
  line[sizeof (line) - 1] = '\0';

  /* search for width and height */
  {
    char *p;
    p = SDL_strstr (line, "_width ");
    if (p)
      width = SDL_atoi (p + 7);
    else
      error = end = true;

    p = SDL_strstr (line, "_height ");
    if (p)
      height = SDL_atoi (p + 8);
    else
      error = end = true;

    if (SDL_strstr (line, " short ") != NULL)
      X10 = true;
  }

  if (error)
    goto done;

  if (width && height)
    {
      bytes = ((width + 7) / 8) * height;
      /* one byte larger for safety with old X10 format */
      bits = (unsigned char *) SDL_malloc (bytes + 1);
    }

  /* this catches different errors */
  if (!bits)
    {
      SDL_SetError ("out of memory");
      error = end = true;
    }

  /* search start of bitmap part */
  if (!end && !error)
    {
      char c;

      SDL_RWseek (src, start, RW_SEEK_SET);

      do
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = true;
	}
      while (c != '{' && !error);

      if (error)		/* no '{' found */
	goto done;

      /* skip newline */
      SDL_RWread (src, &c, sizeof (c), 1);
    }

  while (!end && !error)
    {
      char c;
      unsigned int linepos;

      /* read line */
      linepos = 0;
      c = '\0';
      while (!end && linepos < sizeof (line) && c != '\n')
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = true;

	  if (c != '\n' && c != '}')
	    line[linepos++] = c;

	  if (c == '}')
	    end = true;
	}
      line[linepos] = '\0';

      /* parse line */
      if (line[0] != '\0')
	{
	  char *p;
	  char *endptr;
	  long value;
	  bool end_of_line;

	  p = line;
	  end_of_line = false;
	  while (!end_of_line && bmpos < bytes)
	    {
	      value = SDL_strtol (p, &endptr, 0);
	      if (endptr == p)
		end_of_line = true;
	      else
		{
		  if (!X10)
		    bits[bmpos++] = value;
		  else		/* X10 */
		    {
		      unsigned short *v;
		      /* image is assumed to be in native endianess */
		      v = (unsigned short *) (bits + bmpos);
		      *v = value;
		      bmpos += sizeof (*v);
		    }

		  p = endptr + 1;	/* skip comma */
		}
	    }
	}
    }

  if (!error)
    img = avt_load_image_xbm (bits, width, height, red, green, blue);

done:
  /* free bits */
  if (bits)
    SDL_free (bits);

  if (freesrc)
    SDL_RWclose (src);
  else if (error)
    SDL_RWseek (src, start, RW_SEEK_SET);

  return img;
}

#ifdef LINK_SDL_IMAGE

/*
 * assign functions from linked SDL_image
 */
static void
load_image_initialize (void)
{
  if (!load_image.initialized)
    {
      load_image.handle = NULL;
      load_image.rw = IMG_Load_RW;

      load_image.initialized = true;
    }
}

/* speedup */
#define load_image_init(void) \
  if (!load_image.initialized) \
    load_image_initialize()

#define load_image_done(void)	/* empty */

#else /* ! LINK_SDL_IMAGE */

/* helper functions */

static SDL_Surface *
avt_load_image_RW (SDL_RWops * src, int freesrc)
{
  SDL_Surface *img;

  img = NULL;

  if (src)
    {
      img = SDL_LoadBMP_RW (src, 0);

      if (img == NULL)
	img = avt_load_image_xpm_RW (src, 0);

      if (img == NULL)
	img = avt_load_image_xbm_RW (src, 0, XBM_DEFAULT_COLOR);

      if (freesrc)
	SDL_RWclose (src);
    }

  return img;
}


/*
 * try to load the library SDL_image dynamically
 * (XPM and uncompressed BMP files can always be loaded)
 */
static void
load_image_initialize (void)
{
  if (!load_image.initialized)	/* avoid loading it twice! */
    {
      /* first load defaults from plain SDL */
      load_image.handle = NULL;
      load_image.rw = avt_load_image_RW;

#ifndef NO_SDL_IMAGE
/* loadso.h is only available with SDL 1.2.6 or higher */
#ifdef _SDL_loadso_h
      load_image.handle = SDL_LoadObject (AVT_SDL_IMAGE_LIB);
      if (load_image.handle)
	{
	  load_image.rw =
	    (SDL_Surface * (*)(SDL_RWops *, int))
	    SDL_LoadFunction (load_image.handle, "IMG_Load_RW");
	}
#endif /* _SDL_loadso_h */
#endif /* NO_SDL_IMAGE */

      load_image.initialized = true;
    }
}

/* speedup */
#define load_image_init(void) \
  if (!load_image.initialized) \
    load_image_initialize()

#ifndef _SDL_loadso_h
#  define load_image_done(void)	/* empty */
#else /* _SDL_loadso_h */
static void
load_image_done (void)
{
  if (load_image.handle)
    {
      SDL_UnloadObject (load_image.handle);
      load_image.handle = NULL;
      load_image.rw = NULL;
      load_image.initialized = false;	/* try again next time */
    }
}
#endif /* _SDL_loadso_h */

#endif /* ! LINK_SDL_IMAGE */

#ifdef USE_SDL_ICONV
static size_t
avt_iconv (avt_iconv_t cd,
	   char **inbuf, size_t * inbytesleft,
	   char **outbuf, size_t * outbytesleft)
{
  size_t r;

  r = SDL_iconv (cd, inbuf, inbytesleft, outbuf, outbytesleft);

  switch (r)
    {
    case SDL_ICONV_E2BIG:
      errno = E2BIG;
      r = (size_t) (-1);
      break;

    case SDL_ICONV_EILSEQ:
      errno = EILSEQ;
      r = (size_t) (-1);
      break;

    case SDL_ICONV_EINVAL:
      errno = EINVAL;
      r = (size_t) (-1);
      break;

    case SDL_ICONV_ERROR:
      errno = EBADMSG;		/* ??? */
      r = (size_t) (-1);
      break;
    }

  return r;
}
#endif /* USE_SDL_ICONV */


extern const char *
avt_version (void)
{
  return AVTVERSIONSTR;
}

extern const char *
avt_copyright (void)
{
  return "Copyright (c) " COPYRIGHTYEAR " Andreas K. Foerster";
}

extern const char *
avt_license (void)
{
  return "License GPLv3+: GNU GPL version 3 or later "
    "<http://gnu.org/licenses/gpl.html>";
}

extern bool
avt_initialized (void)
{
  return (screen != NULL);
}

extern int
avt_get_status (void)
{
  return _avt_STATUS;
}

extern void
avt_set_status (int status)
{
  _avt_STATUS = status;
}

/* taken from the SDL documentation */
/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static Uint32
getpixel (SDL_Surface * surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
    {
    case 1:
      return *p;

    case 2:
      return *(Uint16 *) p;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	return p[0] << 16 | p[1] << 8 | p[2];
      else
	return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
      return *(Uint32 *) p;

    default:
      return 0;			/* shouldn't happen, but avoids warnings */
    }
}

/* avoid using the libc */
static int
avt_strwidth (const wchar_t * m)
{
  int l = 0;

  while (*m != 0)
    {
      m++;
      l++;
    }
  return l;
}

/* shows or clears the text cursor in the current position */
/* note: this function is rather time consuming */
static void
avt_show_text_cursor (bool on)
{
  SDL_Rect dst;

  on = AVT_MAKE_BOOL (on);

  if (on != text_cursor_actually_visible && !hold_updates)
    {
      dst.x = cursor.x;
      dst.y = cursor.y;
      dst.w = FONTWIDTH;
      dst.h = FONTHEIGHT;

      if (on)
	{
	  /* save character under cursor */
	  SDL_BlitSurface (screen, &dst, avt_cursor_character, NULL);

	  /* show text-cursor */
	  SDL_BlitSurface (avt_text_cursor, NULL, screen, &dst);
	  AVT_UPDATE_TRECT (dst);
	}
      else
	{
	  /* restore saved character */
	  SDL_BlitSurface (avt_cursor_character, NULL, screen, &dst);
	  AVT_UPDATE_TRECT (dst);
	}

      text_cursor_actually_visible = on;
    }
}

extern void
avt_activate_cursor (bool on)
{
  text_cursor_visible = AVT_MAKE_BOOL (on);

  if (screen && textfield.x >= 0)
    avt_show_text_cursor (text_cursor_visible);
}

/* fills the screen with the background color,
 * but doesn't update the screen yet
 */
static void
avt_free_screen (void)
{
  /* switch clipping off */
  SDL_SetClipRect (screen, NULL);
  /* fill the whole screen with background color */
  SDL_FillRect (screen, NULL, background_color);
}

extern void
avt_clear_screen (void)
{
  if (screen)
    {
      avt_free_screen ();
      AVT_UPDATE_ALL ();
    }

  /* undefine textfield / viewport */
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;
  avt_visible = false;
}

#define NAME_PADDING 3

static void
avt_show_name (void)
{
  SDL_Rect dst;
  SDL_Color old_colors[2], colors[2];
  wchar_t *p;

  if (screen && avatar_image && avt_name)
    {
      /* save old character colors */
      old_colors[0] = avt_character->format->palette->colors[0];
      old_colors[1] = avt_character->format->palette->colors[1];

      /* tan background */
      colors[0].r = 210;
      colors[0].g = 180;
      colors[0].b = 140;

      /* black foreground */
      colors[1].r = colors[1].g = colors[1].b = 0;

      SDL_SetColors (avt_character, colors, 0, 2);

      dst.x = window.x + AVATAR_MARGIN + avatar_image->w + BUTTON_DISTANCE;
      dst.y = window.y + window.h - AVATAR_MARGIN - FONTHEIGHT
	- 2 * NAME_PADDING;
      dst.w = (avt_strwidth (avt_name) * FONTWIDTH) + 2 * NAME_PADDING;
      dst.h = FONTHEIGHT + 2 * NAME_PADDING;

      /* draw sign */
      SDL_FillRect (screen, &dst,
		    SDL_MapRGB (screen->format,
				colors[0].r, colors[0].g, colors[0].b));

      /* show name */
      cursor.x = dst.x + NAME_PADDING;
      cursor.y = dst.y + NAME_PADDING;

      p = avt_name;
      while (*p)
	{
	  avt_drawchar ((avt_char) * p++, screen);
	  cursor.x += FONTWIDTH;
	}

      /* restore old character colors */
      SDL_SetColors (avt_character, old_colors, 0, 2);
    }
}

/* draw the avatar image,
 * but doesn't update the screen yet
 */
static void
avt_draw_avatar (void)
{
  SDL_Rect dst;

  if (screen)
    {
      /* fill the screen with background color */
      /* (not only the window!) */
      avt_free_screen ();

      SDL_SetClipRect (screen, &window);

      if (avatar_image)
	{
	  /* left */
	  dst.x = window.x + AVATAR_MARGIN;
	  /* bottom */
	  dst.y = window.y + window.h - avatar_image->h - AVATAR_MARGIN;
	  dst.w = avatar_image->w;
	  dst.h = avatar_image->h;
	  SDL_BlitSurface (avatar_image, NULL, screen, &dst);
	}

      if (avt_name)
	avt_show_name ();
    }
}

extern void
avt_show_avatar (void)
{
  if (screen)
    {
      avt_draw_avatar ();
      AVT_UPDATE_ALL ();

      /* undefine textfield */
      textfield.x = textfield.y = textfield.w = textfield.h = -1;
      viewport = textfield;
      avt_visible = true;
    }
}

static void
avt_draw_balloon2 (int offset, Uint32 ballooncolor)
{
  SDL_Rect shape;

  /* full size */
  shape.x = textfield.x - BALLOON_INNER_MARGIN + offset;
  shape.w = textfield.w + (2 * BALLOON_INNER_MARGIN);
  shape.y = textfield.y - BALLOON_INNER_MARGIN + offset;
  shape.h = textfield.h + (2 * BALLOON_INNER_MARGIN);

  /* horizontal shape */
  {
    SDL_Rect hshape;
    hshape.x = shape.x;
    hshape.w = shape.w;
    hshape.y = shape.y + (circle_height / 2);
    hshape.h = shape.h - circle_height;
    SDL_FillRect (screen, &hshape, ballooncolor);
  }

  /* vertical shape */
  {
    SDL_Rect vshape;
    vshape.x = shape.x + (circle_width / 2);
    vshape.w = shape.w - circle_width;
    vshape.y = shape.y;
    vshape.h = shape.h;
    SDL_FillRect (screen, &vshape, ballooncolor);
  }

  /* draw corners */
  {
    SDL_Rect circle_piece, corner_pos;

    /* prepare circle piece */
    /* the size is always the same */
    circle_piece.w = circle_width / 2;
    circle_piece.h = circle_height / 2;

    /* upper left corner */
    circle_piece.x = 0;
    circle_piece.y = 0;
    corner_pos.x = shape.x;
    corner_pos.y = shape.y;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    /* upper right corner */
    circle_piece.x = ((circle_width + 7) / 8) / 2;
    circle_piece.y = 0;
    corner_pos.x = shape.x + shape.w - circle_piece.w;
    corner_pos.y = shape.y;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    /* lower left corner */
    circle_piece.x = 0;
    circle_piece.y = circle_height / 2;
    corner_pos.x = shape.x;
    corner_pos.y = shape.y + shape.h - circle_piece.h;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    /* lower right corner */
    circle_piece.x = ((circle_width + 7) / 8) / 2;
    circle_piece.y = circle_height / 2;
    corner_pos.x = shape.x + shape.w - circle_piece.w;
    corner_pos.y = shape.y + shape.h - circle_piece.h;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);
  }

  /* draw balloonpointer */
  /* only if there is an avatar image */
  if (avatar_image)
    {
      SDL_Rect pointer_shape, pointer_pos;

      pointer_shape.x = pointer_shape.y = 0;
      pointer_shape.w = pointer->w;
      pointer_shape.h = pointer->h;

      /* if the balloonpointer is too large, cut it */
      if (pointer_shape.h > (avatar_image->h / 2))
	{
	  pointer_shape.y = pointer_shape.h - (avatar_image->h / 2);
	  pointer_shape.h -= pointer_shape.y;
	}

      pointer_pos.x =
	window.x + avatar_image->w + (2 * AVATAR_MARGIN) +
	BALLOONPOINTER_OFFSET + offset;
      pointer_pos.y =
	window.y + (balloonmaxheight * LINEHEIGHT) +
	(2 * BALLOON_INNER_MARGIN) + TOPMARGIN + offset;

      /* only draw the balloonpointer, when it fits */
      if (pointer_pos.x + pointer->w + BALLOONPOINTER_OFFSET
	  + BALLOON_INNER_MARGIN < window.x + window.w)
	SDL_BlitSurface (pointer, &pointer_shape, screen, &pointer_pos);
    }
}

static void
avt_draw_balloon (void)
{
  if (!avt_visible)
    avt_draw_avatar ();

  SDL_SetClipRect (screen, &window);

  textfield.w = (balloonwidth * FONTWIDTH);
  textfield.h = (balloonheight * LINEHEIGHT);

  if (avatar_image)
    textfield.y = window.y + ((balloonmaxheight - balloonheight) * LINEHEIGHT)
      + TOPMARGIN + BALLOON_INNER_MARGIN;
  else				/* middle of the window */
    textfield.y = window.y + (window.h / 2) - (textfield.h / 2);

  /* centered as default */
  textfield.x = window.x + (window.w / 2) - (balloonwidth * FONTWIDTH / 2);

  /* align with balloonpointer */
  if (avatar_image)
    {
      /* left border not aligned with balloon pointer? */
      if (textfield.x >
	  window.x + avatar_image->w + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET)
	textfield.x =
	  window.x + avatar_image->w + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET;

      /* right border not aligned with balloon pointer? */
      if (textfield.x + textfield.w <
	  window.x + avatar_image->w + pointer->w
	  + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET)
	{
	  textfield.x =
	    window.x + avatar_image->w - textfield.w + pointer->w
	    + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET;

	  /* align with right window-border */
	  if (textfield.x > window.x + window.w - (balloonwidth * FONTWIDTH)
	      - (2 * BALLOON_INNER_MARGIN))
	    textfield.x = window.x + window.w - (balloonwidth * FONTWIDTH)
	      - (2 * BALLOON_INNER_MARGIN);
	}
    }

  viewport = textfield;

  /* shadow color is a little darker than the background color */
  {
    SDL_Color shadow_color;

    shadow_color.r =
      (backgroundcolor_RGB.r > 0x20) ? backgroundcolor_RGB.r - 0x20 : 0;
    shadow_color.g =
      (backgroundcolor_RGB.g > 0x20) ? backgroundcolor_RGB.g - 0x20 : 0;
    shadow_color.b =
      (backgroundcolor_RGB.b > 0x20) ? backgroundcolor_RGB.b - 0x20 : 0;

    SDL_SetColors (circle, &shadow_color, 1, 1);
    SDL_SetColors (pointer, &shadow_color, 1, 1);

    /* first draw shadow */
    avt_draw_balloon2 (SHADOWOFFSET,
		       SDL_MapRGB (screen->format, shadow_color.r,
				   shadow_color.g, shadow_color.b));
  }


  /* real balloon */
  {
    SDL_SetColors (circle, &ballooncolor_RGB, 1, 1);
    SDL_SetColors (pointer, &ballooncolor_RGB, 1, 1);

    avt_draw_balloon2 (0, SDL_MapRGB (screen->format,
				      ballooncolor_RGB.r,
				      ballooncolor_RGB.g,
				      ballooncolor_RGB.b));
  }

  linestart =
    (textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;

  avt_visible = true;

  /* cursor at top  */
  cursor.x = linestart;
  cursor.y = viewport.y;

  /* reset saved position */
  saved_position = cursor;

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  /* update everything */
  /* (there may be leftovers from large images) */
  AVT_UPDATE_ALL ();

  /*
   * only allow drawings inside this area from now on
   * (only for blitting)
   */
  SDL_SetClipRect (screen, &viewport);
}

extern void
avt_text_direction (int direction)
{
  SDL_Rect area;

  textdir_rtl = direction;

  /*
   * if there is already a ballon,
   * recalculate the linestart and put the cursor in the first position
   */
  if (screen && textfield.x >= 0)
    {
      if (text_cursor_visible)
	avt_show_text_cursor (false);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      linestart = (textdir_rtl) ? area.x + area.w - FONTWIDTH : area.x;
      cursor.x = linestart;

      if (text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_set_balloon_width (int width)
{
  if (width != balloonwidth)
    {
      if (width < AVT_LINELENGTH && width > 0)
	balloonwidth = (width > 7) ? width : 7;
      else
	balloonwidth = AVT_LINELENGTH;

      /* if balloon is visible, redraw it */
      if (textfield.x >= 0)
	{
	  avt_visible = false;	/* force to redraw everything */
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_balloon_height (int height)
{
  if (height != balloonheight)
    {
      if (height > 0 && height < balloonmaxheight)
	balloonheight = height;
      else
	balloonheight = balloonmaxheight;

      /* if balloon is visible, redraw it */
      if (textfield.x >= 0)
	{
	  avt_visible = false;	/* force to redraw everything */
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_balloon_size (int height, int width)
{
  if (height != balloonheight || width != balloonwidth)
    {
      if (height > 0 && height < balloonmaxheight)
	balloonheight = height;
      else
	balloonheight = balloonmaxheight;

      if (width < AVT_LINELENGTH && width > 0)
	balloonwidth = (width > 7) ? width : 7;
      else
	balloonwidth = AVT_LINELENGTH;

      /* if balloon is visible, redraw it */
      if (textfield.x >= 0)
	{
	  avt_visible = false;	/* force to redraw everything */
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_bell (void)
{
  bell ();
}

/* flashes the screen */
extern void
avt_flash (void)
{
  SDL_Surface *oldwindowimage;

  if (!screen)
    return;

  oldwindowimage = SDL_CreateRGBSurface (SDL_SWSURFACE, window.w, window.h,
					 screen->format->BitsPerPixel,
					 screen->format->Rmask,
					 screen->format->Gmask,
					 screen->format->Bmask,
					 screen->format->Amask);

  SDL_BlitSurface (screen, &window, oldwindowimage, NULL);

  /* switch clipping off */
  SDL_SetClipRect (screen, NULL);
  /* fill the whole screen with color */
  SDL_FillRect (screen, NULL, SDL_MapRGB (screen->format, 0xFF, 0xFF, 0x00));
  AVT_UPDATE_ALL ();
  SDL_Delay (150);

  /* fill the whole screen with background color */
  SDL_FillRect (screen, NULL, background_color);
  /* restore image */
  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (oldwindowimage, NULL, screen, &window);
  SDL_FreeSurface (oldwindowimage);

  /* make visible again */
  AVT_UPDATE_ALL ();

  /* restore the clipping */
  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);
}

static void
avt_change_mode (void)
{
  SDL_Surface *oldwindowimage;

  /* save the window */
  oldwindowimage = SDL_CreateRGBSurface (SDL_SWSURFACE, window.w, window.h,
					 screen->format->BitsPerPixel,
					 screen->format->Rmask,
					 screen->format->Gmask,
					 screen->format->Bmask,
					 screen->format->Amask);

  SDL_BlitSurface (screen, &window, oldwindowimage, NULL);

  /* set new mode */
  screen = SDL_SetVideoMode (windowmode_size.w, windowmode_size.h,
			     COLORDEPTH, screenflags);

  background_color = SDL_MapRGB (screen->format,
				 backgroundcolor_RGB.r,
				 backgroundcolor_RGB.g,
				 backgroundcolor_RGB.b);
  avt_free_screen ();

  /* new position of the window on the screen */
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  /* restore image */
  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (oldwindowimage, NULL, screen, &window);
  SDL_FreeSurface (oldwindowimage);

  /* make all changes visible */
  AVT_UPDATE_ALL ();
}

extern void
avt_toggle_fullscreen (void)
{
  if (avt_mode != AVT_FULLSCREENNOSWITCH)
    {
      /* toggle bit for fullscreenmode */
      screenflags ^= SDL_FULLSCREEN;

      if ((screenflags & SDL_FULLSCREEN) != 0)
	avt_mode = AVT_FULLSCREEN;
      else
	avt_mode = AVT_WINDOW;

      avt_change_mode ();
    }
}

/* switch to fullscreen or window mode */
extern void
avt_switch_mode (int mode)
{
  if (screen && mode != avt_mode)
    {
      avt_mode = mode;
      switch (mode)
	{
	case AVT_FULLSCREENNOSWITCH:
	case AVT_FULLSCREEN:
	  if ((screenflags & SDL_FULLSCREEN) == 0)
	    {
	      screenflags |= SDL_FULLSCREEN;
	      avt_change_mode ();
	    }
	  break;
	case AVT_WINDOW:
	  if ((screenflags & SDL_FULLSCREEN) != 0)
	    {
	      screenflags &= ~SDL_FULLSCREEN;
	      avt_change_mode ();
	    }
	  break;
	}
    }
}

extern int
avt_get_mode (void)
{
  return avt_mode;
}

/* external: not in the API, but used in avatar-audio */
void AVT_HIDDEN
avt_analyze_event (SDL_Event * event)
{
  switch (event->type)
    {
    case SDL_QUIT:
      _avt_STATUS = AVT_QUIT;
      break;

    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      if (avt_ext_mousehandler)
	{
	  int x, y;
	  x = (event->button.x - textfield.x) / FONTWIDTH + 1;
	  y = (event->button.y - textfield.y) / LINEHEIGHT + 1;

	  /* if x or y is invalid set both to -1 */
	  if (x < 1 || x > AVT_LINELENGTH
	      || y < 1 || y > (textfield.h / LINEHEIGHT))
	    x = y = -1;

	  avt_ext_mousehandler (event->button.button,
				(event->button.state == SDL_PRESSED), x, y);
	}
      break;

    case SDL_KEYDOWN:
      switch (event->key.keysym.sym)
	{
	case SDLK_PAUSE:
	  avt_pause ();
	  break;

	  /* no "break" default for the following ones: */
	  /* they may fall through to default: */

	case SDLK_F11:
	  if (event->key.keysym.sym == SDLK_F11 && !reserve_single_keys)
	    {
	      avt_toggle_fullscreen ();
	      break;
	    }

	case SDLK_ESCAPE:
	  if (event->key.keysym.sym == SDLK_ESCAPE && !reserve_single_keys)
	    {
	      _avt_STATUS = AVT_QUIT;
	      break;
	    }

	  /* Alt + Q -> Quit */
	case SDLK_q:
	  if (event->key.keysym.sym == SDLK_q
	      && (event->key.keysym.mod & KMOD_LALT))
	    {
	      _avt_STATUS = AVT_QUIT;
	      break;
	    }

	  /* Left Alt + Return -> avt_toggle_fullscreen */
	case SDLK_RETURN:
	  if (event->key.keysym.sym == SDLK_RETURN
	      && event->key.keysym.mod & KMOD_LALT)
	    {
	      avt_toggle_fullscreen ();
	      break;
	    }

	  /* Ctrl + Left Alt + F -> avt_toggle_fullscreen */
	case SDLK_f:
	  if (event->key.keysym.sym == SDLK_f
	      && (event->key.keysym.mod & KMOD_CTRL)
	      && (event->key.keysym.mod & KMOD_LALT))
	    {
	      avt_toggle_fullscreen ();
	      break;
	    }

	default:
	  if (avt_ext_keyhandler)
	    avt_ext_keyhandler (event->key.keysym.sym, event->key.keysym.mod,
				event->key.keysym.unicode);
	  break;
	}			/* switch (*event.key.keysym.sym) */
    }				/* switch (*event.type) */
}

static int
avt_pause (void)
{
  SDL_Event event;
  bool pause;
  bool audio_initialized;

  audio_initialized = (SDL_WasInit (SDL_INIT_AUDIO) != 0);
  pause = true;

  if (audio_initialized)
    SDL_PauseAudio (pause);

  do
    {
      if (SDL_WaitEvent (&event))
	{
	  if (event.type == SDL_KEYDOWN)
	    pause = false;
	  avt_analyze_event (&event);
	}
    }
  while (pause && !_avt_STATUS);

  if (audio_initialized && !_avt_STATUS)
    SDL_PauseAudio (pause);

  return _avt_STATUS;
}

/* external: not in the API, but used in avatar-audio */
int AVT_HIDDEN
avt_checkevent (void)
{
  SDL_Event event;

  while (SDL_PollEvent (&event))
    avt_analyze_event (&event);

  return _avt_STATUS;
}

/* checks for events and gives some time to other apps */
extern int
avt_update (void)
{
  if (screen)
    {
      SDL_Delay (1);
      avt_checkevent ();
    }

  return _avt_STATUS;
}

/* send a timeout event */
static Uint32
avt_timeout (Uint32 intervall, void *param)
{
  SDL_Event event;

  event.type = SDL_USEREVENT;
  event.user.code = AVT_TIMEOUT;
  event.user.data1 = event.user.data2 = NULL;
  SDL_PushEvent (&event);

  return 0;
}

extern int
avt_wait (size_t milliseconds)
{
  if (screen && _avt_STATUS == AVT_NORMAL)
    {
      if (milliseconds <= 500)	/* short delay */
	{
	  SDL_Delay (milliseconds);
	  avt_checkevent ();
	}
      else			/* longer */
	{
	  SDL_Event event;
	  SDL_TimerID t;

	  t = SDL_AddTimer (milliseconds, avt_timeout, NULL);

	  if (t == NULL)
	    {
	      /* extremely unlikely error */
	      SDL_SetError ("AddTimer doesn't work");
	      _avt_STATUS = AVT_ERROR;
	      return _avt_STATUS;
	    }

	  while (_avt_STATUS == AVT_NORMAL)
	    {
	      SDL_WaitEvent (&event);
	      if (event.type == SDL_USEREVENT
		  && event.user.code == AVT_TIMEOUT)
		break;
	      else
		avt_analyze_event (&event);
	    }

	  SDL_RemoveTimer (t);
	}
    }

  return _avt_STATUS;
}

extern size_t
avt_ticks (void)
{
  return (size_t) SDL_GetTicks ();
}

extern int
avt_where_x (void)
{
  if (screen && textfield.x >= 0)
    {
      if (origin_mode)
	return ((cursor.x - viewport.x) / FONTWIDTH) + 1;
      else
	return ((cursor.x - textfield.x) / FONTWIDTH) + 1;
    }
  else
    return -1;
}

extern int
avt_where_y (void)
{
  if (screen && textfield.x >= 0)
    {
      if (origin_mode)
	return ((cursor.y - viewport.y) / LINEHEIGHT) + 1;
      else
	return ((cursor.y - textfield.y) / LINEHEIGHT) + 1;
    }
  else
    return -1;
}

extern bool
avt_home_position (void)
{
  if (!screen || textfield.x < 0)
    return true;		/* about to be set to home position */
  else
    return (cursor.y == viewport.y && cursor.x == linestart);
}

/* this always means the full textfield */
extern int
avt_get_max_x (void)
{
  if (screen)
    return balloonwidth;
  else
    return -1;
}

/* this always means the full textfield */
extern int
avt_get_max_y (void)
{
  if (screen)
    return balloonheight;
  else
    return -1;
}

extern void
avt_move_x (int x)
{
  SDL_Rect area;

  if (screen && textfield.x >= 0)
    {
      if (x < 1)
	x = 1;

      if (text_cursor_visible)
	avt_show_text_cursor (false);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      cursor.x = (x - 1) * FONTWIDTH + area.x;

      /* max-pos exeeded? */
      if (cursor.x > area.x + area.w - FONTWIDTH)
	cursor.x = area.x + area.w - FONTWIDTH;

      if (text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_move_y (int y)
{
  SDL_Rect area;

  if (screen && textfield.x >= 0)
    {
      if (y < 1)
	y = 1;

      if (text_cursor_visible)
	avt_show_text_cursor (false);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      cursor.y = (y - 1) * LINEHEIGHT + area.y;

      /* max-pos exeeded? */
      if (cursor.y > area.y + area.h - LINEHEIGHT)
	cursor.y = area.y + area.h - LINEHEIGHT;

      if (text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_move_xy (int x, int y)
{
  SDL_Rect area;

  if (screen && textfield.x >= 0)
    {
      if (x < 1)
	x = 1;

      if (y < 1)
	y = 1;

      if (text_cursor_visible)
	avt_show_text_cursor (false);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      cursor.x = (x - 1) * FONTWIDTH + area.x;
      cursor.y = (y - 1) * LINEHEIGHT + area.y;

      /* max-pos exeeded? */
      if (cursor.x > area.x + area.w - FONTWIDTH)
	cursor.x = area.x + area.w - FONTWIDTH;

      if (cursor.y > area.y + area.h - LINEHEIGHT)
	cursor.y = area.y + area.h - LINEHEIGHT;

      if (text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_save_position (void)
{
  saved_position = cursor;
}

extern void
avt_restore_position (void)
{
  cursor = saved_position;
}

extern void
avt_insert_spaces (int num)
{
  SDL_Rect rest, dest, clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  /* get the rest of the viewport */
  rest.x = cursor.x;
  rest.w = viewport.w - (cursor.x - viewport.x) - (num * FONTWIDTH);
  rest.y = cursor.y;
  rest.h = LINEHEIGHT;

  dest.x = cursor.x + (num * FONTWIDTH);
  dest.y = cursor.y;
  SDL_BlitSurface (screen, &rest, screen, &dest);

  clear.x = cursor.x;
  clear.y = cursor.y;
  clear.w = num * FONTWIDTH;
  clear.h = LINEHEIGHT;
  SDL_FillRect (screen, &clear, text_background_color);

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  /* update line */
  if (!hold_updates)
    SDL_UpdateRect (screen, viewport.x, cursor.y, viewport.w, FONTHEIGHT);
}

extern void
avt_delete_characters (int num)
{
  SDL_Rect rest, dest, clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  /* get the rest of the viewport */
  rest.x = cursor.x + (num * FONTWIDTH);
  rest.w = viewport.w - (cursor.x - viewport.x) - (num * FONTWIDTH);
  rest.y = cursor.y;
  rest.h = LINEHEIGHT;

  dest.x = cursor.x;
  dest.y = cursor.y;
  SDL_BlitSurface (screen, &rest, screen, &dest);

  clear.x = viewport.x + viewport.w - (num * FONTWIDTH);
  clear.y = cursor.y;
  clear.w = num * FONTWIDTH;
  clear.h = LINEHEIGHT;
  SDL_FillRect (screen, &clear, text_background_color);

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  /* update line */
  if (!hold_updates)
    SDL_UpdateRect (screen, viewport.x, cursor.y, viewport.w, FONTHEIGHT);
}

extern void
avt_erase_characters (int num)
{
  SDL_Rect clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  clear.x = (textdir_rtl) ? cursor.x - (num * FONTWIDTH) : cursor.x;
  clear.y = cursor.y;
  clear.w = num * FONTWIDTH;
  clear.h = LINEHEIGHT;
  SDL_FillRect (screen, &clear, text_background_color);

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  /* update area */
  AVT_UPDATE_TRECT (clear);
}

extern void
avt_delete_lines (int line, int num)
{
  SDL_Rect rest, dest, clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  if (!origin_mode)
    line -= (viewport.y - textfield.y) / LINEHEIGHT;

  /* check if values are sane */
  if (line < 1 || num < 1 || line > (viewport.h / LINEHEIGHT))
    return;

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  /* get the rest of the viewport */
  rest.x = viewport.x;
  rest.w = viewport.w;
  rest.y = viewport.y + ((line - 1 + num) * LINEHEIGHT);
  rest.h = viewport.h - ((line - 1 + num) * LINEHEIGHT);

  dest.x = viewport.x;
  dest.y = viewport.y + ((line - 1) * LINEHEIGHT);
  SDL_BlitSurface (screen, &rest, screen, &dest);

  clear.w = viewport.w;
  clear.h = num * LINEHEIGHT;
  clear.x = viewport.x;
  clear.y = viewport.y + viewport.h - (num * LINEHEIGHT);
  SDL_FillRect (screen, &clear, text_background_color);

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  AVT_UPDATE_TRECT (viewport);
}

extern void
avt_insert_lines (int line, int num)
{
  SDL_Rect rest, dest, clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  if (!origin_mode)
    line -= (viewport.y - textfield.y) / LINEHEIGHT;

  /* check if values are sane */
  if (line < 1 || num < 1 || line > (viewport.h / LINEHEIGHT))
    return;

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  /* get the rest of the viewport */
  rest.x = viewport.x;
  rest.w = viewport.w;
  rest.y = viewport.y + ((line - 1) * LINEHEIGHT);
  rest.h = viewport.h - ((line - 1 + num) * LINEHEIGHT);

  dest.x = viewport.x;
  dest.y = viewport.y + ((line - 1 + num) * LINEHEIGHT);
  SDL_BlitSurface (screen, &rest, screen, &dest);

  clear.x = viewport.x;
  clear.y = viewport.y + ((line - 1) * LINEHEIGHT);
  clear.w = viewport.w;
  clear.h = num * LINEHEIGHT;
  SDL_FillRect (screen, &clear, text_background_color);

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  AVT_UPDATE_TRECT (viewport);
}

extern void
avt_viewport (int x, int y, int width, int height)
{
  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  viewport.x = textfield.x + ((x - 1) * FONTWIDTH);
  viewport.y = textfield.y + ((y - 1) * LINEHEIGHT);
  viewport.w = width * FONTWIDTH;
  viewport.h = height * LINEHEIGHT;

  linestart =
    (textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;

  cursor.x = linestart;
  cursor.y = viewport.y;

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  if (origin_mode)
    SDL_SetClipRect (screen, &viewport);
  else
    SDL_SetClipRect (screen, &textfield);
}

extern void
avt_newline_mode (bool mode)
{
  newline_mode = mode;
}


extern bool
avt_get_newline_mode (void)
{
  return newline_mode;
}

extern void
avt_set_auto_margin (bool mode)
{
  auto_margin = mode;
}

extern bool
avt_get_auto_margin (void)
{
  return auto_margin;
}

extern void
avt_set_origin_mode (bool mode)
{
  SDL_Rect area;

  origin_mode = AVT_MAKE_BOOL (mode);

  if (text_cursor_visible && textfield.x >= 0)
    avt_show_text_cursor (false);

  if (origin_mode)
    area = viewport;
  else
    area = textfield;

  linestart = (textdir_rtl) ? area.x + area.w - FONTWIDTH : area.x;

  /* cursor to position 1,1 */
  /* when origin mode is off, then it may be outside the viewport (sic) */
  cursor.x = linestart;
  cursor.y = area.y;

  /* reset saved position */
  saved_position = cursor;

  if (text_cursor_visible && textfield.x >= 0)
    avt_show_text_cursor (true);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &area);
}

extern bool
avt_get_origin_mode (void)
{
  return origin_mode;
}

extern void
avt_clear (void)
{
  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  /* use background color of characters */
  SDL_FillRect (screen, &viewport, text_background_color);

  cursor.x = linestart;

  if (origin_mode)
    cursor.y = viewport.y;
  else
    cursor.y = textfield.y;

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  AVT_UPDATE_TRECT (viewport);
}

extern void
avt_clear_up (void)
{
  SDL_Rect dst;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  dst.x = viewport.x;
  dst.w = viewport.w;
  dst.y = viewport.y + FONTHEIGHT;
  dst.h = cursor.y;

  SDL_FillRect (screen, &dst, text_background_color);

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  AVT_UPDATE_TRECT (dst);
}

extern void
avt_clear_down (void)
{
  SDL_Rect dst;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  dst.x = viewport.x;
  dst.w = viewport.w;
  dst.y = cursor.y;
  dst.h = viewport.h - (cursor.y - viewport.y);

  SDL_FillRect (screen, &dst, text_background_color);

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  AVT_UPDATE_TRECT (dst);
}

extern void
avt_clear_eol (void)
{
  SDL_Rect dst;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  if (textdir_rtl)		/* right to left */
    {
      dst.x = viewport.x;
      dst.y = cursor.y;
      dst.h = FONTHEIGHT;
      dst.w = cursor.x + FONTWIDTH - viewport.x;
    }
  else				/* left to right */
    {
      dst.x = cursor.x;
      dst.y = cursor.y;
      dst.h = FONTHEIGHT;
      dst.w = viewport.w - (cursor.x - viewport.x);
    }

  SDL_FillRect (screen, &dst, text_background_color);

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  AVT_UPDATE_TRECT (dst);
}

/* clear beginning of line */
extern void
avt_clear_bol (void)
{
  SDL_Rect dst;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  if (textdir_rtl)		/* right to left */
    {
      dst.x = cursor.x;
      dst.y = cursor.y;
      dst.h = FONTHEIGHT;
      dst.w = viewport.w - (cursor.x - viewport.x);
    }
  else				/* left to right */
    {
      dst.x = viewport.x;
      dst.y = cursor.y;
      dst.h = FONTHEIGHT;
      dst.w = cursor.x + FONTWIDTH - viewport.x;
    }

  SDL_FillRect (screen, &dst, text_background_color);

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  AVT_UPDATE_TRECT (dst);
}

extern void
avt_clear_line (void)
{
  SDL_Rect dst;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  dst.x = viewport.x;
  dst.y = cursor.y;
  dst.h = FONTHEIGHT;
  dst.w = viewport.w;

  SDL_FillRect (screen, &dst, text_background_color);

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  AVT_UPDATE_TRECT (dst);
}

extern int
avt_flip_page (void)
{
  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return _avt_STATUS;

  /* do nothing when the textfield is already empty */
  if (cursor.x == linestart && cursor.y == viewport.y)
    return _avt_STATUS;

  /* the viewport must be updated,
     if it's not updated letter by letter */
  if (!text_delay)
    AVT_UPDATE_TRECT (viewport);

  avt_wait (flip_page_delay);
  avt_clear ();
  return _avt_STATUS;
}

static void
avt_scroll_up (void)
{
  switch (scroll_mode)
    {
    case -1:
      /* move cursor outside of balloon */
      cursor.y += LINEHEIGHT;
      break;
    case 1:
      avt_delete_lines (((viewport.y - textfield.y) / LINEHEIGHT) + 1, 1);

      if (origin_mode)
	cursor.y = viewport.y + viewport.h - LINEHEIGHT;
      else
	cursor.y = textfield.y + textfield.h - LINEHEIGHT;

      if (newline_mode)
	cursor.x = linestart;
      break;
    case 0:
      avt_flip_page ();
      break;
    }
}

static void
avt_carriage_return (void)
{
  if (text_cursor_visible)
    avt_show_text_cursor (false);

  cursor.x = linestart;

  if (text_cursor_visible)
    avt_show_text_cursor (true);
}

extern int
avt_new_line (void)
{
  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return _avt_STATUS;

  if (text_cursor_visible)
    avt_show_text_cursor (false);

  if (newline_mode)
    cursor.x = linestart;

  /* if the cursor is at the last line of the viewport
   * scroll up
   */
  if (cursor.y == viewport.y + viewport.h - LINEHEIGHT)
    avt_scroll_up ();
  else
    cursor.y += LINEHEIGHT;

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  return _avt_STATUS;
}

/* avt_drawchar: draws the raw char - with no interpretation */
#if (FONTWIDTH > 8)
static void
avt_drawchar (avt_char ch, SDL_Surface * surface)
{
  const unsigned short *font_line;
  unsigned int y;
  SDL_Rect dest;
  unsigned short *pixels, *p;
  Uint16 pitch;

  pitch = avt_character->pitch / sizeof (*p);
  pixels = p = (unsigned short *) avt_character->pixels;
  font_line = (const unsigned short *) get_font_char ((int) ch);
  if (!font_line)
    font_line = (const unsigned short *) get_font_char (0);

  for (y = 0; y < FONTHEIGHT; y++)
    {
      /* TODO: needs test on big endian machines */
      *p = SDL_SwapBE16 (*font_line);
      if (bold && !NOT_BOLD)
	*p |= SDL_SwapBE16 (*font_line >> 1);
      if (inverse)
	*p = ~*p;
      font_line++;
      p += pitch;
    }

  if (underlined)
    pixels[UNDERLINE * pitch] = (inverse) ? 0x0000 : 0xFFFF;

  dest.x = cursor.x;
  dest.y = cursor.y;
  SDL_BlitSurface (avt_character, NULL, surface, &dest);
}

#else /* FONTWIDTH <= 8 */

static void
avt_drawchar (avt_char ch, SDL_Surface * surface)
{
  const unsigned char *font_line;
  int y;
  SDL_Rect dest;
  Uint8 *pixels, *p;
  Uint16 pitch;

  pitch = avt_character->pitch;
  pixels = p = (Uint8 *) avt_character->pixels;
  font_line = (const unsigned char *) get_font_char ((int) ch);
  if (!font_line)
    font_line = (const unsigned char *) get_font_char (0);

  for (y = 0; y < FONTHEIGHT; y++)
    {
      *p = *font_line;
      if (bold && !NOT_BOLD)
	*p |= (*font_line >> 1);
      if (inverse)
	*p = ~*p;
      font_line++;
      p += pitch;
    }

  if (underlined)
    pixels[UNDERLINE * pitch] = (inverse) ? 0x00 : 0xFF;

  dest.x = cursor.x;
  dest.y = cursor.y;
  SDL_BlitSurface (avt_character, NULL, surface, &dest);
}

#endif /* FONTWIDTH <= 8 */

extern void
avt_get_font_size (int *width, int *height)
{
  *width = FONTWIDTH;
  *height = FONTHEIGHT;
}

extern bool
avt_is_printable (avt_char ch)
{
  return (bool) (get_font_char ((int) ch) != NULL);
}

/* make current char visible */
static void
avt_showchar (void)
{
  if (!hold_updates)
    {
      SDL_UpdateRect (screen, cursor.x, cursor.y, FONTWIDTH, FONTHEIGHT);
      text_cursor_actually_visible = false;
    }
}

/* advance position - only in the textfield */
extern int
avt_forward (void)
{
  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return _avt_STATUS;

  cursor.x = (textdir_rtl) ? cursor.x - FONTWIDTH : cursor.x + FONTWIDTH;

  if (text_cursor_visible)
    avt_show_text_cursor (true);

  return _avt_STATUS;
}

/*
 * if cursor is horizontally outside of the viewport
 *  and the balloon is visible
 * and auto_margin is activated
 * then start a new line
 */
static void
check_auto_margin (void)
{
  if (screen && textfield.x >= 0 && auto_margin)
    {
      if (cursor.x < viewport.x
	  || cursor.x > viewport.x + viewport.w - FONTWIDTH)
	{
	  if (!newline_mode)
	    avt_carriage_return ();
	  avt_new_line ();
	}

      if (text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_reset_tab_stops (void)
{
  int i;

  for (i = 0; i < AVT_LINELENGTH; i++)
    if (i % 8 == 0)
      avt_tab_stops[i] = true;
    else
      avt_tab_stops[i] = false;
}

extern void
avt_clear_tab_stops (void)
{
  SDL_memset (&avt_tab_stops, false, sizeof (avt_tab_stops));
}

extern void
avt_set_tab (int x, bool onoff)
{
  avt_tab_stops[x - 1] = AVT_MAKE_BOOL (onoff);
}

/* advance to next tabstop */
extern void
avt_next_tab (void)
{
  int x;
  int i;

  /* here we count zero based */
  x = avt_where_x () - 1;

  if (textdir_rtl)		/* right to left */
    {
      for (i = x; i >= 0; i--)
	{
	  if (avt_tab_stops[i])
	    break;
	}
      avt_move_x (i);
    }
  else				/* left to right */
    {
      for (i = x + 1; i < AVT_LINELENGTH; i++)
	{
	  if (avt_tab_stops[i])
	    break;
	}
      avt_move_x (i + 1);
    }
}

/* go to last tabstop */
extern void
avt_last_tab (void)
{
  int x;
  int i;

  /* here we count zero based */
  x = avt_where_x () - 1;

  if (textdir_rtl)		/* right to left */
    {
      for (i = x; i < AVT_LINELENGTH; i++)
	{
	  if (avt_tab_stops[i])
	    break;
	}
      avt_move_x (i);
    }
  else				/* left to right */
    {
      for (i = x + 1; i >= 0; i--)
	{
	  if (avt_tab_stops[i])
	    break;
	}
      avt_move_x (i + 1);
    }
}

static void
avt_clearchar (void)
{
  SDL_Rect dst;

  dst.x = cursor.x;
  dst.y = cursor.y;
  dst.w = FONTWIDTH;
  dst.h = FONTHEIGHT;

  SDL_FillRect (screen, &dst, text_background_color);
  avt_showchar ();
}

extern void
avt_backspace (void)
{
  if (screen && textfield.x >= 0)
    {
      if (cursor.x != linestart)
	{
	  if (text_cursor_visible)
	    avt_show_text_cursor (false);

	  cursor.x =
	    (textdir_rtl) ? cursor.x + FONTWIDTH : cursor.x - FONTWIDTH;
	}

      if (text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

/*
 * writes a character to the textfield -
 * interprets control characters
 */
extern int
avt_put_char (avt_char ch)
{
  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  switch (ch)
    {
    case 0x000A:		/* LF: Line Feed */
    case 0x000B:		/* VT: Vertical Tab */
    case 0x0085:		/* NEL: NExt Line */
    case 0x2028:		/* LS: Line Separator */
    case 0x2029:		/* PS: Paragraph Separator */
      avt_new_line ();
      break;

    case 0x000D:		/* CR: Carriage Return */
      avt_carriage_return ();
      break;

    case 0x000C:		/* FF: Form Feed */
      avt_flip_page ();
      break;

    case 0x0009:		/* HT: Horizontal Tab */
      avt_next_tab ();
      break;

    case 0x0008:		/* BS: Back Space */
      avt_backspace ();
      break;

    case 0x0007:		/* BEL */
      bell ();
      break;

      /* ignore BOM here
       * must be handled outside of the library
       */
    case 0xFEFF:
      break;

      /* LRM/RLM: only supported at the beginning of a line */
    case 0x200E:		/* LEFT-TO-RIGHT MARK (LRM) */
      avt_text_direction (AVT_LEFT_TO_RIGHT);
      break;

    case 0x200F:		/* RIGHT-TO-LEFT MARK (RLM) */
      avt_text_direction (AVT_RIGHT_TO_LEFT);
      break;

      /* other ignorable (invisible) characters */
    case 0x200B:
    case 0x200C:
    case 0x200D:
      break;

    case 0x0020:		/* SP: space */
      if (auto_margin)
	check_auto_margin ();
      if (!underlined && !inverse)
	avt_clearchar ();
      else			/* underlined or inverse */
	{
	  avt_drawchar (0x0020, screen);
	  avt_showchar ();
	}
      avt_forward ();
      /*
       * no delay for the space char
       * it'd be annoying if you have a sequence of spaces
       */
      break;

    default:
      if (ch > 0x0020 || ch == 0x0000)
	{
	  if (markup && ch == 0x005F)	/* '_' */
	    underlined = !underlined;
	  else if (markup && ch == 0x002A)	/* '*' */
	    bold = !bold;
	  else			/* not a markup character */
	    {
	      if (auto_margin)
		check_auto_margin ();
	      avt_drawchar (ch, screen);
	      avt_showchar ();
	      if (text_delay)
		SDL_Delay (text_delay);
	      avt_forward ();
	      avt_checkevent ();
	    }			/* if not markup */
	}			/* if (ch > 0x0020) */
    }				/* switch */

  return _avt_STATUS;
}

/*
 * bold can be stored as b\bbo\bol\bld\bd
 * underlinded as _\bu_\bn_\bd_\be_\br_\bl_\bi_\bn_\be_\bd
 */
static int
avt_overstrike (const wchar_t * txt)
{
  int r;

  r = 0;

  /* check if all conditions are met */
  if (!*txt || !*(txt + 1) || !*(txt + 2) || *(txt + 1) != L'\b')
    r = -1;
  else
    {
      if (*txt == L'_')
	{
	  underlined = true;
	  if (avt_put_char ((avt_char) * (txt + 2)))
	    r = -1;
	  underlined = false;
	}
      else if (*txt == *(txt + 2))
	{
	  bold = true;
	  if (avt_put_char ((avt_char) * txt))
	    r = -1;
	  bold = false;
	}
    }

  return r;
}

/*
 * writes L'\0' terminated string to textfield -
 * interprets control characters
 */
extern int
avt_say (const wchar_t * txt)
{
  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* nothing to do, when there is no text  */
  if (!txt || !*txt)
    return avt_checkevent ();

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  while (*txt != L'\0')
    {
      if (*(txt + 1) == L'\b')
	{
	  if (avt_overstrike (txt))
	    break;
	  txt += 2;
	}
      else
	{
	  avt_char c = (avt_char) * txt;
	  if (0xD800 <= *txt && *txt <= 0xDBFF)	/* UTF-16 high surrogate */
	    {
	      c = ((*txt & 0x3FF) << 10) + (*(txt + 1) & 0x3FF) + 0x10000;
	      txt++;
	    }

	  if (avt_put_char (c))
	    break;
	}
      txt++;
    }

  return _avt_STATUS;
}

/*
 * writes string with given length to textfield -
 * interprets control characters
 */
extern int
avt_say_len (const wchar_t * txt, size_t len)
{
  size_t i;

  /* nothing to do, when txt == NULL */
  /* but do allow a text to start with zeros here */
  if (!screen || !txt || _avt_STATUS != AVT_NORMAL)
    return avt_checkevent ();

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  for (i = 0; i < len; i++, txt++)
    {
      if (*(txt + 1) == L'\b' && i < len - 1)
	{
	  if (avt_overstrike (txt))
	    break;
	  txt += 2;
	  i += 2;
	}
      else
	{
	  avt_char c = (avt_char) * txt;
	  if (0xD800 <= *txt && *txt <= 0xDBFF)	/* UTF-16 high surrogate */
	    {
	      c = ((*txt & 0x3FF) << 10) + (*(txt + 1) & 0x3FF) + 0x10000;
	      txt++;
	      i++;
	    }

	  if (avt_put_char (c))
	    break;
	}
    }

  return _avt_STATUS;
}

extern int
avt_tell_len (const wchar_t * txt, size_t len)
{
  int width, height, line_length;
  size_t pos;
  const wchar_t *p;

  if (!txt || !*txt || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  width = 0;
  height = 1;
  line_length = 0;
  pos = 1;
  p = txt;

  while ((len > 0 && pos <= len) || (len == 0 && *p))
    {
      switch (*p)
	{
	case L'\n':
	case L'\v':
	case L'\x0085':	/* NEL: NExt Line */
	case L'\x2028':	/* LS: Line Separator */
	case L'\x2029':	/* PS: Paragraph Separator */
	  if (width < line_length)
	    width = line_length;
	  line_length = 0;
	  height++;
	  break;

	case L'\r':
	  if (width < line_length)
	    width = line_length;
	  line_length = 0;
	  break;

	case L'\t':
	  /* FIXME */
	  line_length += 8;
	  break;

	case L'\b':
	  line_length--;
	  break;

	case L'\a':
	case L'\xFEFF':
	case L'\x200E':
	case L'\x200F':
	case L'\x200B':
	case L'\x200C':
	case L'\x200D':
	  /* no width */
	  break;

	  /* '_' is tricky because of overstrike */
	  /* must be therefore before '*' */
	case L'_':
	  if (markup)
	    {
	      if (*(p + 1) == L'\b')
		line_length++;
	      break;
	    }
	  /* else fall through */

	case L'*':
	  if (markup)
	    break;
	  /* else fall through */

	default:
	  if ((*p >= 32 || *p == 0) && (*p < 0xD800 || *p > 0xDBFF))
	    {
	      line_length++;
	      if (auto_margin && line_length > AVT_LINELENGTH)
		{
		  width = AVT_LINELENGTH;
		  height++;
		  line_length = 0;
		}
	    }
	  break;
	}

      p++;
      pos++;
    }

  if (width < line_length)
    width = line_length;

  avt_set_balloon_size (height, width);
  avt_clear ();

  if (len > 0)
    avt_say_len (txt, len);
  else
    avt_say (txt);

  return _avt_STATUS;
}

extern int
avt_tell (const wchar_t * txt)
{
  return avt_tell_len (txt, 0);
}

extern int
avt_mb_encoding (const char *encoding)
{
  if (encoding == NULL)
    encoding = "";

  /*
   * check if it is the result of avt_get_mb_encoding()
   * or the same encoding
   */
  if (encoding == avt_encoding || SDL_strcmp (encoding, avt_encoding) == 0)
    return _avt_STATUS;

  SDL_strlcpy (avt_encoding, encoding, sizeof (avt_encoding));

  /* if encoding is "" and SYSTEMENCODING is not "" */
  if (encoding[0] == '\0' && SYSTEMENCODING[0] != '\0')
    encoding = SYSTEMENCODING;

  /* output */

  /*  if it is already open, close it first */
  if (output_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (output_cd);

  /* initialize the conversion framework */
  output_cd = avt_iconv_open (WCHAR_ENCODING, encoding);

  /* check if it was successfully initialized */
  if (output_cd == ICONV_UNINITIALIZED)
    {
      SDL_SetError ("encoding \"%s\" not supported for output", encoding);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  /* input */

  /*  if it is already open, close it first */
  if (input_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (input_cd);

  /* initialize the conversion framework */
  input_cd = avt_iconv_open (encoding, WCHAR_ENCODING);

  /* check if it was successfully initialized */
  if (input_cd == ICONV_UNINITIALIZED)
    {
      avt_iconv_close (output_cd);
      output_cd = ICONV_UNINITIALIZED;
      SDL_SetError ("encoding \"%s\" not supported for input", encoding);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  return _avt_STATUS;
}

extern char *
avt_get_mb_encoding (void)
{
  return avt_encoding;
}

/* size in bytes */
/* returns length (number of characters) */
extern size_t
avt_mb_decode_buffer (wchar_t * dest, size_t dest_size,
		      const char *src, size_t src_size)
{
  static char rest_buffer[10];
  static size_t rest_bytes = 0;
  char *outbuf;
  char *inbuf, *restbuf;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  /* check if sizes are useful */
  if (!dest || !dest_size || !src || !src_size)
    return (size_t) (-1);

  /* check if encoding was set */
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = src_size;
  inbuf = (char *) src;

  /* leave room for terminator */
  outbytesleft = dest_size - sizeof (wchar_t);
  outbuf = (char *) dest;

  restbuf = (char *) rest_buffer;

  /* if there is a rest from last call, try to complete it */
  while (rest_bytes > 0 && inbytesleft > 0)
    {
      rest_buffer[rest_bytes++] = *inbuf;
      inbuf++;
      inbytesleft--;
      returncode =
	avt_iconv (output_cd, &restbuf, &rest_bytes, &outbuf, &outbytesleft);

      if (returncode != (size_t) (-1))
	rest_bytes = 0;
      else if (errno != EINVAL)
	{
	  *((wchar_t *) outbuf) = L'\xFFFD';

	  outbuf += sizeof (wchar_t);
	  outbytesleft -= sizeof (wchar_t);
	  rest_bytes = 0;
	}
    }

  /* do the conversion */
  returncode =
    avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  /* handle invalid characters */
  while (returncode == (size_t) (-1) && errno == EILSEQ)
    {
      inbuf++;
      inbytesleft--;

      *((wchar_t *) outbuf) = L'\xFFFD';

      outbuf += sizeof (wchar_t);
      outbytesleft -= sizeof (wchar_t);
      returncode =
	avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  /* check for incomplete sequences and put them into the rest_buffer */
  if (returncode == (size_t) (-1) && errno == EINVAL
      && inbytesleft <= sizeof (rest_buffer))
    {
      rest_bytes = inbytesleft;
      SDL_memcpy ((void *) &rest_buffer, inbuf, rest_bytes);
    }

  /* ignore E2BIG - just put in as much as fits */

  /* terminate outbuf */
  *((wchar_t *) outbuf) = L'\0';

  return ((dest_size - sizeof (wchar_t) - outbytesleft) / sizeof (wchar_t));
}

/* size in bytes */
/* dest must be freed by caller */
extern size_t
avt_mb_decode (wchar_t ** dest, const char *src, size_t src_size)
{
  size_t dest_size;
  size_t length;

  if (!dest)
    return (size_t) (-1);

  *dest = NULL;

  if (!src || !src_size)
    return (size_t) (-1);

  /* get enough space */
  /* a character may be 4 Bytes, also in UTF-16  */
  /* plus the terminator */
  dest_size = src_size * 4 + sizeof (wchar_t);

  /* minimal string size */
  if (dest_size < 8)
    dest_size = 8;

  *dest = (wchar_t *) SDL_malloc (dest_size);

  if (!*dest)
    return (size_t) (-1);

  length = avt_mb_decode_buffer (*dest, dest_size, src, src_size);

  if (length == (size_t) (-1) || length == 0)
    {
      SDL_free (*dest);
      *dest = NULL;
      return (size_t) (-1);
    }

  return length;
}

extern size_t
avt_mb_encode_buffer (char *dest, size_t dest_size, const wchar_t * src,
		      size_t len)
{
  char *outbuf;
  char *inbuf;
  size_t inbytesleft, outbytesleft;

  /* check if sizes are useful */
  if (!dest || !dest_size || !src || !len)
    return (size_t) (-1);

  /* check if encoding was set */
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = len * sizeof (wchar_t);
  inbuf = (char *) src;

  /* leave room for terminator */
  outbytesleft = dest_size - sizeof (char);
  outbuf = dest;

  /* do the conversion */
  (void) avt_iconv (input_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
  /* ignore errors */

  /* terminate outbuf */
  *outbuf = '\0';

  return (dest_size - sizeof (char) - outbytesleft);
}

extern size_t
avt_mb_encode (char **dest, const wchar_t * src, size_t len)
{
  size_t dest_size, size;

  if (!dest)
    return (size_t) (-1);

  *dest = NULL;

  /* check if len is useful */
  if (!src || !len)
    return (size_t) (-1);

  /* get enough space */
  /* UTF-8 may need 4 bytes per character */
  /* +1 for the terminator */
  dest_size = len * 4 + 1;
  *dest = (char *) SDL_malloc (dest_size);

  if (!*dest)
    return (size_t) (-1);

  size = avt_mb_encode_buffer (*dest, dest_size, src, len);

  if (size == (size_t) (-1) || size == 0)
    {
      SDL_free (*dest);
      *dest = NULL;
      return (size_t) (-1);
    }

  return size;
}

extern size_t
avt_recode_buffer (const char *tocode, const char *fromcode,
		   char *dest, size_t dest_size, const char *src,
		   size_t src_size)
{
  avt_iconv_t cd;
  char *outbuf, *inbuf;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  /* check if sizes are useful */
  if (!dest || !dest_size || !src || !src_size)
    return (size_t) (-1);

  /* NULL as code means the encoding, which was set */

  if (!tocode)
    tocode = avt_encoding;

  if (!fromcode)
    fromcode = avt_encoding;

  cd = avt_iconv_open (tocode, fromcode);
  if (cd == (avt_iconv_t) (-1))
    return (size_t) (-1);

  inbuf = (char *) src;
  inbytesleft = src_size;

  /*
   * I reserve 4 Bytes for the terminator,
   * in case of using UTF-32
   */
  outbytesleft = dest_size - 4;
  outbuf = dest;

  /* do the conversion */
  returncode = avt_iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  /* jump over invalid characters */
  while (returncode == (size_t) (-1) && errno == EILSEQ)
    {
      inbuf++;
      inbytesleft--;
      returncode =
	avt_iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  /* ignore E2BIG - just put in as much as fits */

  avt_iconv_close (cd);

  /* terminate outbuf (4 Bytes were reserved) */
  SDL_memset (outbuf, 0, 4);

  return (dest_size - 4 - outbytesleft);
}

/* dest must be freed by the caller */
extern size_t
avt_recode (const char *tocode, const char *fromcode,
	    char **dest, const char *src, size_t src_size)
{
  avt_iconv_t cd;
  char *outbuf, *inbuf;
  size_t dest_size;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  if (!dest)
    return (size_t) (-1);

  *dest = NULL;

  /* check if size is useful */
  if (!src || !src_size)
    return (size_t) (-1);

  /* NULL as code means the encoding, which was set */

  if (!tocode)
    tocode = avt_encoding;

  if (!fromcode)
    fromcode = avt_encoding;

  cd = avt_iconv_open (tocode, fromcode);
  if (cd == (avt_iconv_t) (-1))
    return (size_t) (-1);

  inbuf = (char *) src;
  inbytesleft = src_size;

  /*
   * I reserve 4 Bytes for the terminator,
   * in case of using UTF-32
   */

  /* guess it's the same size */
  dest_size = src_size + 4;
  *dest = (char *) SDL_malloc (dest_size);

  if (*dest == NULL)
    {
      avt_iconv_close (cd);
      return -1;
    }

  outbuf = *dest;
  outbytesleft = dest_size - 4;	/* reserve 4 Bytes for terminator */

  /* do the conversion */
  while (inbytesleft > 0)
    {
      returncode =
	avt_iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

      /* check for fatal errors */
      if (returncode == (size_t) (-1))
	switch (errno)
	  {
	  case E2BIG:		/* needs more memory */
	    {
	      ptrdiff_t old_size = outbuf - *dest;

	      dest_size *= 2;
	      *dest = (char *) SDL_realloc (*dest, dest_size);
	      if (*dest == NULL)
		{
		  avt_iconv_close (cd);
		  return (size_t) (-1);
		}

	      outbuf = *dest + old_size;
	      outbytesleft = dest_size - old_size - 4;
	      /* reserve 4 bytes for terminator again */
	    }
	    break;

	  case EILSEQ:
	    inbuf++;		/* jump over invalid chars bytewise */
	    inbytesleft--;
	    break;

	  case EINVAL:		/* incomplete sequence */
	  default:
	    inbytesleft = 0;	/* cannot continue */
	    break;
	  }
    }

  avt_iconv_close (cd);

  /* terminate outbuf (4 Bytes were reserved) */
  SDL_memset (outbuf, 0, 4);

  return (dest_size - 4 - outbytesleft);
}

extern void
avt_free (void *ptr)
{
  if (ptr)
    SDL_free (ptr);
}

extern int
avt_say_mb (const char *txt)
{
  if (screen && _avt_STATUS == AVT_NORMAL)
    avt_say_mb_len (txt, SDL_strlen (txt));

  return _avt_STATUS;
}

extern int
avt_say_mb_len (const char *txt, size_t len)
{
  wchar_t wctext[AVT_LINELENGTH];
  char *inbuf, *outbuf;
  size_t inbytesleft, outbytesleft, nconv;
  int err;

  static char rest_buffer[10];
  static size_t rest_bytes = 0;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* check if encoding was set */
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  if (len > 0)
    inbytesleft = len;
  else
    inbytesleft = SDL_strlen (txt);

  inbuf = (char *) txt;

  /* if there is a rest from last call, try to complete it */
  while (rest_bytes > 0 && rest_bytes < sizeof (rest_buffer)
	 && inbytesleft > 0)
    {
      char *rest_buf;
      size_t rest_bytes_left;

      outbuf = (char *) wctext;
      outbytesleft = sizeof (wctext);

      rest_buffer[rest_bytes] = *inbuf;
      rest_bytes++;
      inbuf++;
      inbytesleft--;

      rest_buf = (char *) rest_buffer;
      rest_bytes_left = rest_bytes;

      nconv =
	avt_iconv (output_cd, &rest_buf, &rest_bytes_left, &outbuf,
		   &outbytesleft);
      err = errno;

      if (nconv != (size_t) (-1))	/* no error */
	{
	  avt_say_len (wctext,
		       (sizeof (wctext) - outbytesleft) / sizeof (wchar_t));
	  rest_bytes = 0;
	}
      else if (err != EINVAL)	/* any error, but incomplete sequence */
	{
	  avt_put_char (0xFFFD);	/* broken character */
	  rest_bytes = 0;
	}
    }

  /* convert and display the text */
  while (inbytesleft > 0)
    {
      outbuf = (char *) wctext;
      outbytesleft = sizeof (wctext);

      nconv =
	avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
      err = errno;

      avt_say_len (wctext,
		   (sizeof (wctext) - outbytesleft) / sizeof (wchar_t));

      if (nconv == (size_t) (-1))
	{
	  if (err == EILSEQ)	/* broken character */
	    {
	      avt_put_char (0xFFFD);
	      inbuf++;
	      inbytesleft--;
	    }
	  else if (err == EINVAL)	/* incomplete sequence */
	    {
	      rest_bytes = inbytesleft;
	      SDL_memcpy (&rest_buffer, inbuf, rest_bytes);
	      inbytesleft = 0;
	    }
	}
    }

  return _avt_STATUS;
}

/*
 * for avt_tell_mb_len  we must convert the whole text
 * or else analyzing it would be too complicated
 */
extern int
avt_tell_mb_len (const char *txt, size_t len)
{
  wchar_t *wctext;
  int wclen;

  if (len == 0)
    len = SDL_strlen (txt);

  if (screen && _avt_STATUS == AVT_NORMAL)
    {
      wclen = avt_mb_decode (&wctext, txt, len);

      if (wctext)
	{
	  avt_tell_len (wctext, wclen);
	  SDL_free (wctext);
	}
    }

  return _avt_STATUS;
}

extern int
avt_tell_mb (const char *txt)
{
  if (screen && _avt_STATUS == AVT_NORMAL)
    avt_tell_mb_len (txt, SDL_strlen (txt));

  return _avt_STATUS;
}

extern int
avt_key (avt_char * ch)
{
  SDL_Event event;

  if (screen)
    {
      *ch = 0;
      while ((*ch == 0) && (_avt_STATUS == AVT_NORMAL))
	{
	  SDL_WaitEvent (&event);
	  avt_analyze_event (&event);

	  if (SDL_KEYDOWN == event.type)
	    {
	      if (event.key.keysym.unicode)
		*ch = event.key.keysym.unicode;
	      else
		switch (event.key.keysym.sym)
		  {
		  case SDLK_UP:
		  case SDLK_KP8:
		    *ch = AVT_KEY_UP;
		    break;
		  case SDLK_DOWN:
		  case SDLK_KP2:
		    *ch = AVT_KEY_DOWN;
		    break;
		  case SDLK_RIGHT:
		  case SDLK_KP6:
		    *ch = AVT_KEY_RIGHT;
		    break;
		  case SDLK_LEFT:
		  case SDLK_KP4:
		    *ch = AVT_KEY_LEFT;
		    break;
		  case SDLK_INSERT:
		  case SDLK_KP0:
		    *ch = AVT_KEY_INSERT;
		    break;
		  case SDLK_DELETE:
		  case SDLK_KP_PERIOD:
		    *ch = AVT_KEY_DELETE;
		    break;
		  case SDLK_BACKSPACE:
		    *ch = AVT_KEY_BACKSPACE;
		    break;
		  case SDLK_HOME:
		  case SDLK_KP7:
		    *ch = AVT_KEY_HOME;
		    break;
		  case SDLK_END:
		  case SDLK_KP1:
		    *ch = AVT_KEY_END;
		    break;
		  case SDLK_PAGEUP:
		  case SDLK_KP9:
		    *ch = AVT_KEY_PAGEUP;
		    break;
		  case SDLK_PAGEDOWN:
		  case SDLK_KP3:
		    *ch = AVT_KEY_PAGEDOWN;
		    break;
		  case SDLK_HELP:
		    *ch = AVT_KEY_HELP;
		    break;
		  case SDLK_MENU:
		    *ch = AVT_KEY_MENU;
		    break;
		  case SDLK_EURO:
		    *ch = 0x20AC;
		    break;
		  case SDLK_F1:
		  case SDLK_F2:
		  case SDLK_F3:
		  case SDLK_F4:
		  case SDLK_F5:
		  case SDLK_F6:
		  case SDLK_F7:
		  case SDLK_F8:
		  case SDLK_F9:
		  case SDLK_F10:
		  case SDLK_F12:
		  case SDLK_F13:
		  case SDLK_F14:
		  case SDLK_F15:
		    *ch = AVT_KEY_F1 + (event.key.keysym.sym - SDLK_F1);
		    break;
		  case SDLK_F11:
		    if (reserve_single_keys)
		      *ch = AVT_KEY_F11;
		    break;
		  default:
		    break;
		  }		/* switch */
	    }			/* if (event.type) */
	}			/* while */
    }				/* if (screen) */

  return _avt_STATUS;
}

static void
update_menu_bar (int menu_start, int menu_end, int line_nr, int old_line,
		 SDL_Surface * plain_menu, SDL_Surface * bar)
{
  SDL_Rect s, t;

  if (line_nr != old_line)
    {
      /* restore oldline */
      if (old_line >= menu_start && old_line <= menu_end)
	{
	  s.x = 0;
	  s.y = (old_line - 1) * LINEHEIGHT;
	  s.w = viewport.w;
	  s.h = LINEHEIGHT;
	  t.x = viewport.x;
	  t.y = viewport.y + s.y;
	  SDL_BlitSurface (plain_menu, &s, screen, &t);
	  SDL_UpdateRect (screen, t.x, t.y, viewport.w, LINEHEIGHT);
	}

      /* show bar */
      if (line_nr >= menu_start && line_nr <= menu_end)
	{
	  t.x = viewport.x;
	  t.y = viewport.y + ((line_nr - 1) * LINEHEIGHT);
	  SDL_BlitSurface (bar, NULL, screen, &t);
	  SDL_UpdateRect (screen, t.x, t.y, viewport.w, LINEHEIGHT);
	}
    }
}

extern int
avt_choice (int *result, int start_line, int items, int key,
	    bool back, bool forward)
{
  SDL_Surface *plain_menu, *bar;
  SDL_Event event;
  int last_key;
  int end_line;
  int line_nr, old_line;

  if (screen && _avt_STATUS == AVT_NORMAL)
    {
      /* get a copy of the viewport */
      plain_menu = SDL_CreateRGBSurface (SDL_SWSURFACE,
					 viewport.w, viewport.h,
					 screen->format->BitsPerPixel,
					 screen->format->Rmask,
					 screen->format->Gmask,
					 screen->format->Bmask,
					 screen->format->Amask);

      if (!plain_menu)
	{
	  SDL_SetError ("out of memory");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      SDL_BlitSurface (screen, &viewport, plain_menu, NULL);

      /* prepare transparent bar */
      bar = SDL_CreateRGBSurface (SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL,
				  viewport.w, LINEHEIGHT, 8, 0, 0, 0, 128);

      if (!bar)
	{
	  SDL_FreeSurface (plain_menu);
	  SDL_SetError ("out of memory");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      /* set color for bar and make it transparent */
      SDL_FillRect (bar, NULL, 0);
      SDL_SetColors (bar, &cursor_color, 0, 1);
      SDL_SetAlpha (bar, SDL_SRCALPHA | SDL_RLEACCEL, 128);

      SDL_EventState (SDL_MOUSEMOTION, SDL_ENABLE);

      end_line = start_line + items - 1;

      if (key)
	last_key = key + items - 1;
      else
	last_key = 0;

      line_nr = -1;
      old_line = 0;
      *result = -1;
      while ((*result == -1) && (_avt_STATUS == AVT_NORMAL))
	{
	  SDL_WaitEvent (&event);
	  avt_analyze_event (&event);

	  switch (event.type)
	    {
	    case SDL_KEYDOWN:
	      if (key && (event.key.keysym.unicode >= key)
		  && (event.key.keysym.unicode <= last_key))
		*result = (int) (event.key.keysym.unicode - key + 1);
	      else if ((event.key.keysym.sym == SDLK_DOWN
			|| event.key.keysym.sym == SDLK_KP2))
		{
		  if (line_nr != end_line)
		    {
		      if (line_nr < start_line || line_nr > end_line)
			line_nr = start_line;
		      else
			line_nr++;
		      update_menu_bar (start_line, end_line, line_nr,
				       old_line, plain_menu, bar);
		      old_line = line_nr;
		    }
		  else if (forward)
		    *result = items;
		}
	      else if ((event.key.keysym.sym == SDLK_UP
			|| event.key.keysym.sym == SDLK_KP8))
		{
		  if (line_nr != start_line)
		    {
		      if (line_nr < start_line || line_nr > end_line)
			line_nr = end_line;
		      else
			line_nr--;
		      update_menu_bar (start_line, end_line, line_nr,
				       old_line, plain_menu, bar);
		      old_line = line_nr;
		    }
		  else if (back)
		    *result = 1;
		}
	      else if (back && (event.key.keysym.sym == SDLK_PAGEUP))
		*result = 1;
	      else if (forward && (event.key.keysym.sym == SDLK_PAGEDOWN))
		*result = items;
	      else if ((event.key.keysym.sym == SDLK_RETURN
			|| event.key.keysym.sym == SDLK_KP_ENTER
			|| event.key.keysym.sym == SDLK_RIGHT
			|| event.key.keysym.sym == SDLK_KP6)
		       && line_nr >= start_line && line_nr <= end_line)
		*result = line_nr - start_line + 1;
	      break;

	    case SDL_MOUSEMOTION:
	      if (event.motion.x >= viewport.x
		  && event.motion.x <= viewport.x + viewport.w
		  && event.motion.y
		  >= viewport.y + ((start_line - 1) * LINEHEIGHT)
		  && event.motion.y < viewport.y + (end_line * LINEHEIGHT))
		line_nr = ((event.motion.y - viewport.y) / LINEHEIGHT) + 1;

	      if (line_nr != old_line)
		{
		  update_menu_bar (start_line, end_line, line_nr, old_line,
				   plain_menu, bar);
		  old_line = line_nr;
		}
	      break;

	    case SDL_MOUSEBUTTONDOWN:
	      /* any of the first three buttons, but not the wheel */
	      if (event.button.button <= 3)
		{
		  /* if a valid line was chosen */
		  if (line_nr >= start_line && line_nr <= end_line)
		    *result = line_nr - start_line + 1;
		  else
		    {
		      /* check if mouse currently points to a valid line */
		      if (event.button.x >= viewport.x &&
			  event.button.x <= viewport.x + viewport.w)
			{
			  line_nr =
			    ((event.button.y - viewport.y) / LINEHEIGHT) + 1;
			  if (line_nr >= start_line && line_nr <= end_line)
			    *result = line_nr - start_line + 1;
			}
		    }
		}
	      else if (event.button.button == SDL_BUTTON_WHEELUP)
		{
		  if (line_nr != start_line)
		    {
		      if (line_nr < start_line || line_nr > end_line)
			line_nr = end_line;
		      else
			line_nr--;
		      update_menu_bar (start_line, end_line, line_nr,
				       old_line, plain_menu, bar);
		      old_line = line_nr;
		    }
		  else if (back)
		    *result = 1;
		}
	      else if (event.button.button == SDL_BUTTON_WHEELDOWN)
		{
		  if (line_nr != end_line)
		    {
		      if (line_nr < start_line || line_nr > end_line)
			line_nr = start_line;
		      else
			line_nr++;
		      update_menu_bar (start_line, end_line, line_nr,
				       old_line, plain_menu, bar);
		      old_line = line_nr;
		    }
		  else if (forward)
		    *result = items;
		}
	      break;
	    }
	}

      SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
      SDL_FreeSurface (plain_menu);
      SDL_FreeSurface (bar);
    }

  return _avt_STATUS;
}

extern void
avt_lock_updates (bool lock)
{
  hold_updates = AVT_MAKE_BOOL (lock);

  /* side effect: set text_delay to 0 */
  if (hold_updates)
    text_delay = 0;

  /* if hold_updates is not set update the textfield */
  AVT_UPDATE_TRECT (textfield);
}

static void
avt_button_inlay (SDL_Rect btn_rect, const unsigned char *bits,
		  int width, int height, int red, int green, int blue)
{
  SDL_Surface *inlay;
  SDL_Rect inlay_rect;
  int radius;

  radius = btn_rect.w / 2;
  inlay = avt_load_image_xbm (bits, width, height, red, green, blue);
  inlay_rect.w = inlay->w;
  inlay_rect.h = inlay->h;
  inlay_rect.x = btn_rect.x + radius - (inlay_rect.w / 2);
  inlay_rect.y = btn_rect.y + radius - (inlay_rect.h / 2);
  SDL_BlitSurface (inlay, NULL, screen, &inlay_rect);
  SDL_FreeSurface (inlay);
}

static size_t
avt_pager_line (const wchar_t * txt, size_t pos, size_t len)
{
  const wchar_t *tpos;
  int line_length;

  line_length = 0;
  tpos = txt + pos;

  /* skip formfeeds */
  if (*tpos == L'\f')
    tpos++;

  /* search for newline or end of text */
  while (pos + line_length < len && *(tpos + line_length) != L'\n'
	 && *(tpos + line_length) != L'\f')
    line_length++;

  avt_say_len (tpos, line_length);
  pos += line_length;

  /* skip \n | \f */
  if (pos < len)
    pos++;

  return pos;
}

static size_t
avt_pager_screen (const wchar_t * txt, size_t pos, size_t len)
{
  int line_nr;

  hold_updates = true;
  SDL_FillRect (screen, &textfield, text_background_color);

  for (line_nr = 0; line_nr < balloonheight; line_nr++)
    {
      cursor.x = linestart;
      cursor.y = line_nr * LINEHEIGHT + textfield.y;
      pos = avt_pager_line (txt, pos, len);
    }

  hold_updates = false;
  AVT_UPDATE_TRECT (textfield);

  return pos;
}

static size_t
avt_pager_lines_back (const wchar_t * txt, size_t pos, int lines)
{
  if (pos > 0)
    pos--;			/* go to last \n */

  lines--;

  while (lines--)
    {
      if (pos > 0 && *(txt + pos) == L'\n')
	pos--;			/* go before last \n */

      while (pos > 0 && *(txt + pos) != L'\n')
	pos--;
    }

  if (pos > 0)
    pos++;

  return pos;
}

extern int
avt_pager (const wchar_t * txt, size_t len, int startline)
{
  size_t pos;
  bool old_auto_margin, old_reserve_single_keys, old_tc;
  bool quit;
  avt_keyhandler old_keyhandler;
  avt_mousehandler old_mousehandler;
  void (*old_alert_func) (void);
  SDL_Event event;
  SDL_Surface *button;
  SDL_Rect btn_rect;

  if (!screen)
    return AVT_ERROR;

  /* do we actually have something to show? */
  if (!txt || !*txt || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* get len if not given */
  if (len == 0)
    len = avt_strwidth (txt);

  /* find startline */
  pos = 0;
  if (startline > 1)
    {
      int nr;

      nr = startline - 1;
      while (nr > 0 && pos < len)
	{
	  while (pos < len && *(txt + pos) != L'\n')
	    pos++;
	  pos++;
	  nr--;
	}
    }

  /* last screen */
  if (pos >= len)
    pos = avt_pager_lines_back (txt, len, balloonheight + 1);

  if (textfield.x < 0)
    avt_draw_balloon ();
  else
    viewport = textfield;

  /* show close-button */

  /* load button */
  button = avt_load_image_xpm (btn_xpm);

  /* alignment: right bottom */
  btn_rect.x = window.x + window.w - button->w - AVATAR_MARGIN;
  btn_rect.y = window.y + window.h - button->h - AVATAR_MARGIN;
  if (btn_rect.y < textfield.y + textfield.h)	/* shouldn't be clipped */
    btn_rect.y = textfield.y + textfield.h;
  btn_rect.w = button->w;
  btn_rect.h = button->h;

  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (button, NULL, screen, &btn_rect);
  avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_cancel), BUTTON_COLOR);
  SDL_FreeSurface (button);
  button = NULL;
  AVT_UPDATE_RECT (btn_rect);

  /* limit to viewport (else more problems with binary files */
  SDL_SetClipRect (screen, &viewport);

  old_tc = text_cursor_visible;
  text_cursor_visible = false;
  old_auto_margin = auto_margin;
  auto_margin = false;
  old_reserve_single_keys = reserve_single_keys;
  reserve_single_keys = false;
  old_keyhandler = avt_ext_keyhandler;
  avt_ext_keyhandler = NULL;
  old_mousehandler = avt_ext_mousehandler;
  avt_ext_mousehandler = NULL;

  /* temporarily disable the alert function */
  old_alert_func = avt_alert_func;
  avt_alert_func = NULL;

  avt_set_text_delay (0);
  if (markup)
    {
      avt_normal_text ();
      markup = true;
    }
  else
    avt_normal_text ();

  /* show first screen */
  pos = avt_pager_screen (txt, pos, len);

  /* last screen */
  if (pos >= len)
    {
      pos = avt_pager_lines_back (txt, len, balloonheight + 1);
      pos = avt_pager_screen (txt, pos, len);
    }

  quit = false;

  while (!quit && _avt_STATUS == AVT_NORMAL)
    {
      SDL_WaitEvent (&event);
      avt_analyze_event (&event);

      switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	  if (event.button.button == SDL_BUTTON_WHEELDOWN)
	    event.key.keysym.sym = SDLK_DOWN;
	  else if (event.button.button == SDL_BUTTON_WHEELUP)
	    event.key.keysym.sym = SDLK_UP;
	  else if (event.button.button == SDL_BUTTON_MIDDLE)	/* press on wheel */
	    {
	      quit = true;
	      break;
	    }
	  else if (event.button.button <= 3
		   && event.button.y >= btn_rect.y
		   && event.button.y <= btn_rect.y + btn_rect.h
		   && event.button.x >= btn_rect.x
		   && event.button.x <= btn_rect.x + btn_rect.w)
	    {
	      quit = true;
	      break;
	    }
	  else
	    break;

	  /* deliberate fallthrough here */

	case SDL_KEYDOWN:
	  if (event.key.keysym.sym == SDLK_DOWN
	      || event.key.keysym.sym == SDLK_KP2)
	    {
	      /* if it's not the end */
	      if (pos < len)
		{
		  hold_updates = true;
		  avt_delete_lines (1, 1);
		  cursor.x = linestart;
		  cursor.y = (balloonheight - 1) * LINEHEIGHT + textfield.y;
		  pos = avt_pager_line (txt, pos, len);
		  hold_updates = false;
		  AVT_UPDATE_TRECT (textfield);
		}
	    }
	  else if (event.key.keysym.sym == SDLK_PAGEDOWN
		   || event.key.keysym.sym == SDLK_KP3
		   || event.key.keysym.sym == SDLK_SPACE
		   || event.key.keysym.sym == SDLK_f)
	    {
	      if (pos < len)
		{
		  pos = avt_pager_screen (txt, pos, len);
		  if (pos >= len)
		    {
		      pos =
			avt_pager_lines_back (txt, len, balloonheight + 1);
		      pos = avt_pager_screen (txt, pos, len);
		    }
		}
	    }
	  else if (event.key.keysym.sym == SDLK_UP
		   || event.key.keysym.sym == SDLK_KP8)
	    {
	      int start_pos;

	      start_pos = avt_pager_lines_back (txt, pos, balloonheight + 2);

	      if (start_pos == 0)
		pos = avt_pager_screen (txt, 0, len);
	      else
		{
		  hold_updates = true;
		  avt_insert_lines (1, 1);
		  cursor.x = linestart;
		  cursor.y = textfield.y;
		  avt_pager_line (txt, start_pos, len);
		  hold_updates = false;
		  AVT_UPDATE_TRECT (textfield);
		  pos = avt_pager_lines_back (txt, pos, 2);
		}
	    }
	  else if (event.key.keysym.sym == SDLK_PAGEUP
		   || event.key.keysym.sym == SDLK_KP9
		   || event.key.keysym.sym == SDLK_b)
	    {
	      pos = avt_pager_lines_back (txt, pos, 2 * balloonheight + 1);
	      pos = avt_pager_screen (txt, pos, len);
	    }
	  else if (event.key.keysym.sym == SDLK_HOME
		   || event.key.keysym.sym == SDLK_KP7)
	    pos = avt_pager_screen (txt, 0, len);
	  else if (event.key.keysym.sym == SDLK_END
		   || event.key.keysym.sym == SDLK_KP1)
	    {
	      pos = avt_pager_lines_back (txt, len, balloonheight + 1);
	      pos = avt_pager_screen (txt, pos, len);
	    }
	  else if (event.key.keysym.sym == SDLK_q)
	    quit = true;	/* Q with any combination-key quits */
	  break;
	}
    }

  /* quit request only quits the pager here */
  if (_avt_STATUS == AVT_QUIT)
    _avt_STATUS = AVT_NORMAL;

  /* remove button */
  SDL_SetClipRect (screen, &window);
  SDL_FillRect (screen, &btn_rect, background_color);
  AVT_UPDATE_RECT (btn_rect);
  SDL_SetClipRect (screen, &viewport);

  auto_margin = old_auto_margin;
  reserve_single_keys = old_reserve_single_keys;
  avt_ext_keyhandler = old_keyhandler;
  avt_ext_mousehandler = old_mousehandler;
  avt_activate_cursor (old_tc);

  avt_alert_func = old_alert_func;

  return _avt_STATUS;
}

/*
 * Note: in the beginning I only had avt_pager_mb
 * and converted it line by line to save memory.
 * But that broke with UTF-16 and UTF-32.
 */
extern int
avt_pager_mb (const char *txt, size_t len, int startline)
{
  wchar_t *wctext;
  int wclen;

  if (screen && txt && _avt_STATUS == AVT_NORMAL)
    {
      if (len == 0)
	len = SDL_strlen (txt);

      wclen = avt_mb_decode (&wctext, txt, len);

      if (wctext)
	{
	  avt_pager (wctext, wclen, startline);
	  SDL_free (wctext);
	}
    }

  return _avt_STATUS;
}

/* size in Bytes! */
extern int
avt_ask (wchar_t * s, size_t size)
{
  avt_char ch;
  size_t len, maxlen, pos;
  int old_textdir;
  bool insert_mode;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* this function only works with left to right text */
  /* it would be too hard otherwise */
  old_textdir = textdir_rtl;
  textdir_rtl = AVT_LEFT_TO_RIGHT;

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  /* if the cursor is beyond the end of the viewport,
   * get a new page
   */
  if (cursor.y > viewport.y + viewport.h - LINEHEIGHT)
    avt_flip_page ();

  /* maxlen is the rest of line */
  if (textdir_rtl)
    maxlen = (cursor.x - viewport.x) / FONTWIDTH;
  else
    maxlen = ((viewport.x + viewport.w) - cursor.x) / FONTWIDTH;

  /* does it fit in the buffer size? */
  if (maxlen > size / sizeof (wchar_t) - 1)
    maxlen = size / sizeof (wchar_t) - 1;

  /* clear the input field */
  avt_erase_characters (maxlen);

  len = pos = 0;
  insert_mode = true;
  SDL_memset (s, 0, size);
  ch = 0;

  do
    {
      /* show cursor */
      avt_show_text_cursor (true);

      if (avt_key (&ch) != AVT_NORMAL)
	break;

      switch (ch)
	{
	case AVT_KEY_ENTER:
	  break;

	case AVT_KEY_HOME:
	  avt_show_text_cursor (false);
	  cursor.x -= pos * FONTWIDTH;
	  pos = 0;
	  break;

	case AVT_KEY_END:
	  avt_show_text_cursor (false);
	  if (len < maxlen)
	    {
	      cursor.x += (len - pos) * FONTWIDTH;
	      pos = len;
	    }
	  else
	    {
	      cursor.x += (maxlen - 1 - pos) * FONTWIDTH;
	      pos = maxlen - 1;
	    }
	  break;

	case AVT_KEY_BACKSPACE:
	  if (pos > 0)
	    {
	      pos--;
	      avt_show_text_cursor (false);
	      avt_backspace ();
	      avt_delete_characters (1);
	      SDL_memmove (&s[pos], &s[pos + 1],
			   (len - pos - 1) * sizeof (wchar_t));
	      len--;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_DELETE:
	  if (pos < len)
	    {
	      avt_show_text_cursor (false);
	      avt_delete_characters (1);
	      SDL_memmove (&s[pos], &s[pos + 1],
			   (len - pos - 1) * sizeof (wchar_t));
	      len--;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_LEFT:
	  if (pos > 0)
	    {
	      pos--;
	      /* delete cursor */
	      avt_show_text_cursor (false);
	      cursor.x -= FONTWIDTH;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_RIGHT:
	  if (pos < len && pos < maxlen - 1)
	    {
	      pos++;
	      avt_show_text_cursor (false);
	      cursor.x += FONTWIDTH;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_INSERT:
	  insert_mode = !insert_mode;
	  break;

	default:
	  if (pos < maxlen && ch >= 32 && get_font_char (ch) != NULL)
	    {
	      /* delete cursor */
	      avt_show_text_cursor (false);
	      if (insert_mode && pos < len)
		{
		  avt_insert_spaces (1);
		  if (len < maxlen)
		    {
		      SDL_memmove (&s[pos + 1], &s[pos],
				   (len - pos) * sizeof (wchar_t));
		      len++;
		    }
		  else		/* len >= maxlen */
		    SDL_memmove (&s[pos + 1], &s[pos],
				 (len - pos - 1) * sizeof (wchar_t));
		}
	      s[pos] = (wchar_t) ch;
	      avt_drawchar (ch, screen);
	      avt_showchar ();
	      pos++;
	      if (pos > len)
		len++;
	      if (pos > maxlen - 1)
		{
		  pos--;	/* cursor stays where it is */
		  bell ();
		}
	      else
		cursor.x =
		  (textdir_rtl) ? cursor.x - FONTWIDTH : cursor.x + FONTWIDTH;
	    }
	}
    }
  while ((ch != AVT_KEY_ENTER) && (_avt_STATUS == AVT_NORMAL));

  s[len] = L'\0';

  /* delete cursor */
  avt_show_text_cursor (false);

  if (!newline_mode)
    avt_carriage_return ();

  avt_new_line ();

  textdir_rtl = old_textdir;

  return _avt_STATUS;
}

extern int
avt_ask_mb (char *s, size_t size)
{
  wchar_t ws[AVT_LINELENGTH + 1];
  char *inbuf;
  size_t inbytesleft, outbytesleft;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* check if encoding was set */
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  avt_ask (ws, sizeof (ws));

  s[0] = '\0';

  /* if a halt is requested, don't bother with the conversion */
  if (_avt_STATUS)
    return _avt_STATUS;

  /* prepare the buffer */
  inbuf = (char *) &ws;
  inbytesleft = (avt_strwidth (ws) + 1) * sizeof (wchar_t);
  outbytesleft = size;

  /* do the conversion */
  avt_iconv (input_cd, &inbuf, &inbytesleft, &s, &outbytesleft);

  return _avt_STATUS;
}

extern int
avt_move_in (void)
{
  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* fill the screen with background color */
  /* (not only the window!) */
  avt_clear_screen ();

  /* undefine textfield */
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;

  if (avatar_image)
    {
      SDL_Rect dst;
      Uint32 start_time;
      SDL_Rect mywindow;

      /*
       * mywindow is like window,
       * but to the edge of the screen on the right
       */
      mywindow = window;
      mywindow.w = screen->w - mywindow.x;

      dst.x = screen->w;
      /* bottom */
      dst.y = mywindow.y + mywindow.h - avatar_image->h - AVATAR_MARGIN;
      dst.w = avatar_image->w;
      dst.h = avatar_image->h;
      start_time = SDL_GetTicks ();

      while (dst.x > mywindow.x + AVATAR_MARGIN)
	{
	  Sint16 oldx = dst.x;

	  /* move */
	  dst.x = screen->w - ((SDL_GetTicks () - start_time) / MOVE_DELAY);

	  if (dst.x != oldx)
	    {
	      /* draw */
	      SDL_BlitSurface (avatar_image, NULL, screen, &dst);

	      /* update */
	      if ((oldx + dst.w) >= screen->w)
		SDL_UpdateRect (screen, dst.x, dst.y,
				screen->w - dst.x, dst.h);
	      else
		SDL_UpdateRect (screen, dst.x, dst.y,
				dst.w + (oldx - dst.x), dst.h);

	      /* delete (not visibly yet) */
	      SDL_FillRect (screen, &dst, background_color);
	    }

	  /* check event */
	  if (avt_checkevent ())
	    return _avt_STATUS;

	  /* some time for other processes */
	  SDL_Delay (1);
	}

      /* final position */
      avt_show_avatar ();
    }

  return _avt_STATUS;
}

extern int
avt_move_out (void)
{
  if (!screen || !avt_visible || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* needed to remove the balloon */
  avt_show_avatar ();

  /*
   * remove clipping
   */
  SDL_SetClipRect (screen, NULL);

  if (avatar_image)
    {
      SDL_Rect dst;
      Uint32 start_time;
      Sint16 start_position;
      SDL_Rect mywindow;

      /*
       * mywindow is like window,
       * but to the edge of the screen on the right
       */
      mywindow = window;
      mywindow.w = screen->w - mywindow.x;

      start_position = mywindow.x + AVATAR_MARGIN;
      dst.x = start_position;
      /* bottom */
      dst.y = mywindow.y + mywindow.h - avatar_image->h - AVATAR_MARGIN;
      dst.w = avatar_image->w;
      dst.h = avatar_image->h;
      start_time = SDL_GetTicks ();

      /* delete (not visibly yet) */
      SDL_FillRect (screen, &dst, background_color);

      while (dst.x < screen->w)
	{
	  Sint16 oldx;

	  oldx = dst.x;

	  /* move */
	  dst.x =
	    start_position + ((SDL_GetTicks () - start_time) / MOVE_DELAY);

	  if (dst.x != oldx)
	    {
	      /* draw */
	      SDL_BlitSurface (avatar_image, NULL, screen, &dst);

	      /* update */
	      if ((dst.x + dst.w) >= screen->w)
		SDL_UpdateRect (screen, oldx, dst.y, screen->w - oldx, dst.h);
	      else
		SDL_UpdateRect (screen, oldx, dst.y,
				dst.w + dst.x - oldx, dst.h);

	      /* delete (not visibly yet) */
	      SDL_FillRect (screen, &dst, background_color);
	    }

	  /* check event */
	  if (avt_checkevent ())
	    return _avt_STATUS;

	  /* some time for other processes */
	  SDL_Delay (1);
	}
    }

  /* fill the whole screen with background color */
  avt_clear_screen ();

  return _avt_STATUS;
}

extern int
avt_wait_button (void)
{
  SDL_Event event;
  SDL_Surface *button;
  SDL_Rect btn_rect;
  bool nokey;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* load button */
  button = avt_load_image_xpm (btn_xpm);

  /* alignment: right bottom */
  btn_rect.x = window.x + window.w - button->w - AVATAR_MARGIN;
  btn_rect.y = window.y + window.h - button->h - AVATAR_MARGIN;
  btn_rect.w = button->w;
  btn_rect.h = button->h;

  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (button, NULL, screen, &btn_rect);
  SDL_FreeSurface (button);
  button = NULL;

  avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_right), BUTTON_COLOR);
  AVT_UPDATE_RECT (btn_rect);

  nokey = true;
  while (nokey)
    {
      SDL_WaitEvent (&event);
      switch (event.type)
	{
	case SDL_QUIT:
	  nokey = false;
	  _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_KEYDOWN:
	  if (SDLK_F11 != event.key.keysym.sym || reserve_single_keys)
	    nokey = false;
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  /* ignore the wheel */
	  if (event.button.button <= 3)
	    nokey = false;
	  break;
	}

      /* do other stuff */
      avt_analyze_event (&event);
    }

  /* delete button */
  /* TODO: save/restore background */
  SDL_SetClipRect (screen, &window);
  SDL_FillRect (screen, &btn_rect, background_color);
  AVT_UPDATE_RECT (btn_rect);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return _avt_STATUS;
}

/*
 * maximum number of displayable navigation buttons
 * for the smallest resolution
 */
#define NAV_MAX 15

extern int
avt_navigate (const char *buttons)
{
  SDL_Event event;
  SDL_Surface *base_button, *buttons_area;
  SDL_Rect rect[NAV_MAX], buttons_rect;
  int i, button_count, button_pos, audio_end_button;
  int result;

  result = AVT_ERROR;		/* no result */
  audio_end_button = false;	/* none */

  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (!screen)
    return AVT_ERROR;

  button_count = SDL_strlen (buttons);

  if (!buttons || !*buttons || button_count > NAV_MAX)
    {
      SDL_SetError ("No or too many buttons for navigation bar");
      return AVT_FAILURE;
    }

  SDL_SetClipRect (screen, &window);

  /* load base button image */
  base_button = avt_load_image_xpm (btn_xpm);

  /* common button area */
  buttons_rect.y = window.y + window.h - base_button->h - AVATAR_MARGIN;
  buttons_rect.x = window.x + window.w - AVATAR_MARGIN
    - (button_count * (base_button->w + BUTTON_DISTANCE)) + BUTTON_DISTANCE;
  buttons_rect.h = base_button->h;
  buttons_rect.w = window.w - AVATAR_MARGIN - buttons_rect.x;

  /* yet another check, if there are too many buttons */
  if (buttons_rect.x < 0)
    {
      SDL_FreeSurface (base_button);
      SDL_SetError ("too many buttons");
      return AVT_FAILURE;
    }

  /* save background for common button area */
  buttons_area =
    SDL_CreateRGBSurface (SDL_SWSURFACE, buttons_rect.w, buttons_rect.h,
			  screen->format->BitsPerPixel,
			  screen->format->Rmask, screen->format->Gmask,
			  screen->format->Bmask, screen->format->Amask);

  SDL_BlitSurface (screen, &buttons_rect, buttons_area, NULL);

  /* common values for button rectangles */
  for (i = 0; i < NAV_MAX; i++)
    {
      rect[i].w = base_button->w;
      rect[i].h = base_button->h;
      rect[i].x = 0;		/* changed later */
      rect[i].y = buttons_rect.y;
    }

#define avt_nav_inlay(bt) \
  avt_button_inlay(rect[i],bt##_bits,bt##_width,bt##_height,BUTTON_COLOR)

  button_pos = buttons_rect.x;
  for (i = 0; i < button_count; i++)
    {
      rect[i].x = button_pos;

      /* show base button, if it's not a spacer */
      if (buttons[i] != ' ')
	SDL_BlitSurface (base_button, NULL, screen, &rect[i]);

      switch (buttons[i])
	{
	case 'l':
	  avt_nav_inlay (btn_left);
	  break;

	case 'd':
	  avt_nav_inlay (btn_down);
	  break;

	case 'u':
	  avt_nav_inlay (btn_up);
	  break;

	case 'r':
	  avt_nav_inlay (btn_right);
	  break;

	case 'x':
	  avt_nav_inlay (btn_cancel);
	  break;

	case 's':
	  avt_nav_inlay (btn_stop);
	  if (!audio_end_button)	/* 'f' has precedence */
	    audio_end_button = 's';
	  break;

	case 'f':
	  avt_nav_inlay (btn_fastforward);
	  audio_end_button = 'f';	/* this has precedence */
	  break;

	case 'b':
	  avt_nav_inlay (btn_fastbackward);
	  break;

	case '+':
	  avt_nav_inlay (btn_yes);
	  break;

	case '-':
	  avt_nav_inlay (btn_no);
	  break;

	case 'p':
	  avt_nav_inlay (btn_pause);
	  break;

	case '?':
	  avt_nav_inlay (btn_help);
	  break;

	case 'e':
	  avt_nav_inlay (btn_eject);
	  break;

	case '*':
	  avt_nav_inlay (btn_circle);
	  break;

	default:
	  /*
	   * empty button allowed
	   * for compatibility to buttons in later versions
	   */
	  break;
	}

      button_pos += base_button->w + BUTTON_DISTANCE;
    }

  SDL_FreeSurface (base_button);
  base_button = NULL;

  /* show all buttons */
  AVT_UPDATE_RECT (buttons_rect);

  while (result < 0 && _avt_STATUS == AVT_NORMAL)
    {
      SDL_WaitEvent (&event);

      switch (event.type)
	{
	  /* end of audio triggers stop key */
	case SDL_USEREVENT:
	  if (event.user.code == AVT_AUDIO_ENDED && audio_end_button)
	    result = audio_end_button;
	  break;

	case SDL_KEYDOWN:
	  {
	    int r = -1;

	    if (event.key.keysym.sym == SDLK_UP
		|| event.key.keysym.sym == SDLK_KP8
		|| event.key.keysym.sym == SDLK_HOME
		|| event.key.keysym.sym == SDLK_KP7)
	      r = 'u';
	    else if (event.key.keysym.sym == SDLK_DOWN
		     || event.key.keysym.sym == SDLK_KP2
		     || event.key.keysym.sym == SDLK_END
		     || event.key.keysym.sym == SDLK_KP1)
	      r = 'd';
	    else if (event.key.keysym.sym == SDLK_LEFT
		     || event.key.keysym.sym == SDLK_KP4)
	      r = 'l';
	    else if (event.key.keysym.sym == SDLK_RIGHT
		     || event.key.keysym.sym == SDLK_KP6)
	      r = 'r';
	    else if (event.key.keysym.sym == SDLK_HELP
		     || event.key.keysym.sym == SDLK_F1)
	      r = '?';
	    else if (event.key.keysym.sym == SDLK_PAUSE)
	      {
		r = 'p';
		/* prevent further handling of the Pause-key */
		event.key.keysym.sym = SDLK_UNKNOWN;
	      }
	    else if (event.key.keysym.unicode > 32
		     && event.key.keysym.unicode < 127)
	      r = event.key.keysym.unicode;

	    /* check if it is one of the requested characters */
	    {
	      const char *b = buttons;
	      while (*b)
		if (r == (int) *b++)
		  result = r;
	    }
	  }
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  if (event.button.button <= 3
	      && event.button.y >= buttons_rect.y
	      && event.button.y <= buttons_rect.y + buttons_rect.h
	      && event.button.x >= buttons_rect.x
	      && event.button.x <= buttons_rect.x + buttons_rect.w)
	    {
	      for (i = 0; i < button_count && result < 0; i++)
		{
		  if (buttons[i] != ' '
		      && event.button.x >= rect[i].x
		      && event.button.x <= rect[i].x + rect[i].w)
		    result = buttons[i];
		}
	    }
	  break;
	}

      avt_analyze_event (&event);
    }

  /* restore background */
  SDL_BlitSurface (buttons_area, NULL, screen, &buttons_rect);
  SDL_FreeSurface (buttons_area);
  AVT_UPDATE_RECT (buttons_rect);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  if (_avt_STATUS != AVT_NORMAL)
    result = _avt_STATUS;

  return result;
}

extern bool
avt_decide (void)
{
  SDL_Event event;
  SDL_Surface *base_button;
  SDL_Rect yes_rect, no_rect;
  int result;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  SDL_SetClipRect (screen, &window);

  /* show buttons */
  base_button = avt_load_image_xpm (btn_xpm);

  /* alignment: right bottom */
  yes_rect.x = window.x + window.w - base_button->w - AVATAR_MARGIN;
  yes_rect.y = window.y + window.h - base_button->h - AVATAR_MARGIN;
  yes_rect.w = base_button->w;
  yes_rect.h = base_button->h;
  SDL_BlitSurface (base_button, NULL, screen, &yes_rect);
  avt_button_inlay (yes_rect, AVT_XBM_INFO (btn_yes), 0, 0xAA, 0);

  no_rect.x = yes_rect.x - BUTTON_DISTANCE - base_button->w;
  no_rect.y = yes_rect.y;
  no_rect.w = base_button->w;
  no_rect.h = base_button->h;
  SDL_BlitSurface (base_button, NULL, screen, &no_rect);
  avt_button_inlay (no_rect, AVT_XBM_INFO (btn_no), 0xAA, 0, 0);

  AVT_UPDATE_RECT (no_rect);
  AVT_UPDATE_RECT (yes_rect);

  SDL_FreeSurface (base_button);
  base_button = NULL;

  result = -1;			/* no result */
  while (result < 0)
    {
      SDL_WaitEvent (&event);
      switch (event.type)
	{
	case SDL_QUIT:
	  result = false;
	  _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_KEYDOWN:
	  if (event.key.keysym.sym == SDLK_ESCAPE)
	    {
	      result = false;
	      _avt_STATUS = AVT_QUIT;
	    }
	  else if (event.key.keysym.unicode == L'-'
		   || event.key.keysym.unicode == L'0'
		   || event.key.keysym.sym == SDLK_BACKSPACE)
	    result = false;
	  else if (event.key.keysym.unicode == L'+'
		   || event.key.keysym.unicode == L'1'
		   || event.key.keysym.unicode == L'\r')
	    result = true;
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  /* assume both buttons have the same height */
	  /* any mouse button, but ignore the wheel */
	  if (event.button.button <= 3
	      && event.button.y >= yes_rect.y
	      && event.button.y <= yes_rect.y + yes_rect.h)
	    {
	      if (event.button.x >= yes_rect.x
		  && event.button.x <= yes_rect.x + yes_rect.w)
		result = true;
	      else
		if (event.button.x >= no_rect.x
		    && event.button.x <= no_rect.x + no_rect.w)
		result = false;
	    }
	  break;
	}

      avt_analyze_event (&event);
    }

  /* delete buttons */
  /* TODO: save/restore background */
  SDL_SetClipRect (screen, &window);
  SDL_FillRect (screen, &no_rect, background_color);
  SDL_FillRect (screen, &yes_rect, background_color);
  AVT_UPDATE_RECT (no_rect);
  AVT_UPDATE_RECT (yes_rect);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return AVT_MAKE_BOOL (result);
}


#ifndef DISABLE_DEPRECATED
extern void
avt_free_image (avt_image_t * image)
{
  SDL_FreeSurface (image);
}
#endif


static void
avt_show_image (SDL_Surface * image)
{
  SDL_Rect dst;

  /* clear the screen */
  avt_free_screen ();

  /* set informational variables */
  avt_visible = false;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;

  /* center image on screen */
  dst.x = (screen->w / 2) - (image->w / 2);
  dst.y = (screen->h / 2) - (image->h / 2);
  dst.w = image->w;
  dst.h = image->h;

  /* if image is larger than the window,
   * just the upper left part is shown, as far as it fits
   */
  SDL_BlitSurface (image, NULL, screen, &dst);
  AVT_UPDATE_ALL ();
  SDL_SetClipRect (screen, &window);
  avt_checkevent ();
}

/*
 * load image
 * if SDL_image isn't available then
 * XPM and uncompressed BMP are still supported
 */
extern int
avt_show_image_file (const char *filename)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  RW = SDL_RWFromFile (filename, "rb");

  /* try internal XPM reader first */
  /* it's better than in SDL_image */
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);
    }

  SDL_RWclose (RW);

  if (image == NULL)
    {
      avt_clear_screen ();	/* at least clear the screen */
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

extern int
avt_show_image_stream (avt_stream * stream)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  RW = SDL_RWFromFP ((FILE *) stream, 0);

  /* try internal XPM reader first */
  /* it's better than in SDL_image */
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);
    }

  SDL_RWclose (RW);

  if (image == NULL)
    {
      avt_clear_screen ();	/* at least clear the screen */
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

/*
 * show image from image data
 */
extern int
avt_show_image_data (void *img, size_t imgsize)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  RW = SDL_RWFromMem (img, imgsize);

  /* try internal XPM reader first */
  /* it's better than in SDL_image */
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);
    }

  SDL_RWclose (RW);

  if (image == NULL)
    {
      avt_clear_screen ();	/* at least clear the screen */
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

extern int
avt_show_image_xpm (char **xpm)
{
  SDL_Surface *image = NULL;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  image = avt_load_image_xpm (xpm);

  if (image == NULL)
    {
      avt_clear_screen ();	/* at least clear the screen */
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

extern int
avt_show_image_xbm (const unsigned char *bits, int width, int height,
		    const char *colorname)
{
  SDL_Surface *image;
  int red, green, blue;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (width <= 0 || height <= 0)
    return AVT_FAILURE;

  if (avt_name_to_color (colorname, &red, &green, &blue) < 0)
    {
      avt_clear ();		/* at least clear the balloon */
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  image = avt_load_image_xbm (bits, width, height, red, green, blue);

  if (image == NULL)
    {
      avt_clear_screen ();	/* at least clear the screen */
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

extern int
avt_image_max_width (void)
{
  return screen->w;
}

extern int
avt_image_max_height (void)
{
  return screen->h;
}

/*
 * show raw image
 * only 3 or 4 Bytes per pixel supported (RGB or RGBA)
 */
extern int
avt_show_raw_image (void *image_data, int width, int height,
		    int bytes_per_pixel)
{
  SDL_Surface *image;

  if (!screen || _avt_STATUS != AVT_NORMAL || !image_data)
    return _avt_STATUS;

  if (bytes_per_pixel < 3 || bytes_per_pixel > 4)
    {
      SDL_SetError ("wrong number of bytes_per_pixel for raw image");
      return AVT_FAILURE;
    }

  image = NULL;

  /* the wrong endianess can be optimized away while compiling */
  if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
    {
      if (bytes_per_pixel == 3)
	image = SDL_CreateRGBSurfaceFrom (image_data, width, height,
					  3 * 8, 3 * width,
					  0xFF0000, 0x00FF00, 0x0000FF, 0);
      else if (bytes_per_pixel == 4)
	image = SDL_CreateRGBSurfaceFrom (image_data, width, height,
					  4 * 8, 4 * width,
					  0xFF000000, 0x00FF0000, 0x0000FF00,
					  0x000000FF);
    }
  else				/* little endian */
    {
      if (bytes_per_pixel == 3)
	image = SDL_CreateRGBSurfaceFrom (image_data, width, height,
					  3 * 8, 3 * width,
					  0x0000FF, 0x00FF00, 0xFF0000, 0);
      else if (bytes_per_pixel == 4)
	image = SDL_CreateRGBSurfaceFrom (image_data, width, height,
					  4 * 8, 4 * width,
					  0x000000FF, 0x0000FF00, 0x00FF0000,
					  0xFF000000);
    }

  if (image == NULL)
    {
      avt_clear_screen ();	/* at least clear the screen */
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

static int
avt_init_SDL (void)
{
  /* only if not already initialized */
  if (SDL_WasInit (SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0)
    {
      /* don't try to use the mouse
       * might be needed for the fbcon driver
       * the mouse still works, it is just not required
       */
      SDL_putenv ("SDL_NOMOUSE=1");

      SDL_SetError ("15ce822f94d7e8e4281f1c2bcdd7c56d");

      if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	  _avt_STATUS = AVT_ERROR;
    }

  return _avt_STATUS;
}

/*
 * make background transparent
 * pixel in the upper left corner is supposed to be the background color
 */
/* deprecated, but needed internally */
extern avt_image_t *
avt_make_transparent (avt_image_t * image)
{
  Uint32 color;

  if (SDL_MUSTLOCK (image))
    SDL_LockSurface (image);

  /* get color of upper left corner */
  color = getpixel (image, 0, 0);

  if (SDL_MUSTLOCK (image))
    SDL_UnlockSurface (image);

  if (!SDL_SetColorKey (image, SDL_SRCCOLORKEY | SDL_RLEACCEL, color))
    image = NULL;

  return image;
}


#ifndef DISABLE_DEPRECATED

/* deprecated */
extern avt_image_t *
avt_import_xpm (char **xpm)
{
  if (avt_init_SDL ())
    return NULL;

  return avt_load_image_xpm (xpm);
}

/* deprecated */
extern avt_image_t *
avt_import_xbm (const unsigned char *bits, int width, int height,
		const char *colorname)
{
  int red, green, blue;

  if (width <= 0 || height <= 0)
    return NULL;

  if (avt_name_to_color (colorname, &red, &green, &blue) < 0)
    return NULL;

  if (avt_init_SDL ())
    return NULL;

  return avt_load_image_xbm (bits, width, height, red, green, blue);
}

/* deprecated */
extern avt_image_t *
avt_import_gimp_image (void *gimp_image)
{
  SDL_Surface *image;
  gimp_img_t *img;

  if (avt_init_SDL ())
    return NULL;

  img = (gimp_img_t *) gimp_image;

  if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
    image = SDL_CreateRGBSurfaceFrom (&img->pixel_data,
				      img->width, img->height, 3 * 8,
				      img->width * 3, 0xFF0000, 0x00FF00,
				      0x0000FF, 0);
  else
    image = SDL_CreateRGBSurfaceFrom (&img->pixel_data,
				      img->width, img->height, 3 * 8,
				      img->width * 3, 0x0000FF, 0x00FF00,
				      0xFF0000, 0);

  avt_make_transparent (image);

  return image;
}

/* deprecated */
extern avt_image_t *
avt_import_image_data (void *img, size_t imgsize)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (avt_init_SDL ())
    return NULL;

  RW = SDL_RWFromMem (img, imgsize);

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  SDL_RWclose (RW);

  return image;
}

/* deprecated */
extern avt_image_t *
avt_import_image_file (const char *filename)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (avt_init_SDL ())
    return NULL;

  RW = SDL_RWFromFile (filename, "rb");

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  SDL_RWclose (RW);

  return image;
}

/* deprecated */
extern avt_image_t *
avt_import_image_stream (avt_stream * stream)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (avt_init_SDL ())
    return NULL;

  RW = SDL_RWFromFP ((FILE *) stream, 0);

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  SDL_RWclose (RW);

  return image;
}

#endif /* DISABLE_DEPRECATED */


static int
calculate_balloonmaxheight (void)
{
  int avatar_height;

  avatar_height = avatar_image ? avatar_image->h + AVATAR_MARGIN : 0;

  balloonmaxheight = (window.h - avatar_height - (2 * TOPMARGIN)
		      - (2 * BALLOON_INNER_MARGIN)) / LINEHEIGHT;

  /* check, whether image is too high */
  /* at least 10 lines */
  if (balloonmaxheight < 10)
    {
      SDL_SetError ("Avatar image too large");
      _avt_STATUS = AVT_ERROR;
      SDL_FreeSurface (avatar_image);
      avatar_image = NULL;
    }

  return _avt_STATUS;
}

/* change avatar image and (re)calculate balloon size */
static int
avt_set_avatar_image (SDL_Surface * image)
{
  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (avt_visible)
    avt_clear_screen ();

  /* free old image */
  if (avatar_image)
    {
      SDL_FreeSurface (avatar_image);
      avatar_image = NULL;
    }

  if (avt_name)
    {
      SDL_free (avt_name);
      avt_name = NULL;
    }

  /* import the avatar image */
  if (image)
    {
      /* convert image to display-format for faster drawing */
      if (image->flags & SDL_SRCALPHA)
	avatar_image = SDL_DisplayFormatAlpha (image);
      else
	avatar_image = SDL_DisplayFormat (image);

      if (!avatar_image)
	{
	  SDL_SetError ("couldn't load avatar");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}
    }

  calculate_balloonmaxheight ();

  /* set actual balloon size to the maximum size */
  balloonheight = balloonmaxheight;
  balloonwidth = AVT_LINELENGTH;

  return _avt_STATUS;
}


#ifndef DISABLE_DEPRECATED
extern int
avt_change_avatar_image (avt_image_t * image)
{
  avt_set_avatar_image (image);

  if (image)
    SDL_FreeSurface (image);

  return _avt_STATUS;
}
#endif


extern int
avt_avatar_image_none (void)
{
  avt_set_avatar_image (NULL);

  return _avt_STATUS;
}


extern int
avt_avatar_image_xpm (char **xpm)
{
  SDL_Surface *image;

  image = avt_load_image_xpm (xpm);

  if (!image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_xbm (const unsigned char *bits,
		      int width, int height, const char *colorname)
{
  SDL_Surface *image;
  int red, green, blue;

  if (width <= 0 || height <= 0
      || avt_name_to_color (colorname, &red, &green, &blue) < 0)
    {
      SDL_SetError ("invalid parameters");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  image = avt_load_image_xbm (bits, width, height, red, green, blue);

  if (!image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_data (void *img, size_t imgsize)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  RW = SDL_RWFromMem (img, imgsize);

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (RW, 0);

  if (!image)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (!image)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  SDL_RWclose (RW);

  if (!image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_file (const char *file)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  RW = SDL_RWFromFile (file, "rb");

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (RW, 0);

  if (!image)
    image = avt_load_image_xbm_RW (RW, 0, XBM_DEFAULT_COLOR);

  if (!image)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  SDL_RWclose (RW);

  if (!image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_set_avatar_name (const wchar_t * name)
{
  int size;

  /* clear old name */
  if (avt_name)
    {
      SDL_free (avt_name);
      avt_name = NULL;
    }

  /* copy name */
  if (name && *name)
    {
      size = (avt_strwidth (name) + 1) * sizeof (wchar_t);
      avt_name = (wchar_t *) SDL_malloc (size);
      SDL_memcpy (avt_name, name, size);
    }

  if (avt_visible)
    avt_show_avatar ();

  return _avt_STATUS;
}

extern int
avt_set_avatar_name_mb (const char *name)
{
  wchar_t *wcname;

  if (name == NULL || *name == '\0')
    avt_set_avatar_name (NULL);
  else
    {
      avt_mb_decode (&wcname, name, SDL_strlen (name) + 1);

      if (wcname)
	{
	  avt_set_avatar_name (wcname);
	  SDL_free (wcname);
	}
    }

  return _avt_STATUS;
}

extern void
avt_set_text_background_ballooncolor (void)
{
  if (avt_character)
    {
      SDL_SetColors (avt_character, &ballooncolor_RGB, 0, 1);

      text_background_color = SDL_MapRGB (screen->format,
					  ballooncolor_RGB.r,
					  ballooncolor_RGB.g,
					  ballooncolor_RGB.b);
    }
}

/* can and should be called before avt_initialize */
extern void
avt_set_balloon_color (int red, int green, int blue)
{
  ballooncolor_RGB.r = red;
  ballooncolor_RGB.g = green;
  ballooncolor_RGB.b = blue;

  if (screen)
    {
      avt_set_text_background_ballooncolor ();

      /* redraw the balloon, if it is visible */
      if (textfield.x >= 0)
	avt_draw_balloon ();
    }
}


/* can and should be called before avt_initialize */
extern void
avt_set_balloon_color_name (const char *name)
{
  int red, green, blue;

  if (avt_name_to_color (name, &red, &green, &blue) == 0)
    avt_set_balloon_color (red, green, blue);
}

/* can and should be called before avt_initialize */
extern void
avt_set_background_color (int red, int green, int blue)
{
  backgroundcolor_RGB.r = red;
  backgroundcolor_RGB.g = green;
  backgroundcolor_RGB.b = blue;

  if (screen)
    {
      background_color = SDL_MapRGB (screen->format, red, green, blue);

      if (textfield.x >= 0)
	{
	  avt_visible = false;	/* force to redraw everything */
	  avt_draw_balloon ();
	}
      else if (avt_visible)
	avt_show_avatar ();
      else
	avt_clear_screen ();
    }
}

/* can and should be called before avt_initialize */
extern void
avt_set_background_color_name (const char *name)
{
  int red, green, blue;

  if (avt_name_to_color (name, &red, &green, &blue) == 0)
    avt_set_background_color (red, green, blue);
}

extern void
avt_get_background_color (int *red, int *green, int *blue)
{
  if (red && green && blue)
    {
      *red = backgroundcolor_RGB.r;
      *green = backgroundcolor_RGB.g;
      *blue = backgroundcolor_RGB.b;
    }
}

extern void
avt_reserve_single_keys (bool onoff)
{
  reserve_single_keys = AVT_MAKE_BOOL (onoff);
}

extern void
avt_register_keyhandler (avt_keyhandler handler)
{
  avt_ext_keyhandler = handler;
}

extern void
avt_register_mousehandler (avt_mousehandler handler)
{
  avt_ext_mousehandler = handler;
}

extern void
avt_set_mouse_visible (bool visible)
{
  if (visible)
    SDL_ShowCursor (SDL_ENABLE);
  else
    SDL_ShowCursor (SDL_DISABLE);
}

extern void
avt_set_text_color (int red, int green, int blue)
{
  SDL_Color color;

  if (avt_character)
    {
      color.r = red;
      color.g = green;
      color.b = blue;
      SDL_SetColors (avt_character, &color, 1, 1);
    }
}

extern void
avt_set_text_color_name (const char *name)
{
  int red, green, blue;

  if (avt_name_to_color (name, &red, &green, &blue) == 0)
    avt_set_text_color (red, green, blue);
}

extern void
avt_set_text_background_color (int red, int green, int blue)
{
  SDL_Color color;

  if (avt_character)
    {
      color.r = red;
      color.g = green;
      color.b = blue;
      SDL_SetColors (avt_character, &color, 0, 1);

      text_background_color = SDL_MapRGB (screen->format, red, green, blue);
    }
}

extern void
avt_set_text_background_color_name (const char *name)
{
  int red, green, blue;

  if (avt_name_to_color (name, &red, &green, &blue) == 0)
    avt_set_text_background_color (red, green, blue);
}

extern void
avt_inverse (bool onoff)
{
  inverse = AVT_MAKE_BOOL (onoff);
}

extern bool
avt_get_inverse (void)
{
  return inverse;
}

extern void
avt_bold (bool onoff)
{
  bold = AVT_MAKE_BOOL (onoff);
}

extern bool
avt_get_bold (void)
{
  return bold;
}

extern void
avt_underlined (bool onoff)
{
  underlined = AVT_MAKE_BOOL (onoff);
}

extern bool
avt_get_underlined (void)
{
  return underlined;
}

extern void
avt_normal_text (void)
{
  underlined = bold = inverse = markup = false;

  /* set color table for character canvas */
  if (avt_character)
    {
      SDL_Color colors[2];

      /* background -> ballooncolor */
      colors[0].r = ballooncolor_RGB.r;
      colors[0].g = ballooncolor_RGB.g;
      colors[0].b = ballooncolor_RGB.b;
      /* black foreground */
      colors[1].r = colors[1].g = colors[1].b = 0x00;

      SDL_SetColors (avt_character, colors, 0, 2);

      text_background_color = SDL_MapRGB (screen->format,
					  ballooncolor_RGB.r,
					  ballooncolor_RGB.g,
					  ballooncolor_RGB.b);
    }
}

extern void
avt_markup (bool onoff)
{
  markup = AVT_MAKE_BOOL (onoff);
  underlined = bold = false;
}

extern void
avt_set_text_delay (int delay)
{
  text_delay = delay;

  /* eventually switch off updates lock */
  if (text_delay != 0 && hold_updates)
    avt_lock_updates (false);
}

extern void
avt_set_flip_page_delay (int delay)
{
  flip_page_delay = delay;
}

extern void
avt_set_scroll_mode (int mode)
{
  scroll_mode = mode;
}

extern int
avt_get_scroll_mode (void)
{
  return scroll_mode;
}

extern char *
avt_get_error (void)
{
  return SDL_GetError ();
}


#define CREDITDELAY 50

/* scroll one line up */
static void
avt_credits_up (SDL_Surface * last_line)
{
  SDL_Rect src, dst, line_pos;
  Uint32 moved;
  Uint32 now, next_time;
  Uint32 pixel, tickinterval;

  moved = 0;
  pixel = 1;
  tickinterval = CREDITDELAY;
  next_time = SDL_GetTicks () + tickinterval;

  while (moved <= LINEHEIGHT)
    {
      /* move screen up */
      src.x = window.x;
      src.w = window.w;
      src.y = window.y + pixel;
      src.h = window.h - pixel;
      dst.x = window.x;
      dst.y = window.y;
      SDL_BlitSurface (screen, &src, screen, &dst);

      if (last_line)
	{
	  line_pos.x = window.x;
	  line_pos.y = window.y + window.h - moved;
	  SDL_BlitSurface (last_line, NULL, screen, &line_pos);
	}

      AVT_UPDATE_RECT (window);

      if (avt_checkevent ())
	return;

      moved += pixel;
      now = SDL_GetTicks ();

      if (next_time > now)
	SDL_Delay (next_time - now);
      else
	{
	  /* move more pixels at once and give more time next time */
	  if (pixel < LINEHEIGHT - moved)
	    {
	      pixel++;
	      tickinterval += CREDITDELAY;
	    }
	}

      next_time += tickinterval;
    }
}

extern int
avt_credits (const wchar_t * text, bool centered)
{
  wchar_t line[80];
  SDL_Surface *last_line;
  SDL_Color old_backgroundcolor;
  avt_keyhandler old_keyhandler;
  avt_mousehandler old_mousehandler;
  const wchar_t *p;
  int i;
  int length;

  if (!screen || _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  /* store old background color */
  old_backgroundcolor = backgroundcolor_RGB;

  /* deactivate mous/key- handlers */
  old_keyhandler = avt_ext_keyhandler;
  old_mousehandler = avt_ext_mousehandler;
  avt_ext_keyhandler = NULL;
  avt_ext_mousehandler = NULL;

  /* needed to handle resizing correctly */
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  avt_visible = false;
  hold_updates = false;

  /* the background-color is used when the window is resized */
  /* this implicitly also clears the screen */
  avt_set_background_color (0, 0, 0);
  avt_set_text_background_color (0, 0, 0);
  avt_set_text_color (0xff, 0xff, 0xff);

  window.x = (screen->w / 2) - (80 * FONTWIDTH / 2);
  window.w = 80 * FONTWIDTH;
  /* horizontal values unchanged */

  SDL_SetClipRect (screen, &window);

  /* last line added to credits */
  last_line =
    SDL_CreateRGBSurface (SDL_SWSURFACE | SDL_RLEACCEL,
			  window.w, LINEHEIGHT,
			  screen->format->BitsPerPixel,
			  screen->format->Rmask,
			  screen->format->Gmask,
			  screen->format->Bmask, screen->format->Amask);

  if (!last_line)
    {
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  /* cursor position for last_line */
  cursor.y = 0;

  /* show text */
  p = text;
  while (*p && _avt_STATUS == AVT_NORMAL)
    {
      /* get line */
      length = 0;
      while (*p && *p != L'\n')
	{
	  if (*p >= L' ' && length < 80)
	    {
	      line[length] = *p;
	      length++;
	    }

	  p++;
	}

      /* skip line-end */
      p++;

      /* draw line */
      if (centered)
	cursor.x = (window.w / 2) - (length * FONTWIDTH / 2);
      else
	cursor.x = 0;

      /* clear line */
      SDL_FillRect (last_line, NULL, 0);

      /* print on last_line */
      for (i = 0; i < length; i++, cursor.x += FONTWIDTH)
	avt_drawchar ((avt_char) line[i], last_line);

      avt_credits_up (last_line);
    }

  /* show one empty line to avoid streakes */
  SDL_FillRect (last_line, NULL, 0);
  avt_credits_up (last_line);

  /* scroll up until screen is empty */
  for (i = 0; i < window.h / LINEHEIGHT && _avt_STATUS == AVT_NORMAL; i++)
    avt_credits_up (NULL);

  SDL_FreeSurface (last_line);

  window.w = MINIMALWIDTH;
  window.h = MINIMALHEIGHT;
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  /* back to normal (also sets variables!) */
  avt_set_background_color (old_backgroundcolor.r, old_backgroundcolor.g,
			    old_backgroundcolor.b);
  avt_normal_text ();
  avt_clear_screen ();

  avt_ext_keyhandler = old_keyhandler;
  avt_ext_mousehandler = old_mousehandler;

  return _avt_STATUS;
}

extern int
avt_credits_mb (const char *txt, bool centered)
{
  wchar_t *wctext;

  if (screen && _avt_STATUS == AVT_NORMAL)
    {
      avt_mb_decode (&wctext, txt, SDL_strlen (txt) + 1);

      if (wctext)
	{
	  avt_credits (wctext, centered);
	  SDL_free (wctext);
	}
    }

  return _avt_STATUS;
}

extern void
avt_quit (void)
{
  if (avt_quit_audio_func)
    {
      (*avt_quit_audio_func) ();
      avt_quit_audio_func = NULL;
    }

  load_image_done ();

  /* close conversion descriptors */
  if (output_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (output_cd);
  if (input_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (input_cd);
  output_cd = input_cd = ICONV_UNINITIALIZED;

  avt_encoding[0] = '\0';

  if (screen)
    {
      SDL_FreeCursor (mpointer);
      mpointer = NULL;
      SDL_FreeSurface (circle);
      circle = NULL;
      SDL_FreeSurface (pointer);
      pointer = NULL;
      SDL_FreeSurface (avt_character);
      avt_character = NULL;
      SDL_FreeSurface (avatar_image);
      avatar_image = NULL;
      SDL_FreeSurface (avt_text_cursor);
      avt_text_cursor = NULL;
      SDL_FreeSurface (avt_cursor_character);
      avt_cursor_character = NULL;
      avt_alert_func = NULL;
      SDL_Quit ();
      screen = NULL;		/* it was freed by SDL_Quit */
      avt_visible = false;
      textfield.x = textfield.y = textfield.w = textfield.h = -1;
      viewport = textfield;
    }
}

extern void
avt_button_quit (void)
{
  avt_wait_button ();
  avt_move_out ();
  avt_quit ();
}

#ifdef OLD_SDL

/* old SDL could only handle ASCII titles */

extern void
avt_set_title (const char *title, const char *shortname)
{
  SDL_WM_SetCaption (title, shortname);
}

#else /* not OLD_SDL */

extern void
avt_set_title (const char *title, const char *shortname)
{
  /* check if it's already in correct encoding default="UTF-8" */
  if (SDL_strcasecmp ("UTF-8", avt_encoding) == 0
      || SDL_strcasecmp ("UTF8", avt_encoding) == 0
      || SDL_strcasecmp ("CP65001", avt_encoding) == 0)
    SDL_WM_SetCaption (title, shortname);
  else				/* convert them to UTF-8 */
    {
      char my_title[260];
      char my_shortname[84];

      if (title && *title)
	{
	  if (avt_recode_buffer ("UTF-8", avt_encoding,
				 my_title, sizeof (my_title),
				 title, SDL_strlen (title)) == (size_t) (-1))
	    {
	      SDL_memcpy (my_title, title, sizeof (my_title));
	      my_title[sizeof (my_title) - 1] = '\0';
	    }
	}

      if (shortname && *shortname)
	{
	  if (avt_recode_buffer ("UTF-8", avt_encoding,
				 my_shortname, sizeof (my_shortname),
				 shortname,
				 SDL_strlen (shortname)) == (size_t) (-1))
	    {
	      SDL_memcpy (my_shortname, shortname, sizeof (my_shortname));
	      my_shortname[sizeof (my_shortname) - 1] = '\0';
	    }
	}

      SDL_WM_SetCaption (my_title, my_shortname);
    }
}

#endif /* not OLD_SDL */


#define reverse_byte(b) \
  (((b) & 0x80) >> 7 | \
   ((b) & 0x40) >> 5 | \
   ((b) & 0x20) >> 3 | \
   ((b) & 0x10) >> 1 | \
   ((b) & 0x08) << 1 | \
   ((b) & 0x04) << 3 | \
   ((b) & 0x02) << 5 | \
   ((b) & 0x01) << 7)

/* width must be a multiple of 8 */
#define xbm_bytes(img)  ((img##_width / 8) * img##_height)

static void
avt_set_mouse_pointer (void)
{
  unsigned char mp[xbm_bytes (mpointer)];
  unsigned char mp_mask[xbm_bytes (mpointer_mask)];
  int i;

  /* we need the bytes reversed :-( */

  for (i = 0; i < xbm_bytes (mpointer); i++)
    {
      register unsigned char b = mpointer_bits[i];
      mp[i] = reverse_byte (b);
    }

  for (i = 0; i < xbm_bytes (mpointer_mask); i++)
    {
      register unsigned char b = mpointer_mask_bits[i];
      mp_mask[i] = reverse_byte (b);
    }

  mpointer = SDL_CreateCursor (mp, mp_mask,
			       mpointer_width, mpointer_height,
			       mpointer_x_hot, mpointer_y_hot);

  SDL_SetCursor (mpointer);
}

extern int
avt_start (const char *title, const char *shortname, int mode)
{
  SDL_Surface *icon;

  /* already initialized? */
  if (screen)
    {
      SDL_SetError ("AKFAvatar already initialized");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  avt_mode = mode;
  _avt_STATUS = AVT_NORMAL;
  reserve_single_keys = false;
  newline_mode = true;
  auto_margin = true;
  origin_mode = true;		/* for backwards compatibility */
  avt_visible = false;
  markup = false;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;

  if (avt_init_SDL ())
    {
      SDL_SetError ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (title == NULL)
    title = "AKFAvatar";

  if (shortname == NULL)
    shortname = title;

  avt_set_title (title, shortname);

  /* register icon */
  icon = avt_load_image_xpm (akfavatar_xpm);
  SDL_WM_SetIcon (icon, NULL);
  SDL_FreeSurface (icon);

  /*
   * Initialize the display, accept any format
   */
  screenflags = SDL_SWSURFACE | SDL_ANYFORMAT;

#ifndef __WIN32__
  if (avt_mode == AVT_AUTOMODE)
    {
      SDL_Rect **modes;

      /*
       * if maximum fullscreen mode is exactly the minimal size,
       * then default to fullscreen, else default to window
       */
      modes = SDL_ListModes (NULL, screenflags | SDL_FULLSCREEN);
      if (modes != (SDL_Rect **) (0) && modes != (SDL_Rect **) (-1))
	if (modes[0]->w == MINIMALWIDTH && modes[0]->h == MINIMALHEIGHT)
	  screenflags |= SDL_FULLSCREEN;
    }
#endif

  SDL_ClearError ();

  if (avt_mode >= 1)
    screenflags |= SDL_FULLSCREEN;

  if (avt_mode == AVT_FULLSCREENNOSWITCH)
    {
      screen = SDL_SetVideoMode (0, 0, COLORDEPTH, screenflags);

      /* fallback if 0,0 is not supported yet (before SDL-1.2.10) */
      if (screen && (screen->w == 0 || screen->h == 0))
	screen =
	  SDL_SetVideoMode (MINIMALWIDTH, MINIMALHEIGHT, COLORDEPTH,
			    screenflags);
    }
  else
    screen =
      SDL_SetVideoMode (MINIMALWIDTH, MINIMALHEIGHT, COLORDEPTH, screenflags);

  if (screen == NULL)
    {
      SDL_SetError ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (screen->w < MINIMALWIDTH || screen->h < MINIMALHEIGHT)
    {
      SDL_SetError ("screen too small");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  /* size of the window (not to be confused with the variable window */
  windowmode_size.x = windowmode_size.y = 0;	/* unused */
  windowmode_size.w = screen->w;
  windowmode_size.h = screen->h;

  /* window may be smaller than the screen */
  window.w = MINIMALWIDTH;
  window.h = MINIMALHEIGHT;
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  avt_set_mouse_pointer ();

  background_color = SDL_MapRGB (screen->format,
				 backgroundcolor_RGB.r,
				 backgroundcolor_RGB.g,
				 backgroundcolor_RGB.b);

  /* fill the whole screen with background color */
  avt_clear_screen ();

  /* reserve memory for one character */
  avt_character = SDL_CreateRGBSurface (SDL_SWSURFACE, FONTWIDTH, FONTHEIGHT,
					1, 0, 0, 0, 0);

  if (!avt_character)
    {
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  avt_normal_text ();

  /* prepare text-mode cursor */
  avt_text_cursor =
    SDL_CreateRGBSurface (SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL,
			  FONTWIDTH, FONTHEIGHT, 8, 0, 0, 0, 128);

  if (!avt_text_cursor)
    {
      SDL_FreeSurface (avt_character);
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  /* set color table for character canvas */
  SDL_FillRect (avt_text_cursor, NULL, 0);
  SDL_SetColors (avt_text_cursor, &cursor_color, 0, 1);
  SDL_SetAlpha (avt_text_cursor, SDL_SRCALPHA | SDL_RLEACCEL, 128);

  /* set actual balloon size to the maximum size */
  calculate_balloonmaxheight ();
  balloonheight = balloonmaxheight;
  balloonwidth = AVT_LINELENGTH;

  /* reserve space for character under text-mode cursor */
  avt_cursor_character =
    SDL_CreateRGBSurface (SDL_SWSURFACE, FONTWIDTH, FONTHEIGHT,
			  screen->format->BitsPerPixel,
			  screen->format->Rmask, screen->format->Gmask,
			  screen->format->Bmask, screen->format->Amask);

  if (!avt_cursor_character)
    {
      SDL_FreeSurface (avt_text_cursor);
      SDL_FreeSurface (avt_character);
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  circle =
    avt_load_image_xbm (AVT_XBM_INFO (circle), ballooncolor_RGB.r,
			ballooncolor_RGB.g, ballooncolor_RGB.b);
  pointer =
    avt_load_image_xbm (AVT_XBM_INFO (balloonpointer), ballooncolor_RGB.r,
			ballooncolor_RGB.g, ballooncolor_RGB.b);

  /* needed to get the character of the typed key */
  SDL_EnableUNICODE (1);

  /* key repeat mode */
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  /* ignore what we don't use */
  SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState (SDL_KEYUP, SDL_IGNORE);

  /* visual flash for the alert */
  /* when you initialize the audio stuff, you get an audio alert */
  avt_alert_func = avt_flash;

  /* initialize tab stops */
  avt_reset_tab_stops ();

  return _avt_STATUS;
}


#ifndef DISABLE_DEPRECATED
extern int
avt_initialize (const char *title, const char *shortname,
		avt_image_t * image, int mode)
{
  avt_start (title, shortname, mode);
  avt_set_avatar_image (image);

  if (image)
    SDL_FreeSurface (image);

  return _avt_STATUS;
}
#endif
