/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007, 2008, 2009, 2010 Andreas K. Foerster <info@akfoerster.de>
 *
 * needed:
 *  SDL1.2 (recommended: SDL1.2.11 or later (but not 1.3!))
 * recommended:
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
#define _AVT_NO_DEPRECATED 1

#include "akfavatar.h"
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

#ifdef LINK_SDL_IMAGE
#  include "SDL_image.h"
#endif

#define COPYRIGHTYEAR "2010"
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

#define SHADOWOFFSET 7

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

#  ifndef FORCE_ICONV
#    define FORCE_ICONV
#  endif

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
#  undef SDL_isspace
#  define SDL_isspace             isspace
#  undef SDL_putenv
#  define SDL_putenv              putenv
#  undef SDL_sscanf
#  define SDL_sscanf              sscanf
#endif /* OLD_SDL */

#ifdef FORCE_ICONV
#  include <iconv.h>
#  include <errno.h>
#  define AVT_ICONV_ERROR         (size_t)(-1)
#  define AVT_ICONV_E2BIG         (size_t)(-2)
#  define AVT_ICONV_EILSEQ        (size_t)(-3)
#  define AVT_ICONV_EINVAL        (size_t)(-4)
#  define avt_iconv_t             iconv_t
#  define avt_iconv_open          iconv_open
#  define avt_iconv_close         iconv_close
   /* avt_iconv implemented below */
#else
#  define AVT_ICONV_ERROR         SDL_ICONV_ERROR
#  define AVT_ICONV_E2BIG         SDL_ICONV_E2BIG
#  define AVT_ICONV_EILSEQ        SDL_ICONV_EILSEQ
#  define AVT_ICONV_EINVAL        SDL_ICONV_EINVAL
#  define avt_iconv_t             SDL_iconv_t
#  define avt_iconv_open          SDL_iconv_open
#  define avt_iconv_close         SDL_iconv_close
#  define avt_iconv               SDL_iconv
#endif /* OLD_SDL */

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

/* moving target - grrrmpf! */
/* but compiler warnings about this can be savely ignored */
#if ((SDL_COMPILEDVERSION) >= 1212)
#  define AVT_ICONV_INBUF_T const char
#else
#  define AVT_ICONV_INBUF_T char
#endif

/* try to guess WCHAR_ENCODING,
 * based on WCHAR_MAX or __WCHAR_MAX__ if it is available
 * note: newer SDL versions include stdint.h if available
 */
#if !defined(WCHAR_MAX) && defined(__WCHAR_MAX__)
#  define WCHAR_MAX __WCHAR_MAX__
#endif

#ifndef WCHAR_ENCODING
#  ifdef WCHAR_MAX
#    if (WCHAR_MAX <= 65535U)
#      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#        define WCHAR_ENCODING "UTF-16BE"
#      else /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#        define WCHAR_ENCODING "UTF-16LE"
#      endif /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#    else /* (WCHAR_MAX > 65535U) */
#      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#        define WCHAR_ENCODING "UTF-32BE"
#      else /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#        define WCHAR_ENCODING "UTF-32LE"
#      endif /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#    endif /* (WCHAR_MAX > 65535U) */
#  else	/* not WCHAR_MAX */
#   error "please define WCHAR_ENCODING (no autodetection possible)"
#  endif /* not WCHAR_MAX */
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

/* type for gimp images */
typedef struct
{
  unsigned int width;
  unsigned int height;
  unsigned int bytes_per_pixel;	/* 3:RGB, 4:RGBA */
  unsigned char pixel_data;	/* handle as startpoint */
} gimp_img_t;

/* for an external keyboard/mouse handlers */
static avt_keyhandler avt_ext_keyhandler = NULL;
static avt_mousehandler avt_ext_mousehandler = NULL;

static SDL_Surface *screen, *avt_image, *avt_character;
static SDL_Surface *avt_text_cursor, *avt_cursor_character;
static SDL_Surface *circle, *pointer;
static SDL_Cursor *mpointer;
static wchar_t *avt_name;
static Uint32 background_color;
static Uint32 text_background_color;
static avt_bool_t newline_mode;	/* when off, you need an extra CR */
static avt_bool_t underlined, bold, inverse;	/* text underlined, bold? */
static avt_bool_t auto_margin;	/* automatic new lines? */
static Uint32 screenflags;	/* flags for the screen */
static int avt_mode;		/* whether fullscreen or window or ... */
static SDL_Rect window;		/* if screen is in fact larger */
static SDL_Rect windowmode_size;	/* size of the whole window (screen) */
static avt_bool_t avt_visible;	/* avatar visible? */
static avt_bool_t text_cursor_visible;	/* shall the text cursor be visible? */
static avt_bool_t text_cursor_actually_visible;	/* is it actually visible? */
static avt_bool_t reserve_single_keys;	/* reserve single keys? */
static int scroll_mode = 1;
static SDL_Rect textfield;
static SDL_Rect viewport;	/* sub-window in textfield */
static avt_bool_t avt_tab_stops[AVT_LINELENGTH];

/* origin mode */
/* Home: textfield (AVT_FALSE) or viewport (AVT_TRUE) */
/* avt_initialize sets it to AVT_TRUE for backwards compatibility */
static avt_bool_t origin_mode;
static int textdir_rtl = AVT_LEFT_TO_RIGHT;

/* beginning of line - depending on text direction */
static int linestart;
static int balloonheight, balloonmaxheight, balloonwidth;

/* delay values for printing text and flipping the page */
static int text_delay = 0;	/* AVT_DEFAULT_TEXT_DELAY */
static int flip_page_delay = AVT_DEFAULT_FLIP_PAGE_DELAY;

