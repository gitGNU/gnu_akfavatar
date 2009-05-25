/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007, 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
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

/* $Id: avatar.c,v 2.227 2009-05-25 16:13:35 akf Exp $ */

#include "akfavatar.h"
#include "SDL.h"
#include "version.h"

#include "akfavatar.xpm"
#include "balloonpointer.xpm"
#include "circle.xpm"
#include "btn-cont.xpm"
#include "btn-yes.xpm"
#include "btn-no.xpm"

#ifdef LINK_SDL_IMAGE
#  include "SDL_image.h"
#endif

#define COPYRIGHTYEAR "2009"
#define BUTTON_DISTANCE 10

/* 
 * avt_wait_key_mb uses avt_wait_key. Both are deprecated.
 * So suppress this warning for this file, if possible.
 */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2)
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

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
#  undef SDL_free
#  define SDL_free                free
#  undef SDL_strlen
#  define SDL_strlen              strlen
#  undef SDL_strdup
#  define SDL_strdup              strdup
#  undef SDL_memcpy
#  define SDL_memcpy              memcpy
#  undef SDL_memset
#  define SDL_memset              memset
#  undef SDL_putenv
#  define SDL_putenv              putenv
#  undef SDL_sscanf
#  define SDL_sscanf              sscanf
#  undef SDL_strncasecmp
#  define SDL_strncasecmp         strncasecmp
#  undef SDL_isspace
#  define SDL_isspace             isspace
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
#pragma GCC poison  malloc free strlen memcpy memset getenv putenv
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

/* color independent from the screen mode */
/* pale brown */
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


/* 0 = normal; 1 = quit-request; -1 = error */
int _avt_STATUS;

void (*avt_bell_func) (void) = NULL;
void (*avt_quit_audio_func) (void) = NULL;

/* forward declaration */
static int avt_pause (void);


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
  SDL_Surface *(*xpm) (char **xpm);
} load_image;

#define AVT_UPDATE_RECT(rect) \
  SDL_UpdateRect(screen, rect.x, rect.y, rect.w, rect.h)

#define AVT_UPDATE_ALL(void) SDL_UpdateRect(screen, 0, 0, 0, 0)

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
      load_image.xpm = IMG_ReadXPMFromArray;

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

/* XPM support */
/* up to 256 colors or grayscales */
/* use this for internal stuff! */
static SDL_Surface *
avt_load_image_xpm (char **xpm)
{
  SDL_Surface *img;
  SDL_Color color;
  char *p;
  unsigned int red, green, blue;
  int width, height, ncolors, cpp;
  int line, colornr;
  Uint16 *palette;
  Sint32 palette_nr;

  palette = NULL;

  /* check if we actually have data to process */
  if (!xpm || !*xpm)
    return NULL;

  if (SDL_sscanf (xpm[0], "%d %d %d %d", &width, &height, &ncolors, &cpp) < 4
      || width < 1 || height < 1 || ncolors < 1)
    {
      SDL_SetError ("error in XPM data");
      return NULL;
    }

  /* only limited colors supported */
  if (ncolors > 256 || cpp > 2)
    {
      SDL_SetError ("too many colors for XPM image");
      return NULL;
    }

  img = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);

  if (!img)
    return NULL;

  if (cpp > 1)
    {
      palette = (Uint16 *) SDL_malloc (ncolors * sizeof (Uint16));
      if (!palette)
	{
	  SDL_SetError ("out of memory");
	  SDL_free (img);
	  return NULL;
	}
    }

  palette_nr = 0;

  /* set colors */
  for (colornr = 1; colornr <= ncolors; colornr++, palette_nr++)
    {
      /* if there is only one character per pixel, 
       * the character is the palette number 
       */
      if (cpp == 1)
	palette_nr = xpm[colornr][0];
      else			/* store characters in palette */
	*(palette + palette_nr) = *(Uint16 *) xpm[colornr];

      /* scan for color definition */
      p = &xpm[colornr][cpp];	/* skip color-characters */
      while (*p && (*p != 'c' || !SDL_isspace (*(p + 1))
		    || !SDL_isspace (*(p - 1))))
	p++;

      /* no color found? search for grayscale definition */
      if (!*p)
	{
	  p = &xpm[colornr][cpp];	/* skip color-characters */
	  while (*p && (*p != 'g' || !SDL_isspace (*(p + 1))
			|| !SDL_isspace (*(p - 1))))
	    p++;
	}

      if (*p)
	{
	  p++;

	  if (SDL_sscanf (p, " #%2x%2x%2x", &red, &green, &blue) == 3)
	    {
	      color.r = red;
	      color.g = green;
	      color.b = blue;
	      SDL_SetColors (img, &color, palette_nr, 1);
	    }
	  else if (SDL_strncasecmp (p, " None", 6) == 0)
	    {
	      SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
			       palette_nr);
	    }
	}
    }

  /* copy pixeldata */
  if (SDL_MUSTLOCK (img))
    SDL_LockSurface (img);
  if (cpp == 1)
    {
      for (line = 0; line < height; line++)
	SDL_memcpy ((Uint8 *) img->pixels + (line * img->pitch),
		    xpm[ncolors + 1 + line], width);
    }
  else				/* cpp != 1 */
    {
      Uint16 code;
      int pos;

      for (line = 0; line < height; line++)
	for (pos = 0; pos < width; pos++)
	  {
	    code = *(Uint16 *) (xpm[ncolors + 1 + line] + (pos * cpp));
	    palette_nr = 0;
	    while (*(palette + palette_nr) != code && palette_nr < ncolors)
	      palette_nr++;
	    *((Uint8 *) img->pixels + (line * img->pitch) + pos) = palette_nr;
	  }
    }
  if (SDL_MUSTLOCK (img))
    SDL_UnlockSurface (img);

  if (palette)
    SDL_free (palette);

  return img;
}

#define XPM_MAX_LINES (1 + 256 + MINIMALHEIGHT)

static SDL_Surface *
avt_load_image_xpm_RW (SDL_RWops * src, int freesrc)
{
  int start;
  char head[9];
  char *xpmdata[XPM_MAX_LINES + 1];
  char line[(MINIMALWIDTH * 2) + 1];
  unsigned int linepos, linenr;
  SDL_Surface *img;
  char c;
  avt_bool_t end;

  img = NULL;
  end = AVT_FALSE;

  if (!src)
    return NULL;

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
	    end = AVT_TRUE;	/* shouldn't happen here */

	  if (linepos < (sizeof (line) - 1) && c != '"')
	    line[linepos++] = c;
	}

      /* copy line */
      if (!end)
	{
	  line[linepos] = '\0';
	  xpmdata[linenr] = SDL_strdup (line);
	  if (linenr >= XPM_MAX_LINES)
	    end = AVT_TRUE;
	  linenr++;
	}
    }

  /* terminate the array */
  xpmdata[linenr] = NULL;

  if (freesrc)
    SDL_RWclose (src);

  if (linenr <= XPM_MAX_LINES)
    img = avt_load_image_xpm (xpmdata);
  else
    SDL_SetError ("XPM image too large");

  /* free xpmdata */
  linenr = 0;
  while (linenr <= XPM_MAX_LINES && xpmdata[linenr] != NULL)
    SDL_free (xpmdata[linenr++]);

  return img;
}

static SDL_Surface *
avt_load_image_RW (SDL_RWops * src, int freesrc)
{
  SDL_Surface *img;

  img = NULL;

  if (src)
    {
      if ((img = SDL_LoadBMP_RW (src, 0)) == NULL)
	img = avt_load_image_xpm_RW (src, 0);

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
 * (uncompressed BMP files can always be loaded)
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
      load_image.xpm = avt_load_image_xpm;

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

	  load_image.xpm =
	    (SDL_Surface * (*)(char **))
	    SDL_LoadFunction (load_image.handle, "IMG_ReadXPMFromArray");
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
      load_image.xpm = NULL;
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


const char *
avt_version (void)
{
  return AVTVERSION;
}

const char *
avt_copyright (void)
{
  return "Copyright (c) " COPYRIGHTYEAR " Andreas K. Foerster";
}

const char *
avt_license (void)
{
  return "License GPLv3+: GNU GPL version 3 or later "
    "<http://gnu.org/licenses/gpl.html>";
}

avt_bool_t
avt_initialized (void)
{
  return (screen != NULL);
}

int
avt_get_status (void)
{
  return _avt_STATUS;
}

void
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

  if (on != text_cursor_actually_visible)
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
	  AVT_UPDATE_RECT (dst);
	}
      else
	{
	  /* restore saved character */
	  SDL_BlitSurface (avt_cursor_character, NULL, screen, &dst);
	  AVT_UPDATE_RECT (dst);
	}

      text_cursor_actually_visible = on;
    }
}

void
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

void
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
    }
}