/* holding updates back? */
static avt_bool_t hold_updates;

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
static void avt_drawchar (wchar_t ch, SDL_Surface * surface);


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
  while (SDL_isspace (*name))
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
  avt_bool_t initialized;
  void *handle;			/* handle for dynamically loaded SDL_image */
  SDL_Surface *(*file) (const char *file);
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
  int code_nr;

  codes = NULL;
  colors = NULL;
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
  /* for <= 256 a table is in img */
  if (ncolors > 256)
    {
      colors = (Uint32 *) SDL_calloc (ncolors, sizeof (Uint32));
      if (!colors)
	{
	  SDL_SetError ("out of memory");
	  SDL_free (img);
	  img = NULL;
	  goto done;
	}
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
      while (*p && (*p != 'c' || !SDL_isspace (*(p + 1))
		    || !SDL_isspace (*(p - 1))))
	p++;

      /* no color definition found? search for grayscale definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p && (*p != 'g' || !SDL_isspace (*(p + 1))
			|| !SDL_isspace (*(p - 1))))
	    p++;
	}

      /* no grayscale definition found? search for g4 definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p
		 && (*p != '4' || *(p - 1) != 'g' || !SDL_isspace (*(p + 1))
		     || !SDL_isspace (*(p - 2))))
	    p++;
	}

      /* search for monochrome definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p && (*p != 'm' || !SDL_isspace (*(p + 1))
			|| !SDL_isspace (*(p - 1))))
	    p++;
	}

      if (*p)
	{
	  int red, green, blue;
	  int color_name_pos;
	  char color_name[80];

	  /* skip to color name/definition */
	  p++;
	  while (*p && SDL_isspace (*p))
	    p++;

	  /* copy colorname up to next space */
	  color_name_pos = 0;
	  while (*p && !SDL_isspace (*p)
		 && color_name_pos < (int) sizeof (color_name) - 1)
	    color_name[color_name_pos++] = *p++;
	  color_name[color_name_pos] = '\0';

	  if (SDL_strcasecmp (color_name, "None") == 0)
	    {
	      SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL, code_nr);
	    }
	  else if (avt_name_to_color (color_name, &red, &green, &blue) == 0)
	    {
	      if (ncolors <= 256)
		{
		  SDL_Color color;
		  color.r = red;
		  color.g = green;
		  color.b = blue;

		  SDL_SetColors (img, &color, code_nr, 1);
		}
	      else		/* ncolors > 256 */
		{
		  *(colors + colornr - 1) =
		    SDL_MapRGB (img->format, red, green, blue);
		}
	    }
	}
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
  avt_bool_t end, error;

  if (!src)
    return NULL;

  img = NULL;
  xpm = NULL;
  line = NULL;
  end = error = AVT_FALSE;

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
      error = end = AVT_TRUE;
    }

  while (!end)
    {
      /* skip to next quote */
      do
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    end = AVT_TRUE;
	}
      while (!end && c != '"');

      /* read line */
      linepos = 0;
      c = '\0';
      while (!end && c != '"')
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = AVT_TRUE;	/* shouldn't happen here */

	  if (c != '"')
	    line[linepos++] = c;

	  if (linepos >= linecapacity)
	    {
	      linecapacity += 100;
	      line = (char *) SDL_realloc (line, linecapacity);
	      if (!line)
		error = end = AVT_TRUE;
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
		error = end = AVT_TRUE;
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
  SDL_Color color;
  int y;
  int bpl;			/* Bytes per line */
  Uint8 *line;

  color.r = red;
  color.g = green;
  color.b = blue;

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

  SDL_SetColorKey (img, SDL_SRCCOLORKEY, 0);
  SDL_SetColors (img, &color, 1, 1);

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
  avt_bool_t end, error;
  avt_bool_t X10;

  if (!src)
    return NULL;

  img = NULL;
  bits = NULL;
  X10 = AVT_FALSE;
  end = error = AVT_FALSE;
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
      error = end = AVT_TRUE;

    p = SDL_strstr (line, "_height ");
    if (p)
      height = SDL_atoi (p + 8);
    else
      error = end = AVT_TRUE;

    if (SDL_strstr (line, " short ") != NULL)
      X10 = AVT_TRUE;
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
      error = end = AVT_TRUE;
    }

  /* search start of bitmap part */
  if (!end && !error)
    {
      char c;

      SDL_RWseek (src, start, RW_SEEK_SET);

      do
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = AVT_TRUE;
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
	    error = end = AVT_TRUE;

	  if (c != '\n' && c != '}')
	    line[linepos++] = c;

	  if (c == '}')
	    end = AVT_TRUE;
	}
      line[linepos] = '\0';

      /* parse line */
      if (line[0] != '\0')
	{
	  char *p;
	  char *endptr;
	  long value;
	  avt_bool_t end_of_line;

	  p = line;
	  end_of_line = AVT_FALSE;
	  while (!end_of_line && bmpos < bytes)
	    {
	      value = SDL_strtol (p, &endptr, 0);
	      if (endptr == p)
		end_of_line = AVT_TRUE;
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
      load_image.file = IMG_Load;
      load_image.rw = IMG_Load_RW;

      load_image.initialized = AVT_TRUE;
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


static SDL_Surface *
avt_load_image_file (const char *filename)
{
  return avt_load_image_RW (SDL_RWFromFile (filename, "rb"), 1);
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
      load_image.file = avt_load_image_file;
      load_image.rw = avt_load_image_RW;

#ifndef NO_SDL_IMAGE
/* loadso.h is only available with SDL 1.2.6 or higher */
#ifdef _SDL_loadso_h
      load_image.handle = SDL_LoadObject (AVT_SDL_IMAGE_LIB);
      if (load_image.handle)
	{
	  load_image.file =
	    (SDL_Surface * (*)(const char *))
	    SDL_LoadFunction (load_image.handle, "IMG_Load");

	  load_image.rw =
	    (SDL_Surface * (*)(SDL_RWops *, int))
	    SDL_LoadFunction (load_image.handle, "IMG_Load_RW");
	}
#endif /* _SDL_loadso_h */
#endif /* NO_SDL_IMAGE */

      load_image.initialized = AVT_TRUE;
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
      load_image.file = NULL;
      load_image.rw = NULL;
      load_image.initialized = AVT_FALSE;	/* try again next time */
    }
}
#endif /* _SDL_loadso_h */

#endif /* ! LINK_SDL_IMAGE */

#ifdef FORCE_ICONV
static size_t
avt_iconv (avt_iconv_t cd,
	   AVT_ICONV_INBUF_T ** inbuf, size_t * inbytesleft,
	   char **outbuf, size_t * outbytesleft)
{
  size_t r;

  r = iconv (cd, inbuf, inbytesleft, outbuf, outbytesleft);

  if (r == (size_t) (-1))
    {
      switch (errno)
	{
	case E2BIG:
	  return AVT_ICONV_E2BIG;
	case EILSEQ:
	  return AVT_ICONV_EILSEQ;
	case EINVAL:
	  return AVT_ICONV_EINVAL;
	default:
	  return AVT_ICONV_ERROR;
	}
    }

  return r;
}
#endif /* FORCE_ICONV */


extern const char *
avt_version (void)
{
  return AVTVERSION;
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

extern avt_bool_t
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
avt_show_text_cursor (avt_bool_t on)
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
avt_activate_cursor (avt_bool_t on)
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
  avt_visible = AVT_FALSE;
}

#define NAME_PADDING 3

static void
avt_show_name (void)
{
  SDL_Rect dst;
  SDL_Color old_colors[2], colors[2];
  wchar_t *p;

  if (screen && avt_image && avt_name)
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

      dst.x = window.x + AVATAR_MARGIN + avt_image->w + BUTTON_DISTANCE;
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
	  avt_drawchar (*p++, screen);
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

      if (avt_image)
	{
	  /* left */
	  dst.x = window.x + AVATAR_MARGIN;
	  /* bottom */
	  dst.y = window.y + window.h - avt_image->h - AVATAR_MARGIN;
	  dst.w = avt_image->w;
	  dst.h = avt_image->h;
	  SDL_BlitSurface (avt_image, NULL, screen, &dst);
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
      avt_visible = AVT_TRUE;
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
  if (avt_image)
    {
      SDL_Rect pointer_shape, pointer_pos;

      pointer_shape.x = pointer_shape.y = 0;
      pointer_shape.w = pointer->w;
      pointer_shape.h = pointer->h;

      /* if the balloonpointer is too large, cut it */
      if (pointer_shape.h > (avt_image->h / 2))
	{
	  pointer_shape.y = pointer_shape.h - (avt_image->h / 2);
	  pointer_shape.h -= pointer_shape.y;
	}

      pointer_pos.x =
	window.x + avt_image->w + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET
	+ offset;
      pointer_pos.y = window.y + (balloonmaxheight * LINEHEIGHT)
	+ (2 * BALLOON_INNER_MARGIN) + TOPMARGIN + offset;

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

  if (avt_image)
    textfield.y = window.y + ((balloonmaxheight - balloonheight) * LINEHEIGHT)
      + TOPMARGIN + BALLOON_INNER_MARGIN;
  else				/* middle of the window */
    textfield.y = window.y + (window.h / 2) - (textfield.h / 2);

  /* centered as default */
  textfield.x = window.x + (window.w / 2) - (balloonwidth * FONTWIDTH / 2);

  /* align with balloonpointer */
  if (avt_image)
    {
      /* left border not aligned with balloon pointer? */
      if (textfield.x >
	  window.x + avt_image->w + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET)
	textfield.x =
	  window.x + avt_image->w + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET;

      /* right border not aligned with balloon pointer? */
      if (textfield.x + textfield.w <
	  window.x + avt_image->w + pointer->w
	  + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET)
	{
	  textfield.x =
	    window.x + avt_image->w - textfield.w + pointer->w
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

  avt_visible = AVT_TRUE;

  /* cursor at top  */
  cursor.x = linestart;
  cursor.y = viewport.y;

  /* reset saved position */
  saved_position = cursor;

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
	avt_show_text_cursor (AVT_FALSE);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      linestart = (textdir_rtl) ? area.x + area.w - FONTWIDTH : area.x;
      cursor.x = linestart;

      if (text_cursor_visible)
	avt_show_text_cursor (AVT_TRUE);
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
	  avt_visible = AVT_FALSE;	/* force to redraw everything */
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
	  avt_visible = AVT_FALSE;	/* force to redraw everything */
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
	  avt_visible = AVT_FALSE;	/* force to redraw everything */
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_bell (void)
{
  if (avt_alert_func)
    (*avt_alert_func) ();
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

static void
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
  avt_bool_t pause;
  avt_bool_t audio_initialized;

  audio_initialized = (SDL_WasInit (SDL_INIT_AUDIO) != 0);
  pause = AVT_TRUE;

  if (audio_initialized)
    SDL_PauseAudio (pause);

  do
    {
      if (SDL_WaitEvent (&event))
	{
	  if (event.type == SDL_KEYDOWN)
	    pause = AVT_FALSE;
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

extern int
avt_wait (int milliseconds)
{
  Uint32 endtime;

  if (screen)
    {
      endtime = SDL_GetTicks () + milliseconds;

      /* loop while time is not reached yet, and there is no event */
      while ((SDL_GetTicks () < endtime) && !avt_checkevent ())
	SDL_Delay (1);		/* give some time to other apps */
    }

  return _avt_STATUS;
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

extern avt_bool_t
avt_home_position (void)
{
  if (!screen || textfield.x < 0)
    return AVT_TRUE;		/* about to be set to home position */
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
	avt_show_text_cursor (AVT_FALSE);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      cursor.x = (x - 1) * FONTWIDTH + area.x;

      /* max-pos exeeded? */
      if (cursor.x > area.x + area.w - FONTWIDTH)
	cursor.x = area.x + area.w - FONTWIDTH;

      if (text_cursor_visible)
	avt_show_text_cursor (AVT_TRUE);
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
	avt_show_text_cursor (AVT_FALSE);

      if (origin_mode)
	area = viewport;
      else
	area = textfield;

      cursor.y = (y - 1) * LINEHEIGHT + area.y;

      /* max-pos exeeded? */
      if (cursor.y > area.y + area.h - LINEHEIGHT)
	cursor.y = area.y + area.h - LINEHEIGHT;

      if (text_cursor_visible)
	avt_show_text_cursor (AVT_TRUE);
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
	avt_show_text_cursor (AVT_FALSE);

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
	avt_show_text_cursor (AVT_TRUE);
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
    avt_show_text_cursor (AVT_FALSE);

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
    avt_show_text_cursor (AVT_TRUE);

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
    avt_show_text_cursor (AVT_FALSE);

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
    avt_show_text_cursor (AVT_TRUE);

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
    avt_show_text_cursor (AVT_FALSE);

  clear.x = cursor.x;
  clear.y = cursor.y;
  clear.w = num * FONTWIDTH;
  clear.h = LINEHEIGHT;
  SDL_FillRect (screen, &clear, text_background_color);

  if (text_cursor_visible)
    avt_show_text_cursor (AVT_TRUE);

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
    avt_show_text_cursor (AVT_FALSE);

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
    avt_show_text_cursor (AVT_TRUE);

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
    avt_show_text_cursor (AVT_FALSE);

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
    avt_show_text_cursor (AVT_TRUE);

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
    avt_show_text_cursor (AVT_FALSE);

  viewport.x = textfield.x + ((x - 1) * FONTWIDTH);
  viewport.y = textfield.y + ((y - 1) * LINEHEIGHT);
  viewport.w = width * FONTWIDTH;
  viewport.h = height * LINEHEIGHT;

  linestart =
    (textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;

  cursor.x = linestart;
  cursor.y = viewport.y;

  if (text_cursor_visible)
    avt_show_text_cursor (AVT_TRUE);

  if (origin_mode)
    SDL_SetClipRect (screen, &viewport);
  else
    SDL_SetClipRect (screen, &textfield);
}

extern void
avt_newline_mode (avt_bool_t mode)
{
  newline_mode = mode;
}

extern void
avt_auto_margin (avt_bool_t mode)
{
  auto_margin = mode;
}

extern void
avt_set_origin_mode (avt_bool_t mode)
{
  SDL_Rect area;

  origin_mode = AVT_MAKE_BOOL (mode);

  if (text_cursor_visible && textfield.x >= 0)
    avt_show_text_cursor (AVT_FALSE);

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
    avt_show_text_cursor (AVT_TRUE);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &area);
}

extern avt_bool_t
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
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
    avt_show_text_cursor (AVT_FALSE);

  dst.x = viewport.x;
  dst.w = viewport.w;
  dst.y = cursor.y;
  dst.h = viewport.h - (cursor.y - viewport.y);

  SDL_FillRect (screen, &dst, text_background_color);

  if (text_cursor_visible)
    {
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
      text_cursor_actually_visible = AVT_FALSE;
      avt_show_text_cursor (AVT_TRUE);
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
    avt_show_text_cursor (AVT_FALSE);

  cursor.x = linestart;

  if (text_cursor_visible)
    avt_show_text_cursor (AVT_TRUE);
}

extern int
avt_new_line (void)
{
  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return _avt_STATUS;

  if (text_cursor_visible)
    avt_show_text_cursor (AVT_FALSE);

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
    avt_show_text_cursor (AVT_TRUE);

  return _avt_STATUS;
}

/* avt_drawchar: draws the raw char - with no interpretation */
#if (FONTWIDTH > 8)
static void
avt_drawchar (wchar_t ch, SDL_Surface * surface)
{
  extern const unsigned short *get_font_char (wchar_t ch);
  const unsigned short *font_line;
  unsigned int y;
  SDL_Rect dest;
  unsigned short *pixels, *p;
  Uint16 pitch;

  pitch = avt_character->pitch / sizeof (*p);
  pixels = p = (unsigned short *) avt_character->pixels;
  font_line = get_font_char (ch);

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
avt_drawchar (wchar_t ch, SDL_Surface * surface)
{
  extern const unsigned char *get_font_char (wchar_t ch);
  const unsigned char *font_line;
  int y;
  SDL_Rect dest;
  Uint8 *pixels, *p;
  Uint16 pitch;

  pitch = avt_character->pitch;
  pixels = p = (Uint8 *) avt_character->pixels;
  font_line = get_font_char (ch);

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

/* make current char visible */
static void
avt_showchar (void)
{
  if (!hold_updates)
    {
      SDL_UpdateRect (screen, cursor.x, cursor.y, FONTWIDTH, FONTHEIGHT);
      text_cursor_actually_visible = AVT_FALSE;
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
    avt_show_text_cursor (AVT_TRUE);

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
	avt_show_text_cursor (AVT_TRUE);
    }
}

extern void
avt_reset_tab_stops (void)
{
  int i;

  for (i = 0; i < AVT_LINELENGTH; i++)
    if (i % 8 == 0)
      avt_tab_stops[i] = AVT_TRUE;
    else
      avt_tab_stops[i] = AVT_FALSE;
}

extern void
avt_clear_tab_stops (void)
{
  SDL_memset (&avt_tab_stops, AVT_FALSE, sizeof (avt_tab_stops));
}

extern void
avt_set_tab (int x, avt_bool_t onoff)
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
	    avt_show_text_cursor (AVT_FALSE);

	  cursor.x =
	    (textdir_rtl) ? cursor.x + FONTWIDTH : cursor.x - FONTWIDTH;
	}

      if (text_cursor_visible)
	avt_show_text_cursor (AVT_TRUE);
    }
}

/*
 * writes a character to the textfield -
 * interprets control characters
 */
extern int
avt_put_character (const wchar_t ch)
{
  if (!screen)
    return _avt_STATUS;

  /* nothing to do, when ch == L'\0' */
  if (ch == L'\0')
    return avt_checkevent ();

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  switch (ch)
    {
    case L'\n':
    case L'\v':
      avt_new_line ();
      break;

    case L'\r':
      avt_carriage_return ();
      break;

    case L'\f':
      avt_flip_page ();
      break;

    case L'\t':
      avt_next_tab ();
      break;

    case L'\b':
      avt_backspace ();
      break;

    case L'\a':
      if (avt_alert_func)
	(*avt_alert_func) ();
      break;

      /* ignore BOM here
       * must be handled outside of the library
       */
    case L'\xFEFF':
      break;

      /* LRM/RLM: only supported at the beginning of a line */
    case L'\x200E':		/* LEFT-TO-RIGHT MARK (LRM) */
      avt_text_direction (AVT_LEFT_TO_RIGHT);
      break;

    case L'\x200F':		/* RIGHT-TO-LEFT MARK (RLM) */
      avt_text_direction (AVT_RIGHT_TO_LEFT);
      break;

      /* other ignorable (invisible) characters */
    case L'\x200B':
    case L'\x200C':
    case L'\x200D':
    case L'\x00AD':
    case L'\x2060':
    case L'\x2061':
    case L'\x2062':
    case L'\x2063':
      break;

    case L' ':			/* space */
      if (auto_margin)
	check_auto_margin ();
      if (!underlined && !inverse)
	avt_clearchar ();
      else			/* underlined or inverse */
	{
	  avt_drawchar (' ', screen);
	  avt_showchar ();
	}
      avt_forward ();
      /*
       * no delay for the space char
       * it'd be annoying if you have a sequence of spaces
       */
      break;

    default:
      if (ch > 32)
	{
	  if (auto_margin)
	    check_auto_margin ();
	  avt_drawchar (ch, screen);
	  avt_showchar ();
	  if (text_delay)
	    avt_wait (text_delay);
	  else
	    avt_checkevent ();
	  avt_forward ();
	}			/* if (ch > 32) */
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
	  underlined = AVT_TRUE;
	  if (avt_put_character (*(txt + 2)))
	    r = -1;
	  underlined = AVT_FALSE;
	}
      else if (*txt == *(txt + 2))
	{
	  bold = AVT_TRUE;
	  if (avt_put_character (*txt))
	    r = -1;
	  bold = AVT_FALSE;
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
  if (!screen)
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
	  if (avt_put_character (*txt))
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
avt_say_len (const wchar_t * txt, const int len)
{
  int i;

  /* nothing to do, when txt == NULL */
  /* but do allow a text to start with zeros here */
  if (!screen || !txt)
    return avt_checkevent ();

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  for (i = 0; i < len; i++, txt++)
    {
      if (*(txt + 1) == L'\b')
	{
	  if (avt_overstrike (txt))
	    break;
	  txt += 2;
	  i += 2;
	}
      else
	{
	  if (avt_put_character (*txt))
	    break;
	}
    }

  return _avt_STATUS;
}

extern int
avt_mb_encoding (const char *encoding)
{
  /* output */

  /*  if it is already open, close it first */
  if (output_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (output_cd);

  /* initialize the conversion framework */
  output_cd = avt_iconv_open (WCHAR_ENCODING, encoding);

  /* check if it was successfully initialized */
  if (output_cd == ICONV_UNINITIALIZED)
    {
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("encoding %s not supported for output", encoding);
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
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("encoding %s not supported for input", encoding);
      return _avt_STATUS;
    }

  return _avt_STATUS;
}

/* size in bytes */
/* dest must be freed by caller */
extern int
avt_mb_decode (wchar_t ** dest, const char *src, const int size)
{
  static char rest_buffer[10];
  static size_t rest_bytes = 0;
  char *inbuf_start, *outbuf;
  AVT_ICONV_INBUF_T *inbuf;
  size_t dest_size;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  /* check if size is useful */
  if (size <= 0)
    {
      *dest = NULL;
      return -1;
    }

  /* check if encoding was set */
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = size + rest_bytes;
  inbuf_start = (char *) SDL_malloc (inbytesleft);

  if (!inbuf_start)
    {
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("out of memory");
      *dest = NULL;
      return -1;
    }

  inbuf = inbuf_start;

  /* if there is a rest from last call, put it into the buffer */
  if (rest_bytes > 0)
    SDL_memcpy ((void *) inbuf, &rest_buffer, rest_bytes);

  /* copy the text into the buffer */
  SDL_memcpy ((void *) (inbuf + rest_bytes), src, size);
  rest_bytes = 0;

  /* get enough space */
  /* +1 for the terminator */
  dest_size = (inbytesleft + 1) * sizeof (wchar_t);

  /* minimal string size */
  if (dest_size < 8)
    dest_size = 8;

  *dest = (wchar_t *) SDL_malloc (dest_size);

  if (!*dest)
    {
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("out of memory");
      SDL_free (inbuf_start);
      return -1;
    }

  outbuf = (char *) *dest;
  outbytesleft = dest_size;

  /* do the conversion */
  returncode =
    avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  /* handle invalid characters */
  while (returncode == AVT_ICONV_EILSEQ)
    {
      inbuf++;
      inbytesleft--;

      *((wchar_t *) outbuf) = L'\xFFFD';

      outbuf += sizeof (wchar_t);
      outbytesleft -= sizeof (wchar_t);
      returncode =
	avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  /* check for fatal errors */
  if (returncode == AVT_ICONV_ERROR || returncode == AVT_ICONV_E2BIG)
    {
      SDL_free (*dest);
      *dest = NULL;
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("error while converting the encoding");
      return -1;
    }

  /* check for incomplete sequences and put them into the rest_buffer */
  if (returncode == AVT_ICONV_EINVAL && inbytesleft <= sizeof (rest_buffer))
    {
      rest_bytes = inbytesleft;
      SDL_memcpy ((void *) &rest_buffer, inbuf, rest_bytes);
    }

  /* free the inbuf */
  SDL_free (inbuf_start);

  /* terminate outbuf */
  if (outbytesleft >= sizeof (wchar_t))
    *((wchar_t *) outbuf) = L'\0';

  return ((dest_size - outbytesleft) / sizeof (wchar_t));
}

extern int
avt_mb_encode (char **dest, const wchar_t * src, const int len)
{
  char *inbuf_start, *outbuf;
  AVT_ICONV_INBUF_T *inbuf;
  size_t dest_size;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  /* check if len is useful */
  if (len <= 0)
    {
      *dest = NULL;
      return -1;
    }

  /* check if encoding was set */
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = len * sizeof (wchar_t);
  inbuf_start = (char *) SDL_malloc (inbytesleft);

  if (!inbuf_start)
    {
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("out of memory");
      *dest = NULL;
      return -1;
    }

  inbuf = inbuf_start;

  /* copy the text into the buffer */
  SDL_memcpy ((void *) inbuf, src, inbytesleft);

  /* get enough space */
  /* UTF-8 may need 6 bytes per character */
  /* +1 for the terminator */
  dest_size = len * 6 + 1;
  *dest = (char *) SDL_malloc (dest_size);

  if (!*dest)
    {
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("out of memory");
      SDL_free (inbuf_start);
      return -1;
    }

  outbuf = *dest;
  outbytesleft = dest_size;

  /* do the conversion */
  returncode =
    avt_iconv (input_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  /* check for fatal errors */
  if (returncode == AVT_ICONV_ERROR || returncode == AVT_ICONV_E2BIG)
    {
      SDL_free (*dest);
      *dest = NULL;
      _avt_STATUS = AVT_ERROR;
      SDL_SetError ("error while converting the encoding");
      return -1;
    }

  /* free the inbuf */
  SDL_free (inbuf_start);

  /* terminate outbuf */
  if (outbytesleft >= sizeof (char))
    *outbuf = '\0';

  return (dest_size - outbytesleft);
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
  wchar_t *wctext;

  if (screen)
    {
      avt_mb_decode (&wctext, txt, SDL_strlen (txt) + 1);

      if (wctext)
	{
	  avt_say (wctext);
	  SDL_free (wctext);
	}
    }

  return _avt_STATUS;
}

extern int
avt_say_mb_len (const char *txt, int len)
{
  wchar_t *wctext;
  int wclen;

  if (screen)
    {
      wclen = avt_mb_decode (&wctext, txt, len);

      if (wctext)
	{
	  avt_say_len (wctext, wclen);
	  SDL_free (wctext);
	}
    }

  return _avt_STATUS;
}

extern int
avt_get_key (wchar_t * ch)
{
  SDL_Event event;

  if (screen)
    {
      *ch = L'\0';
      while ((*ch == L'\0') && (_avt_STATUS == AVT_NORMAL))
	{
	  SDL_WaitEvent (&event);
	  avt_analyze_event (&event);

	  if (event.type == SDL_KEYDOWN)
	    *ch = (wchar_t) event.key.keysym.unicode;
	}
    }

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
	    avt_bool_t back, avt_bool_t forward)
{
  SDL_Surface *plain_menu, *bar;
  SDL_Event event;
  int last_key;
  int end_line;
  int line_nr, old_line;

  if (screen)
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
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      /* set color for bar and make it transparent */
      SDL_FillRect (bar, NULL, 0);
      SDL_SetColors (bar, &cursor_color, 0, 1);
      SDL_SetAlpha (bar, SDL_SRCALPHA | SDL_RLEACCEL, 128);

      SDL_EventState (SDL_MOUSEMOTION, SDL_ENABLE);
      SDL_ShowCursor (SDL_ENABLE);	/* mouse cursor */

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

      SDL_ShowCursor (SDL_DISABLE);
      SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
      SDL_FreeSurface (plain_menu);
      SDL_FreeSurface (bar);
    }

  return _avt_STATUS;
}

/* deprecated - just for backward compatibility */
extern int
avt_menu (wchar_t * ch, int menu_start, int menu_end, wchar_t start_code,
	  avt_bool_t back, avt_bool_t forward)
{
  int status, result;

  status = avt_choice (&result, menu_start, menu_end - menu_start + 1,
		       (int) start_code, back, forward);

  *ch = result + start_code - 1;
  return status;
}

/* deprecated - just for backward compatibility */
extern int
avt_get_menu (wchar_t * ch, int menu_start, int menu_end, wchar_t start_code)
{
  int status, result;

  status = avt_choice (&result, menu_start, menu_end - menu_start + 1,
		       (int) start_code, AVT_FALSE, AVT_FALSE);

  *ch = result + start_code - 1;
  return status;
}

extern void
avt_lock_updates (avt_bool_t lock)
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

static int
avt_pager_line (const wchar_t * txt, int pos, int len)
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

static int
avt_pager_screen (const wchar_t * txt, int pos, int len)
{
  int line_nr;

  hold_updates = AVT_TRUE;
  SDL_FillRect (screen, &textfield, text_background_color);

  for (line_nr = 0; line_nr < balloonheight; line_nr++)
    {
      cursor.x = linestart;
      cursor.y = line_nr * LINEHEIGHT + textfield.y;
      pos = avt_pager_line (txt, pos, len);
    }

  hold_updates = AVT_FALSE;
  AVT_UPDATE_TRECT (textfield);

  return pos;
}

static int
avt_pager_lines_back (const wchar_t * txt, int pos, int lines)
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
avt_pager (const wchar_t * txt, int len, int startline)
{
  int pos;
  avt_bool_t old_auto_margin, old_reserve_single_keys, old_tc;
  avt_bool_t quit;
  avt_keyhandler old_keyhandler;
  avt_mousehandler old_mousehandler;
  void (*old_alert_func) (void);
  SDL_Event event;
  SDL_Surface *button;
  SDL_Rect btn_rect;

  if (!screen)
    return AVT_ERROR;

  /* do we actually have something to show? */
  if (!txt || !*txt)
    return _avt_STATUS;

  /* show mouse pointer */
  SDL_ShowCursor (SDL_ENABLE);

  /* get len if not given */
  if (len <= 0)
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
  text_cursor_visible = AVT_FALSE;
  old_auto_margin = auto_margin;
  auto_margin = AVT_FALSE;
  old_reserve_single_keys = reserve_single_keys;
  reserve_single_keys = AVT_FALSE;
  old_keyhandler = avt_ext_keyhandler;
  avt_ext_keyhandler = NULL;
  old_mousehandler = avt_ext_mousehandler;
  avt_ext_mousehandler = NULL;

  /* temporarily disable the alert function */
  old_alert_func = avt_alert_func;
  avt_alert_func = NULL;

  avt_set_text_delay (0);
  avt_normal_text ();

  /* show first screen */
  pos = avt_pager_screen (txt, pos, len);

  /* last screen */
  if (pos >= len)
    {
      pos = avt_pager_lines_back (txt, len, balloonheight + 1);
      pos = avt_pager_screen (txt, pos, len);
    }

  quit = AVT_FALSE;

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
	      quit = AVT_TRUE;
	      break;
	    }
	  else if (event.button.button <= 3
		   && event.button.y >= btn_rect.y
		   && event.button.y <= btn_rect.y + btn_rect.h
		   && event.button.x >= btn_rect.x
		   && event.button.x <= btn_rect.x + btn_rect.w)
	    {
	      quit = AVT_TRUE;
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
		  hold_updates = AVT_TRUE;
		  avt_delete_lines (1, 1);
		  cursor.x = linestart;
		  cursor.y = (balloonheight - 1) * LINEHEIGHT + textfield.y;
		  pos = avt_pager_line (txt, pos, len);
		  hold_updates = AVT_FALSE;
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
		  hold_updates = AVT_TRUE;
		  avt_insert_lines (1, 1);
		  cursor.x = linestart;
		  cursor.y = textfield.y;
		  avt_pager_line (txt, start_pos, len);
		  hold_updates = AVT_FALSE;
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
	    quit = AVT_TRUE;	/* Q with any combination-key quits */
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

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

  avt_alert_func = old_alert_func;

  return _avt_STATUS;
}

/*
 * Note: in the beginning I only had avt_pager_mb
 * and converted it line by line to save memory.
 * But that broke with UTF-16 and UTF-32.
 */
extern int
avt_pager_mb (const char *txt, int len, int startline)
{
  wchar_t *wctext;
  int wclen;

  if (screen && txt)
    {
      if (len <= 0)
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
avt_ask (wchar_t * s, const int size)
{
  wchar_t ch;
  size_t len, maxlen;

  if (!screen)
    return _avt_STATUS;

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  /* if the cursor is beyond the end of the viewport,
   * get a new page
   */
  if (cursor.y > viewport.y + viewport.h - LINEHEIGHT)
    avt_flip_page ();

  /* maxlen is the rest of line minus one for the cursor */
  if (textdir_rtl)
    maxlen = ((cursor.x - viewport.x) / FONTWIDTH) - 1;
  else
    maxlen = (((viewport.x + viewport.w) - cursor.x) / FONTWIDTH) - 1;

  /* does it fit in the buffer size? */
  if (maxlen > size / sizeof (wchar_t))
    maxlen = size / sizeof (wchar_t);

  len = 0;
  s[len] = L'\0';
  ch = L'\0';

  do
    {
      /* show cursor */
      avt_show_text_cursor (AVT_TRUE);

      if (avt_get_key (&ch))
        break;

      switch (ch)
	{
	case 8:
	case 127:
	  if (len > 0)
	    {
	      len--;
	      s[len] = L'\0';

	      /* delete cursor and one char */
	      avt_show_text_cursor (AVT_FALSE);
	      avt_backspace ();
	      avt_clearchar ();
	    }
	  else if (avt_alert_func)
	    (*avt_alert_func) ();
	  break;

	case 13:
	  break;

	default:
	  if ((ch >= 32) && (len < maxlen))
	    {
	      /* delete cursor */
	      avt_show_text_cursor (AVT_FALSE);
	      s[len] = ch;
	      len++;
	      s[len] = L'\0';
	      avt_drawchar (ch, screen);
	      avt_showchar ();
	      cursor.x =
		(textdir_rtl) ? cursor.x - FONTWIDTH : cursor.x + FONTWIDTH;
	    }
	  else if (avt_alert_func)
	    (*avt_alert_func) ();
	}
    }
  while ((ch != 13) && (_avt_STATUS == AVT_NORMAL));

  /* delete cursor */
  avt_show_text_cursor (AVT_FALSE);

  if (!newline_mode)
    avt_carriage_return ();
  avt_new_line ();

  return _avt_STATUS;
}

extern int
avt_ask_mb (char *s, const int size)
{
  wchar_t ws[AVT_LINELENGTH + 1];
  AVT_ICONV_INBUF_T *inbuf;
  size_t inbytesleft, outbytesleft;

  if (!screen)
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
  if (!screen)
    return _avt_STATUS;

  /* fill the screen with background color */
  /* (not only the window!) */
  avt_clear_screen ();

  /* undefine textfield */
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;

  if (avt_image)
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
      dst.y = mywindow.y + mywindow.h - avt_image->h - AVATAR_MARGIN;
      dst.w = avt_image->w;
      dst.h = avt_image->h;
      start_time = SDL_GetTicks ();

      while (dst.x > mywindow.x + AVATAR_MARGIN)
	{
	  Sint16 oldx = dst.x;

	  /* move */
	  dst.x = screen->w - ((SDL_GetTicks () - start_time) / MOVE_DELAY);

	  if (dst.x != oldx)
	    {
	      /* draw */
	      SDL_BlitSurface (avt_image, NULL, screen, &dst);

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
  if (!screen || !avt_visible)
    return _avt_STATUS;

  /* needed to remove the balloon */
  avt_show_avatar ();

  /*
   * remove clipping
   */
  SDL_SetClipRect (screen, NULL);

  if (avt_image)
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
      dst.y = mywindow.y + mywindow.h - avt_image->h - AVATAR_MARGIN;
      dst.w = avt_image->w;
      dst.h = avt_image->h;
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
	      SDL_BlitSurface (avt_image, NULL, screen, &dst);

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
  avt_bool_t nokey;

  if (!screen)
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

  /* show mouse pointer */
  SDL_ShowCursor (SDL_ENABLE);

  nokey = AVT_TRUE;
  while (nokey)
    {
      SDL_WaitEvent (&event);
      switch (event.type)
	{
	case SDL_QUIT:
	  nokey = AVT_FALSE;
	  _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_KEYDOWN:
	  nokey = AVT_FALSE;
	  if (event.key.keysym.sym == SDLK_ESCAPE)
	    _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  /* ignore the wheel */
	  if (event.button.button <= 3)
	    nokey = AVT_FALSE;
	  break;
	}

      /* do other stuff */
      avt_analyze_event (&event);
    }

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

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
  int i, button_count, button_pos;
  int result;

  result = AVT_ERROR;		/* no result */

  if (!screen)
    return AVT_ERROR;

  button_count = SDL_strlen (buttons);

  if (!buttons || !*buttons || button_count > NAV_MAX)
    {
      SDL_SetError ("No or too many buttons for navigation bar");
      return AVT_ERROR;
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
      return AVT_ERROR;
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

	case 'f':
	  avt_nav_inlay (btn_fastforward);
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

	case 's':
	  avt_nav_inlay (btn_stop);
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

  /* show mouse pointer */
  SDL_ShowCursor (SDL_ENABLE);

  while (result < 0 && _avt_STATUS == AVT_NORMAL)
    {
      SDL_WaitEvent (&event);

      switch (event.type)
	{
	case SDL_KEYDOWN:
	  {
	    int r = AVT_ERROR;

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

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

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

/* deprecated: use avt_wait_button */
extern int
avt_wait_key (const wchar_t * message)
{
  SDL_Event event;
  avt_bool_t nokey;
  SDL_Rect dst;
  struct pos oldcursor;

  if (!screen)
    return _avt_STATUS;

  /* print message (outside of textfield!) */
  if (*message)
    {
      const wchar_t *m;
      SDL_Color colors[2], old_colors[2];

      SDL_SetClipRect (screen, &window);

      old_colors[0] = avt_character->format->palette->colors[0];
      old_colors[1] = avt_character->format->palette->colors[1];

      /* background-color */
      colors[0] = backgroundcolor_RGB;
      /* black foreground */
      colors[1].r = colors[1].g = colors[1].b = 0x00;
      SDL_SetColors (avt_character, colors, 0, 2);

      /* alignment: right with one letter space to the border */
      dst.x =
	window.x + window.w - (avt_strwidth (message) * FONTWIDTH) -
	FONTWIDTH;
      dst.y = window.y + window.h - AVATAR_MARGIN - LINEHEIGHT;
      dst.w = window.x + window.w - dst.x;
      dst.h = FONTHEIGHT;

      oldcursor = cursor;
      cursor.x = dst.x;
      cursor.y = dst.y;

      /* message is also needed later in this function */
      m = message;
      while (*m)
	{
	  avt_drawchar (*m, screen);
	  cursor.x += FONTWIDTH;
	  if (cursor.x > window.x + window.w + FONTWIDTH)
	    break;
	  m++;
	}

      AVT_UPDATE_RECT (dst);
      SDL_SetColors (avt_character, old_colors, 0, 2);
      cursor = oldcursor;
    }

  /* show mouse pointer */
  SDL_WarpMouse (dst.x, dst.y + FONTHEIGHT);
  SDL_ShowCursor (SDL_ENABLE);

  nokey = AVT_TRUE;
  while (nokey)
    {
      SDL_WaitEvent (&event);
      switch (event.type)
	{
	case SDL_QUIT:
	  nokey = AVT_FALSE;
	  _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_KEYDOWN:
	  nokey = AVT_FALSE;
	  if (event.key.keysym.sym == SDLK_ESCAPE)
	    _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  /* ignore the wheel */
	  if (event.button.button <= 3)
	    nokey = AVT_FALSE;
	  break;
	}

      avt_analyze_event (&event);
    }

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

  /* clear message */
  if (*message)
    {
      SDL_SetClipRect (screen, &window);
      SDL_FillRect (screen, &dst, background_color);
      AVT_UPDATE_RECT (dst);
    }

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return _avt_STATUS;
}

/* deprecated: use avt_wait_button */
extern int
avt_wait_key_mb (char *message)
{
  wchar_t *wcmessage;

  if (!screen)
    return _avt_STATUS;

  avt_mb_decode (&wcmessage, message, SDL_strlen (message) + 1);

  if (wcmessage)
    {
      avt_wait_key (wcmessage);
      SDL_free (wcmessage);
    }

  return _avt_STATUS;
}

extern avt_bool_t
avt_decide (void)
{
  SDL_Event event;
  SDL_Surface *base_button;
  SDL_Rect yes_rect, no_rect;
  int result;

  if (!screen)
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

  /* show mouse pointer */
  SDL_ShowCursor (SDL_ENABLE);

  result = -1;			/* no result */
  while (result < 0)
    {
      SDL_WaitEvent (&event);
      switch (event.type)
	{
	case SDL_QUIT:
	  result = AVT_FALSE;
	  _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_KEYDOWN:
	  if (event.key.keysym.sym == SDLK_ESCAPE)
	    {
	      result = AVT_FALSE;
	      _avt_STATUS = AVT_QUIT;
	    }
	  else if (event.key.keysym.unicode == L'-'
		   || event.key.keysym.unicode == L'0'
		   || event.key.keysym.sym == SDLK_BACKSPACE)
	    result = AVT_FALSE;
	  else if (event.key.keysym.unicode == L'+'
		   || event.key.keysym.unicode == L'1'
		   || event.key.keysym.unicode == L'\r')
	    result = AVT_TRUE;
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
		result = AVT_TRUE;
	      else
		if (event.button.x >= no_rect.x
		    && event.button.x <= no_rect.x + no_rect.w)
		result = AVT_FALSE;
	    }
	  break;
	}

      avt_analyze_event (&event);
    }

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

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

/* free avt_image_t images */
extern void
avt_free_image (avt_image_t * image)
{
  SDL_FreeSurface ((SDL_Surface *) image);
}

static void
avt_show_image (avt_image_t * image)
{
  SDL_Rect dst;
  SDL_Surface *img = (SDL_Surface *) image;

  /* clear the screen */
  avt_free_screen ();

  /* set informational variables */
  avt_visible = AVT_FALSE;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;

  /* center image on screen */
  dst.x = (screen->w / 2) - (img->w / 2);
  dst.y = (screen->h / 2) - (img->h / 2);
  dst.w = img->w;
  dst.h = img->h;

  /* if image is larger than the window,
   * just the upper left part is shown, as far as it fits
   */
  SDL_BlitSurface (img, NULL, screen, &dst);
  AVT_UPDATE_ALL ();
  SDL_SetClipRect (screen, &window);
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

  if (!screen)
    return _avt_STATUS;

  /* try internal XPM reader first */
  /* it's better than in SDL_image */
  image = avt_load_image_xpm_RW (SDL_RWFromFile (filename, "rb"), 1);

  if (image == NULL)
    image = avt_load_image_xbm_RW (SDL_RWFromFile (filename, "rb"), 1,
				   XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.file (filename);
    }

  if (image == NULL)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

extern int
avt_show_image_stream (avt_stream * stream)
{
  SDL_Surface *image;

  if (!screen)
    return _avt_STATUS;

  /* try internal XPM reader first */
  /* it's better than in SDL_image */
  image = avt_load_image_xpm_RW (SDL_RWFromFP ((FILE *) stream, 0), 1);

  if (image == NULL)
    image = avt_load_image_xbm_RW (SDL_RWFromFP ((FILE *) stream, 0), 1,
				   XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (SDL_RWFromFP ((FILE *) stream, 0), 1);
    }

  if (image == NULL)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

/*
 * show image from image data
 */
extern int
avt_show_image_data (void *img, int imgsize)
{
  SDL_Surface *image;

  if (!screen)
    return _avt_STATUS;

  /* try internal XPM reader first */
  /* it's better than in SDL_image */
  image = avt_load_image_xpm_RW (SDL_RWFromMem (img, imgsize), 1);

  if (image == NULL)
    image = avt_load_image_xbm_RW (SDL_RWFromMem (img, imgsize), 1,
				   XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (SDL_RWFromMem (img, imgsize), 1);
    }

  if (image == NULL)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

extern int
avt_show_image_xpm (char **xpm)
{
  avt_image_t *image = NULL;

  if (!screen)
    return _avt_STATUS;

  image = (avt_image_t *) avt_load_image_xpm (xpm);

  if (image == NULL)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  avt_show_image (image);
  SDL_FreeSurface ((SDL_Surface *) image);

  return _avt_STATUS;
}

/* deprecated - use avt_show_image_xpm */
extern int
avt_show_image_XPM (char **xpm)
{
  return avt_show_image_xpm (xpm);
}

extern int
avt_show_image_xbm (const unsigned char *bits, int width, int height,
		    const char *colorname)
{
  avt_image_t *image;
  int red, green, blue;

  if (!screen)
    return _avt_STATUS;

  if (width <= 0 || height <= 0)
    return AVT_ERROR;

  if (avt_name_to_color (colorname, &red, &green, &blue) < 0)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  image = (avt_image_t *) avt_load_image_xbm (bits, width, height,
					      red, green, blue);

  if (image == NULL)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  avt_show_image (image);
  SDL_FreeSurface ((SDL_Surface *) image);

  return _avt_STATUS;
}

/*
 * show gimp image
 */
extern int
avt_show_gimp_image (void *gimp_image)
{
  SDL_Surface *image;
  gimp_img_t *img;

  if (!screen)
    return _avt_STATUS;

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

  if (image == NULL)
    return AVT_ERROR;

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
extern avt_image_t *
avt_make_transparent (avt_image_t * image)
{
  Uint32 color;
  SDL_Surface *img = (SDL_Surface *) image;

  if (SDL_MUSTLOCK (img))
    SDL_LockSurface (img);

  /* get color of upper left corner */
  color = getpixel (img, 0, 0);

  if (SDL_MUSTLOCK (img))
    SDL_UnlockSurface (img);

  if (!SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL, color))
    img = NULL;

  return (avt_image_t *) img;
}

extern avt_image_t *
avt_import_xpm (char **xpm)
{
  if (avt_init_SDL ())
    return NULL;

  return (avt_image_t *) avt_load_image_xpm (xpm);
}

/* deprecated - use avt_import_xpm */
extern avt_image_t *
avt_import_XPM (char **xpm)
{
  return avt_import_xpm (xpm);
}

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

  return (avt_image_t *) avt_load_image_xbm (bits, width, height,
					     red, green, blue);
}

/*
 * import RGB gimp_image as avatar
 * pixel in the upper left corner is supposed to be the background color
 */
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

  return (avt_image_t *) image;
}

/*
 * import avatar from image data
 */
extern avt_image_t *
avt_import_image_data (void *img, int imgsize)
{
  SDL_Surface *image;

  if (avt_init_SDL ())
    return NULL;

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (SDL_RWFromMem (img, imgsize), 1);

  if (image == NULL)
    image = avt_load_image_xbm_RW (SDL_RWFromMem (img, imgsize), 1,
				   XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (SDL_RWFromMem (img, imgsize), 1);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  return (avt_image_t *) image;
}

/*
 * import avatar from file
 */
extern avt_image_t *
avt_import_image_file (const char *filename)
{
  SDL_Surface *image;

  if (avt_init_SDL ())
    return NULL;

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (SDL_RWFromFile (filename, "rb"), 1);

  if (image == NULL)
    image = avt_load_image_xbm_RW (SDL_RWFromFile (filename, "rb"), 1,
				   XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.file (filename);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  return (avt_image_t *) image;
}

extern avt_image_t *
avt_import_image_stream (avt_stream * stream)
{
  SDL_Surface *image;

  if (avt_init_SDL ())
    return NULL;

  /* try internal XPM reader first */
  image = avt_load_image_xpm_RW (SDL_RWFromFP ((FILE *) stream, 0), 1);

  if (image == NULL)
    image = avt_load_image_xbm_RW (SDL_RWFromFP ((FILE *) stream, 0), 1,
				   XBM_DEFAULT_COLOR);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (SDL_RWFromFP ((FILE *) stream, 0), 1);

      /* if it's not yet transparent, make it transparent */
      if (image)
	if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  return (avt_image_t *) image;
}

static int
calculate_balloonmaxheight (void)
{
  if (avt_image)
    {
      balloonmaxheight = (window.h - avt_image->h - (2 * TOPMARGIN)
			  - (2 * BALLOON_INNER_MARGIN)
			  - AVATAR_MARGIN) / LINEHEIGHT;

      /* check, whether image is too high */
      /* at least 10 lines */
      if (balloonmaxheight < 10)
	{
	  SDL_SetError ("Avatar image too large");
	  _avt_STATUS = AVT_ERROR;
	  SDL_FreeSurface (avt_image);
	  avt_image = NULL;
	}
    }

  if (!avt_image)		/* no avatar? -> whole screen is the balloon */
    {
      balloonmaxheight = (window.h - (2 * TOPMARGIN)
			  - (2 * BALLOON_INNER_MARGIN)) / LINEHEIGHT;
    }

  return _avt_STATUS;
}

/* change avatar image while running */
extern int
avt_change_avatar_image (avt_image_t * image)
{
  if (avt_visible)
    avt_clear_screen ();

  if (avt_image)
    {
      SDL_FreeSurface (avt_image);
      avt_image = NULL;
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
      if (((SDL_Surface *) image)->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY))
	avt_image = SDL_DisplayFormatAlpha ((SDL_Surface *) image);
      else
	avt_image = SDL_DisplayFormat ((SDL_Surface *) image);

      SDL_FreeSurface ((SDL_Surface *) image);

      if (!avt_image)
	{
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
	  avt_visible = AVT_FALSE;	/* force to redraw everything */
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
avt_reserve_single_keys (avt_bool_t onoff)
{
  reserve_single_keys = AVT_MAKE_BOOL (onoff);
}

/* just for backward compatiblity */
extern void
avt_stop_on_esc (avt_bool_t on)
{
  reserve_single_keys = (on == AVT_FALSE);
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

  /* make mouse visible or invisible */
  if (handler)
    SDL_ShowCursor (SDL_ENABLE);
  else
    SDL_ShowCursor (SDL_DISABLE);
}

extern void
avt_set_mouse_visible (avt_bool_t visible)
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
    }

  text_background_color = SDL_MapRGB (screen->format, red, green, blue);
}

extern void
avt_set_text_background_color_name (const char *name)
{
  int red, green, blue;

  if (avt_name_to_color (name, &red, &green, &blue) == 0)
    avt_set_text_background_color (red, green, blue);
}

extern void
avt_inverse (avt_bool_t onoff)
{
  inverse = AVT_MAKE_BOOL (onoff);
}

extern avt_bool_t
avt_get_inverse (void)
{
  return inverse;
}

extern void
avt_bold (avt_bool_t onoff)
{
  bold = AVT_MAKE_BOOL (onoff);
}

extern avt_bool_t
avt_get_bold (void)
{
  return bold;
}

extern void
avt_underlined (avt_bool_t onoff)
{
  underlined = AVT_MAKE_BOOL (onoff);
}

extern avt_bool_t
avt_get_underlined (void)
{
  return underlined;
}

extern void
avt_normal_text (void)
{
  underlined = bold = inverse = AVT_FALSE;

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

/* deprecated: use avt_set_text_delay, avt_set_flip_page_delay */
extern void
avt_set_delays (int text, int flip_page)
{
  text_delay = text;
  flip_page_delay = flip_page;

  /* eventually switch off updates lock */
  if (text_delay != 0 && hold_updates)
    avt_lock_updates (AVT_FALSE);
}

extern void
avt_set_text_delay (int delay)
{
  text_delay = delay;

  /* eventually switch off updates lock */
  if (text_delay != 0 && hold_updates)
    avt_lock_updates (AVT_FALSE);
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
avt_credits (const wchar_t * text, avt_bool_t centered)
{
  wchar_t line[80];
  SDL_Surface *last_line;
  SDL_Color old_backgroundcolor;
  avt_keyhandler old_keyhandler;
  avt_mousehandler old_mousehandler;
  const wchar_t *p;
  int i;
  int length;

  if (!screen)
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
  avt_visible = AVT_FALSE;
  hold_updates = AVT_FALSE;

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
    return AVT_ERROR;

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
	avt_drawchar (line[i], last_line);

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
avt_credits_mb (const char *txt, avt_bool_t centered)
{
  wchar_t *wctext;

  if (screen)
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
      SDL_FreeSurface (avt_image);
      avt_image = NULL;
      SDL_FreeSurface (avt_text_cursor);
      avt_text_cursor = NULL;
      SDL_FreeSurface (avt_cursor_character);
      avt_cursor_character = NULL;
      avt_alert_func = NULL;
      SDL_Quit ();
      screen = NULL;		/* it was freed by SDL_Quit */
      avt_visible = AVT_FALSE;
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

extern void
avt_set_title (const char *title, const char *icontitle)
{
  SDL_WM_SetCaption (title, icontitle);
}

static void
avt_set_mouse_pointer (void)
{
  Uint8 mpointer_bits[] = {
    0x00, 0x06, 0x00, 0x1a, 0x00, 0x62, 0x01, 0x82, 0x06, 0x0c, 0x18, 0x30,
    0x60, 0xc0, 0x81, 0x00, 0x60, 0xc0, 0x18, 0x30, 0x06, 0x0c, 0x01, 0x82,
    0x00, 0x62, 0x00, 0x1a, 0x00, 0x06, 0x00, 0x00
  };

  Uint8 mpointer_mask_bits[] = {
    0x00, 0x06, 0x00, 0x1e, 0x00, 0x7e, 0x01, 0xfe, 0x07, 0xfc, 0x1f, 0xf0,
    0x7f, 0xc0, 0xff, 0x00, 0x7f, 0xc0, 0x1f, 0xf0, 0x07, 0xfc, 0x01, 0xfe,
    0x00, 0x7e, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x00
  };

  mpointer = SDL_CreateCursor (mpointer_bits, mpointer_mask_bits,
			       16, 16, 0, 7);
  SDL_SetCursor (mpointer);
}

extern int
avt_initialize (const char *title, const char *icontitle,
		avt_image_t * image, int mode)
{
  /* already initialized? */
  if (screen)
    {
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  avt_mode = mode;
  _avt_STATUS = AVT_NORMAL;
  reserve_single_keys = AVT_FALSE;
  newline_mode = AVT_TRUE;
  auto_margin = AVT_TRUE;
  origin_mode = AVT_TRUE;	/* for backwards compatibility */
  avt_visible = AVT_FALSE;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;

  if (avt_init_SDL ())
    {
      avt_free_image (image);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (title == NULL)
    title = "AKFAvatar";

  if (icontitle == NULL)
    icontitle = title;

  SDL_WM_SetCaption (title, icontitle);

  /* register icon */
  {
    SDL_Surface *icon;
    icon = avt_load_image_xpm (akfavatar_xpm);
    SDL_WM_SetIcon (icon, NULL);
    SDL_FreeSurface (icon);
  }

  /*
   * Initialize the display, accept any format
   */
  screenflags = SDL_SWSURFACE | SDL_ANYFORMAT;

#ifndef __WIN32__
  if (avt_mode == AVT_AUTOMODE)
    {
      SDL_Rect **modes;

      /* if maximum fullscreen mode is exactly the minimal size,
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
      avt_free_image (image);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (screen->w < MINIMALWIDTH || screen->h < MINIMALHEIGHT)
    {
      avt_free_image (image);
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

  /*
   * hide mouse pointer
   * (must be after SDL_SetVideoMode)
   */
  SDL_ShowCursor (SDL_DISABLE);

  avt_set_mouse_pointer ();

  /* fill the whole screen with background color */
  avt_clear_screen ();

  /* reserve memory for one character */
  avt_character = SDL_CreateRGBSurface (SDL_SWSURFACE, FONTWIDTH, FONTHEIGHT,
					1, 0, 0, 0, 0);

  if (!avt_character)
    {
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  background_color = SDL_MapRGB (screen->format,
				 backgroundcolor_RGB.r,
				 backgroundcolor_RGB.g,
				 backgroundcolor_RGB.b);
  avt_normal_text ();

  /* prepare text-mode cursor */
  avt_text_cursor =
    SDL_CreateRGBSurface (SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL,
			  FONTWIDTH, FONTHEIGHT, 8, 0, 0, 0, 128);

  if (!avt_text_cursor)
    {
      SDL_FreeSurface (avt_character);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  /* set color table for character canvas */
  SDL_FillRect (avt_text_cursor, NULL, 0);
  SDL_SetColors (avt_text_cursor, &cursor_color, 0, 1);
  SDL_SetAlpha (avt_text_cursor, SDL_SRCALPHA | SDL_RLEACCEL, 128);

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
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  circle =
    avt_load_image_xbm (AVT_XBM_INFO (circle), ballooncolor_RGB.r,
			ballooncolor_RGB.g, ballooncolor_RGB.b);
  pointer =
    avt_load_image_xbm (AVT_XBM_INFO (balloonpointer), ballooncolor_RGB.r,
			ballooncolor_RGB.g, ballooncolor_RGB.b);

  /* import the avatar image */
  if (image)
    {
      /* convert image to display-format for faster drawing */
      if (((SDL_Surface *) image)->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY))
	avt_image = SDL_DisplayFormatAlpha ((SDL_Surface *) image);
      else
	avt_image = SDL_DisplayFormat ((SDL_Surface *) image);

      avt_free_image (image);

      if (!avt_image)
	{
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}
    }

  if (calculate_balloonmaxheight () != AVT_NORMAL)
    return _avt_STATUS;

  balloonheight = balloonmaxheight;
  balloonwidth = AVT_LINELENGTH;

  /* needed to get the character of the typed key */
  SDL_EnableUNICODE (1);

  /* key repeat mode */
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  /* ignore what we don't use */
  SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState (SDL_MOUSEBUTTONUP, SDL_IGNORE);
  SDL_EventState (SDL_KEYUP, SDL_IGNORE);

  /* visual flash for the alert */
  /* when you initialize the audio stuff, you get an audio alert */
  avt_alert_func = avt_flash;

  /* initialize tab stops */
  avt_reset_tab_stops ();

  return _avt_STATUS;
}