void
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

  /* horizontal shape */
  {
    SDL_Rect hshape;
    hshape.x = textfield.x - BALLOON_INNER_MARGIN + offset;
    hshape.w = textfield.w + (2 * BALLOON_INNER_MARGIN);
    hshape.y = textfield.y + offset;
    hshape.h = textfield.h;
    SDL_FillRect (screen, &hshape, ballooncolor);
  }

  /* vertical shape */
  {
    SDL_Rect vshape;
    vshape.x = textfield.x + offset;
    vshape.y = textfield.y - BALLOON_INNER_MARGIN + offset;
    vshape.w = textfield.w;
    vshape.h = textfield.h + (2 * BALLOON_INNER_MARGIN);
    SDL_FillRect (screen, &vshape, ballooncolor);
  }

  /* full size */
  shape.x = textfield.x - BALLOON_INNER_MARGIN + offset;
  shape.w = textfield.w + (2 * BALLOON_INNER_MARGIN);
  shape.y = textfield.y - BALLOON_INNER_MARGIN + offset;
  shape.h = textfield.h + (2 * BALLOON_INNER_MARGIN);

  /* draw corners */
  {
    SDL_Rect circle_piece, corner_pos;

    /* prepare circle piece */
    /* the size is always the same */
    circle_piece.w = circle->w / 2;
    circle_piece.h = circle->h / 2;

    /* upper left corner */
    circle_piece.x = 0;
    circle_piece.y = 0;
    corner_pos.x = shape.x;
    corner_pos.y = shape.y;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    /* upper right corner */
    circle_piece.x = circle->w / 2;
    circle_piece.y = 0;
    corner_pos.x = shape.x + shape.w - circle_piece.w;
    corner_pos.y = shape.y;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    /* lower left corner */
    circle_piece.x = 0;
    circle_piece.y = circle->h / 2;
    corner_pos.x = shape.x;
    corner_pos.y = shape.y + shape.h - circle_piece.h;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    /* lower right corner */
    circle_piece.x = circle->w / 2;
    circle_piece.y = circle->h / 2;
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

  textfield.w = (balloonwidth * FONTWIDTH);
  textfield.h = (balloonheight * LINEHEIGHT);

  if (avt_image)
    textfield.y = window.y + ((balloonmaxheight - balloonheight) * LINEHEIGHT)
      + TOPMARGIN + BALLOON_INNER_MARGIN;
  else
    textfield.y = window.y + TOPMARGIN + BALLOON_INNER_MARGIN;

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

    shadow_color.r = backgroundcolor_RGB.r - 0x20;
    shadow_color.g = backgroundcolor_RGB.g - 0x20;
    shadow_color.b = backgroundcolor_RGB.b - 0x20;
    SDL_SetColors (circle, &shadow_color, circle_xpm[2][0], 1);
    SDL_SetColors (pointer, &shadow_color, balloonpointer_xpm[2][0], 1);

    /* first draw shadow */
    avt_draw_balloon2 (SHADOWOFFSET,
		       SDL_MapRGB (screen->format, shadow_color.r,
				   shadow_color.g, shadow_color.b));
  }


  /* real balloon is white */
  {
    SDL_Color balloon_color;

    balloon_color.r = balloon_color.g = balloon_color.b = 0xFF;
    SDL_SetColors (circle, &balloon_color, circle_xpm[2][0], 1);
    SDL_SetColors (pointer, &balloon_color, balloonpointer_xpm[2][0], 1);

    /* real balloon */
    avt_draw_balloon2 (0, SDL_MapRGB (screen->format, 0xFF, 0xFF, 0xFF));
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

void
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

void
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
	  /* also redraw the avatar */
	  avt_visible = AVT_FALSE;
	  avt_draw_balloon ();
	}
    }
}

void
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
	  /* also redraw the avatar */
	  avt_visible = AVT_FALSE;
	  avt_draw_balloon ();
	}
    }
}

void
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
	  /* also redraw the avatar */
	  avt_visible = AVT_FALSE;
	  avt_draw_balloon ();
	}
    }
}

/* rectangles in some functions have to be adjusted */
#define avt_pre_resize(rect) \
  do { rect.x -= window.x; rect.y -= window.y; } while(0)
#define avt_post_resize(rect) \
  do { rect.x += window.x; rect.y += window.y; } while(0)

static void
avt_resize (int w, int h)
{
  SDL_Surface *oldwindowimage;
  SDL_Rect oldwindow;

  if (w < MINIMALWIDTH)
    w = MINIMALWIDTH;
  if (h < MINIMALHEIGHT)
    h = MINIMALHEIGHT;

  /* save the window */
  oldwindow = window;

  oldwindowimage = SDL_CreateRGBSurface (SDL_SWSURFACE, window.w, window.h,
					 screen->format->BitsPerPixel,
					 screen->format->Rmask,
					 screen->format->Gmask,
					 screen->format->Bmask,
					 screen->format->Amask);

  SDL_BlitSurface (screen, &window, oldwindowimage, NULL);

  /* resize screen */
  screen = SDL_SetVideoMode (w, h, COLORDEPTH, screenflags);

  avt_free_screen ();

  /* new position of the window on the screen */
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  /* restore image */
  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (oldwindowimage, NULL, screen, &window);
  SDL_FreeSurface (oldwindowimage);

  /* recalculate textfield & viewport positions */
  if (textfield.x >= 0)
    {
      textfield.x = textfield.x - oldwindow.x + window.x;
      textfield.y = textfield.y - oldwindow.y + window.y;

      viewport.x = viewport.x - oldwindow.x + window.x;
      viewport.y = viewport.y - oldwindow.y + window.y;

      linestart =
	(textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;

      cursor.x = cursor.x - oldwindow.x + window.x;
      cursor.y = cursor.y - oldwindow.y + window.y;
      SDL_SetClipRect (screen, &viewport);
    }

  /* set windowmode_size */
  if ((screenflags & SDL_FULLSCREEN) == 0)
    {
      windowmode_size.w = w;
      windowmode_size.h = h;
    }

  /* make all changes visible */
  AVT_UPDATE_ALL ();
}

void
avt_bell (void)
{
  if (avt_bell_func)
    (*avt_bell_func) ();
}

/* flashes the screen */
void
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
}

void
avt_toggle_fullscreen (void)
{
  if (avt_mode != AVT_FULLSCREENNOSWITCH)
    {
      /* toggle bit for fullscreenmode */
      screenflags ^= SDL_FULLSCREEN;

      if ((screenflags & SDL_FULLSCREEN) != 0)
	{
	  avt_resize (MINIMALWIDTH, MINIMALHEIGHT);
	  avt_mode = AVT_FULLSCREEN;
	}
      else
	{
	  avt_resize (windowmode_size.w, windowmode_size.h);
	  avt_mode = AVT_WINDOW;
	}

      background_color = SDL_MapRGB (screen->format,
				     backgroundcolor_RGB.r,
				     backgroundcolor_RGB.g,
				     backgroundcolor_RGB.b);
    }
}

/* switch to fullscreen or window mode */
void
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
	      avt_resize (MINIMALWIDTH, MINIMALHEIGHT);
	    }
	  break;
	case AVT_WINDOW:
	  if ((screenflags & SDL_FULLSCREEN) != 0)
	    {
	      screenflags &= ~SDL_FULLSCREEN;
	      avt_resize (windowmode_size.w, windowmode_size.h);
	    }
	  break;
	}

      background_color = SDL_MapRGB (screen->format,
				     backgroundcolor_RGB.r,
				     backgroundcolor_RGB.g,
				     backgroundcolor_RGB.b);
    }
}

static void
avt_analyze_event (SDL_Event * event)
{
  switch (event->type)
    {
    case SDL_QUIT:
      _avt_STATUS = AVT_QUIT;
      break;

    case SDL_VIDEORESIZE:
      avt_resize (event->resize.w, event->resize.h);
      break;

    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      if (avt_ext_mousehandler)
	{
	  int x, y;
	  x = (event->button.x - textfield.x) / FONTWIDTH + 1;
	  y = (event->button.y - textfield.y) / LINEHEIGHT + 1;

	  if (x > 0 && x <= AVT_LINELENGTH
	      && y > 0 && y <= (textfield.h / LINEHEIGHT))
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


int
avt_checkevent (void)
{
  SDL_Event event;

  while (SDL_PollEvent (&event))
    avt_analyze_event (&event);

  return _avt_STATUS;
}

/* checks for events and gives some time to other apps */
int
avt_update (void)
{
  if (screen)
    {
      SDL_Delay (1);
      avt_checkevent ();
    }

  return _avt_STATUS;
}

int
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

int
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

int
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

avt_bool_t
avt_home_position (void)
{
  if (!screen || textfield.x < 0)
    return AVT_TRUE;		/* about to be set to home position */
  else
    return (cursor.y == viewport.y && cursor.x == linestart);
}

/* this always means the full textfield */
int
avt_get_max_x (void)
{
  if (screen)
    return balloonwidth;
  else
    return -1;
}

/* this always means the full textfield */
int
avt_get_max_y (void)
{
  if (screen)
    return balloonheight;
  else
    return -1;
}

void
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

void
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

void
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

void
avt_save_position (void)
{
  saved_position = cursor;
}

void
avt_restore_position (void)
{
  cursor = saved_position;
}

void
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
  SDL_UpdateRect (screen, viewport.x, cursor.y, viewport.w, FONTHEIGHT);
}

void
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
  SDL_UpdateRect (screen, viewport.x, cursor.y, viewport.w, FONTHEIGHT);
}

void
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
  AVT_UPDATE_RECT (clear);
}

void
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

  AVT_UPDATE_RECT (viewport);
}

void
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

  AVT_UPDATE_RECT (viewport);
}

void
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

void
avt_newline_mode (avt_bool_t mode)
{
  newline_mode = mode;
}

void
avt_auto_margin (avt_bool_t mode)
{
  auto_margin = mode;
}

void
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

avt_bool_t
avt_get_origin_mode (void)
{
  return origin_mode;
}

void
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

  AVT_UPDATE_RECT (viewport);
}

void
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

  AVT_UPDATE_RECT (dst);
}

void
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

  AVT_UPDATE_RECT (dst);
}

void
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

  AVT_UPDATE_RECT (dst);
}

/* clear beginning of line */
void
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

  AVT_UPDATE_RECT (dst);
}

void
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

  AVT_UPDATE_RECT (dst);
}

int
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
    AVT_UPDATE_RECT (viewport);

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
      if (text_cursor_visible)
	avt_show_text_cursor (AVT_FALSE);

      avt_delete_lines (((viewport.y - textfield.y) / LINEHEIGHT) + 1, 1);

      if (origin_mode)
	cursor.y = viewport.y + viewport.h - LINEHEIGHT;
      else
	cursor.y = textfield.y + textfield.h - LINEHEIGHT;

      if (newline_mode)
	cursor.x = linestart;

      if (text_cursor_visible)
	avt_show_text_cursor (AVT_TRUE);
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

int
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
  int lx, ly;
  SDL_Rect dest;
  Uint8 *p, *dest_line;
  Uint16 pitch;
  int front;

  /* assert (avt_character->format->BytesPerPixel == 1); */

  pitch = avt_character->pitch;
  p = (Uint8 *) avt_character->pixels;
  font_line = get_font_char (ch);
  dest_line = p;

  if (inverse)
    {
      /* fill all */
      SDL_memset (p, 1, FONTHEIGHT * pitch);
      front = 0;
    }
  else
    {
      /* clear all */
      SDL_memset (p, 0, FONTHEIGHT * pitch);
      front = 1;
    }

  if (!bold || NOT_BOLD)
    for (ly = 0; ly < FONTHEIGHT; ly++)
      {
	for (lx = 0; lx < FONTWIDTH; lx++)
	  if (*font_line & (1 << (15 - lx)))
	    *(dest_line + lx) = front;
	font_line++;
	dest_line += pitch;
      }
  else				/* bold */
    for (ly = 0; ly < FONTHEIGHT; ly++)
      {
	/* ignore last column to avoid overflows */
	for (lx = 0; lx < FONTWIDTH - 1; lx++)
	  if (*font_line & (1 << (15 - lx)))
	    {
	      *(dest_line + lx) = front;
	      *(dest_line + lx + 1) = front;
	    }
	font_line++;
	dest_line += pitch;
      }

  if (underlined)
    SDL_memset (p + (UNDERLINE * pitch), front, FONTWIDTH * 1);

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
  int lx, ly;
  SDL_Rect dest;
  Uint8 *p, *dest_line;
  Uint16 pitch;
  int front;

  /* assert (avt_character->format->BytesPerPixel == 1); */

  pitch = avt_character->pitch;
  p = (Uint8 *) avt_character->pixels;
  font_line = get_font_char (ch);
  dest_line = p;

  if (inverse)
    {
      /* fill all */
      SDL_memset (p, 1, FONTHEIGHT * pitch);
      front = 0;
    }
  else
    {
      /* clear all */
      SDL_memset (p, 0, FONTHEIGHT * pitch);
      front = 1;
    }

  if (!bold || NOT_BOLD)
    for (ly = 0; ly < FONTHEIGHT; ly++)
      {
	for (lx = 0; lx < FONTWIDTH; lx++)
	  if (*font_line & (1 << (7 - lx)))
	    *(dest_line + lx) = front;
	font_line++;
	dest_line += pitch;
      }
  else				/* bold */
    for (ly = 0; ly < FONTHEIGHT; ly++)
      {
	/* ignore last column to avoid overflows */
	for (lx = 0; lx < FONTWIDTH - 1; lx++)
	  if (*font_line & (1 << (7 - lx)))
	    {
	      *(dest_line + lx) = front;
	      *(dest_line + lx + 1) = front;
	    }
	font_line++;
	dest_line += pitch;
      }

  if (underlined)
    SDL_memset (p + (UNDERLINE * pitch), front, FONTWIDTH * 1);

  dest.x = cursor.x;
  dest.y = cursor.y;
  SDL_BlitSurface (avt_character, NULL, surface, &dest);
}

#endif /* FONTWIDTH <= 8 */

/* make current char visible */
static void
avt_showchar (void)
{
  SDL_UpdateRect (screen, cursor.x, cursor.y, FONTWIDTH, FONTHEIGHT);
  text_cursor_actually_visible = AVT_FALSE;
}

/* advance position - only in the textfield */
int
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

void
avt_reset_tab_stops (void)
{
  int i;

  for (i = 0; i < AVT_LINELENGTH; i++)
    if (i % 8 == 0)
      avt_tab_stops[i] = AVT_TRUE;
    else
      avt_tab_stops[i] = AVT_FALSE;
}

void
avt_clear_tab_stops (void)
{
  SDL_memset (&avt_tab_stops, AVT_FALSE, sizeof (avt_tab_stops));
}

void
avt_set_tab (int x, avt_bool_t onoff)
{
  avt_tab_stops[x - 1] = AVT_MAKE_BOOL (onoff);
}

/* advance to next tabstop */
void
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
void
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

void
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
int
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
      if (avt_bell_func)
	(*avt_bell_func) ();
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
 * writes L'\0' terminated string to textfield - 
 * interprets control characters
 */
int
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
      if (avt_put_character (*txt))
	break;

      txt++;
    }

  return _avt_STATUS;
}

/*
 * writes string with given length to textfield - 
 * interprets control characters
 */
int
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
      if (avt_put_character (*txt))
	break;
    }

  return _avt_STATUS;
}

int
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
int
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

int
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

void
avt_free (void *ptr)
{
  if (ptr)
    SDL_free (ptr);
}

int
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

int
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

int
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

int
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
int
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
int
avt_get_menu (wchar_t * ch, int menu_start, int menu_end, wchar_t start_code)
{
  int status, result;

  status = avt_choice (&result, menu_start, menu_end - menu_start + 1,
		       (int) start_code, AVT_FALSE, AVT_FALSE);

  *ch = result + start_code - 1;
  return status;
}

/* size in Bytes! */
int
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
  /* it is not changed when the window is resized */
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

      avt_get_key (&ch);
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
	  else if (avt_bell_func)
	    (*avt_bell_func) ();
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
	  else if (avt_bell_func)
	    (*avt_bell_func) ();
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

int
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

int
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

	  /* if window is resized then break */
	  if (window.x != mywindow.x || window.y != mywindow.y)
	    break;

	  /* some time for other processes */
	  SDL_Delay (1);
	}

      /* final position (even when window was resized) */
      avt_show_avatar ();
    }

  return _avt_STATUS;
}

int
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

	  /* if window is resized then break */
	  if (window.x != mywindow.x || window.y != mywindow.y)
	    break;

	  /* some time for other processes */
	  SDL_Delay (1);
	}
    }

  /* fill the whole screen with background color */
  avt_clear_screen ();

  return _avt_STATUS;
}

int
avt_wait_button (void)
{
  SDL_Event event;
  SDL_Surface *button;
  SDL_Rect dst;
  avt_bool_t nokey;

  if (!screen)
    return _avt_STATUS;

  /* show button */
  button = avt_load_image_xpm (btn_cont_xpm);

  /* alignment: right bottom */
  dst.x = window.x + window.w - button->w - AVATAR_MARGIN;
  dst.y = window.y + window.h - button->h - AVATAR_MARGIN;
  dst.w = button->w;
  dst.h = button->h;

  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (button, NULL, screen, &dst);
  AVT_UPDATE_RECT (dst);
  SDL_FreeSurface (button);
  button = NULL;

  /* prepare for possible resize */
  avt_pre_resize (dst);

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
  SDL_SetClipRect (screen, &window);
  avt_post_resize (dst);
  SDL_FillRect (screen, &dst, background_color);
  AVT_UPDATE_RECT (dst);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return _avt_STATUS;
}

/* deprecated: use avt_wait_button */
int
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

  /* prepare for being resized */
  avt_pre_resize (dst);

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

	case SDL_VIDEORESIZE:
	  avt_resize (event.resize.w, event.resize.h);
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
      avt_post_resize (dst);
      SDL_FillRect (screen, &dst, background_color);
      AVT_UPDATE_RECT (dst);
    }

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return _avt_STATUS;
}

/* deprecated: use avt_wait_button */
int
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

avt_bool_t
avt_decide (void)
{
  SDL_Event event;
  SDL_Surface *yes_button, *no_button;
  SDL_Rect yes_rect, no_rect;
  int result;

  if (!screen)
    return _avt_STATUS;

  SDL_SetClipRect (screen, &window);

  /* show buttons */
  yes_button = avt_load_image_xpm (btn_yes_xpm);
  no_button = avt_load_image_xpm (btn_no_xpm);

  /* alignment: right bottom */
  yes_rect.x = window.x + window.w - yes_button->w - AVATAR_MARGIN;
  yes_rect.y = window.y + window.h - yes_button->h - AVATAR_MARGIN;
  yes_rect.w = yes_button->w;
  yes_rect.h = yes_button->h;
  SDL_BlitSurface (yes_button, NULL, screen, &yes_rect);

  no_rect.x = yes_rect.x - BUTTON_DISTANCE - no_button->w;
  no_rect.y = yes_rect.y;
  no_rect.w = no_button->w;
  no_rect.h = no_button->h;
  SDL_BlitSurface (no_button, NULL, screen, &no_rect);

  AVT_UPDATE_RECT (no_rect);
  AVT_UPDATE_RECT (yes_rect);

  SDL_FreeSurface (yes_button);
  SDL_FreeSurface (no_button);
  no_button = yes_button = NULL;

  /* prepare for possible resize */
  avt_pre_resize (yes_rect);
  avt_pre_resize (no_rect);

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
	      && event.button.y >= yes_rect.y + window.y
	      && event.button.y <= yes_rect.y + window.y + yes_rect.h)
	    {
	      if (event.button.x >= yes_rect.x + window.x
		  && event.button.x <= yes_rect.x + window.x + yes_rect.w)
		result = AVT_TRUE;
	      else
		if (event.button.x >= no_rect.x + window.x
		    && event.button.x <= no_rect.x + window.x + no_rect.w)
		result = AVT_FALSE;
	    }
	  break;
	}

      avt_analyze_event (&event);
    }

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

  /* delete buttons */
  SDL_SetClipRect (screen, &window);
  avt_post_resize (yes_rect);
  avt_post_resize (no_rect);
  SDL_FillRect (screen, &no_rect, background_color);
  SDL_FillRect (screen, &yes_rect, background_color);
  AVT_UPDATE_RECT (no_rect);
  AVT_UPDATE_RECT (yes_rect);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return AVT_MAKE_BOOL (result);
}

/* free avt_image_t images */
void
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
 * if SDL_image isn't available then uncompressed BMP is still supported
 */
int
avt_show_image_file (const char *file)
{
  SDL_Surface *image;

  if (!screen)
    return _avt_STATUS;

  load_image_init ();
  image = load_image.file (file);

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
int
avt_show_image_data (void *img, int imgsize)
{
  SDL_Surface *image;

  if (!screen)
    return _avt_STATUS;

  load_image_init ();
  image = load_image.rw (SDL_RWFromMem (img, imgsize), 1);

  if (image == NULL)
    {
      avt_clear ();		/* at least clear the balloon */
      return AVT_ERROR;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}

int
avt_show_image_XPM (char **xpm)
{
  avt_image_t *image = NULL;

  if (!screen)
    return _avt_STATUS;

  image = avt_import_XPM (xpm);

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
int
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

      SDL_SetError ("94981e6dd48cb9985c7cc1e93a957062");

      if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	_avt_STATUS = AVT_ERROR;
    }

  return _avt_STATUS;
}

/*
 * make background transparent
 * pixel in the upper left corner is supposed to be the background color
 */
avt_image_t *
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

avt_image_t *
avt_import_XPM (char **xpm)
{
  SDL_Surface *image;

  if (avt_init_SDL ())
    return NULL;

  image = NULL;

  /* if load_image isn't intitialized, try the internal loader first */
  if (!load_image.initialized)
    image = avt_load_image_xpm (xpm);

  /* if image wasn't loaded yet or loading failed */
  if (image == NULL)
    {
      /* try load_image framework */
      load_image_init ();
      image = load_image.xpm (xpm);
    }

  return (avt_image_t *) image;
}


/* 
 * import RGB gimp_image as avatar
 * pixel in the upper left corner is supposed to be the background color
 */
avt_image_t *
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
avt_image_t *
avt_import_image_data (void *img, int imgsize)
{
  SDL_Surface *image;

  if (avt_init_SDL ())
    return NULL;

  load_image_init ();
  image = load_image.rw (SDL_RWFromMem (img, imgsize), 1);

  /* if it's not yet transparent, make it transparent */
  if (image)
    if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
      avt_make_transparent (image);

  return (avt_image_t *) image;
}

/* 
 * import avatar from file
 */
avt_image_t *
avt_import_image_file (const char *file)
{
  SDL_Surface *image;

  if (avt_init_SDL ())
    return NULL;

  load_image_init ();
  image = load_image.file (file);

  /* if it's not yet transparent, make it transparent */
  if (image)
    if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
      avt_make_transparent (image);

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
int
avt_change_avatar_image (avt_image_t * image)
{
  if (avt_visible)
    avt_clear_screen ();

  if (avt_image)
    {
      SDL_FreeSurface (avt_image);
      avt_image = NULL;
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

/* can and should be called before avt_initialize */
void
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
	  avt_visible = AVT_FALSE;
	  avt_draw_balloon ();
	  AVT_UPDATE_ALL ();
	}
      else if (avt_visible)
	avt_show_avatar ();
      else
	avt_clear_screen ();
    }
}

void
avt_reserve_single_keys (avt_bool_t onoff)
{
  reserve_single_keys = AVT_MAKE_BOOL (onoff);
}

/* just for backward compatiblity */
void
avt_stop_on_esc (avt_bool_t on)
{
  reserve_single_keys = (on == AVT_FALSE);
}

void
avt_register_keyhandler (avt_keyhandler handler)
{
  avt_ext_keyhandler = handler;
}

void
avt_register_mousehandler (avt_mousehandler handler)
{
  avt_ext_mousehandler = handler;

  /* make mouse visible or invisible */
  if (handler)
    SDL_ShowCursor (SDL_ENABLE);
  else
    SDL_ShowCursor (SDL_DISABLE);
}

void
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

void
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

void
avt_inverse (avt_bool_t onoff)
{
  inverse = AVT_MAKE_BOOL (onoff);
}

avt_bool_t
avt_get_inverse (void)
{
  return inverse;
}

void
avt_bold (avt_bool_t onoff)
{
  bold = AVT_MAKE_BOOL (onoff);
}

avt_bool_t
avt_get_bold (void)
{
  return bold;
}

void
avt_underlined (avt_bool_t onoff)
{
  underlined = AVT_MAKE_BOOL (onoff);
}

avt_bool_t
avt_get_underlined (void)
{
  return underlined;
}

void
avt_normal_text (void)
{
  underlined = bold = inverse = AVT_FALSE;

  /* set color table for character canvas */
  if (avt_character)
    {
      SDL_Color colors[2];

      /* white background */
      colors[0].r = colors[0].g = colors[0].b = 0xFF;
      /* black foreground */
      colors[1].r = colors[1].g = colors[1].b = 0x00;

      SDL_SetColors (avt_character, colors, 0, 2);
      text_background_color = SDL_MapRGB (screen->format, 0xFF, 0xFF, 0xFF);
    }
}

/* about to be removed */
void
avt_set_delays (int text, int flip_page)
{
  text_delay = text;
  flip_page_delay = flip_page;
}

void
avt_set_text_delay (int delay)
{
  text_delay = delay;
}

void
avt_set_flip_page_delay (int delay)
{
  flip_page_delay = delay;
}

void
avt_set_scroll_mode (int mode)
{
  scroll_mode = mode;
}

int
avt_get_scroll_mode (void)
{
  return scroll_mode;
}

char *
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

int
avt_credits (const wchar_t * text, avt_bool_t centered)
{
  wchar_t line[80];
  SDL_Surface *last_line;
  SDL_Color old_backgroundcolor;
  const wchar_t *p;
  int i;
  int length;

  if (!screen)
    return _avt_STATUS;

  /* store old background color */
  old_backgroundcolor = backgroundcolor_RGB;

  /* needed to handle resizing correctly */
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  avt_visible = AVT_FALSE;

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

  /* restore old background color */
  backgroundcolor_RGB = old_backgroundcolor;

  window.w = MINIMALWIDTH;
  window.h = MINIMALHEIGHT;
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  /* back to normal (also sets variables!) */
  avt_normal_text ();
  avt_clear_screen ();

  return _avt_STATUS;
}

int
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

void
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
  avt_bell_func = NULL;
  SDL_Quit ();
  screen = NULL;		/* it was freed by SDL_Quit */
  avt_visible = AVT_FALSE;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;
}

void
avt_button_quit (void)
{
  avt_wait_button ();
  avt_move_out ();
  avt_quit ();
}

void
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

int
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

  SDL_SetError ("$Id: avatar.c,v 2.227 2009-05-25 16:13:35 akf Exp $");

  /*
   * Initialize the display, accept any format
   */
  screenflags = SDL_SWSURFACE | SDL_ANYFORMAT | SDL_RESIZABLE;

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
					8, 0, 0, 0, 0);

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

  circle = avt_load_image_xpm (circle_xpm);
  pointer = avt_load_image_xpm (balloonpointer_xpm);

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

  /* visual flash for the bell */
  /* when you initialize the audio stuff, you get an audio "bell" */
  avt_bell_func = avt_flash;

  /* initialize tab stops */
  avt_reset_tab_stops ();

  return _avt_STATUS;
}
