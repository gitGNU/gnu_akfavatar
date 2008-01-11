/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * needed: 
 *  SDL1.2 (recommended: SDL1.2.11)
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

/* $Id: avatar.c,v 2.63 2008-01-11 09:47:29 akf Exp $ */

#include "akfavatar.h"
#include "SDL.h"
#include "version.h"

#include "balloonpointer.c"
#include "circle.c"
#include "regicon.c"
#include "keybtn.c"

#if defined(QVGA)
#  define FONTWIDTH 4
#  define FONTHEIGHT 6
#  define LINEHEIGHT FONTHEIGHT	/* + something, if you want */
#elif defined(VGA)
#  define FONTWIDTH 7
#  define FONTHEIGHT 14
#  define LINEHEIGHT FONTHEIGHT	/* + something, if you want */
#else
#  define FONTWIDTH 9
#  define FONTHEIGHT 18
#  define LINEHEIGHT FONTHEIGHT	/* + something, if you want */
#endif

/* 
 * newer vesions of SDL have some fallback implementations
 * for libc functionality - I try to use it, if available
 * This is the fallback for older SDL versions.
 */
#ifndef _SDL_stdinc_h
#  define OLD_SDL 1
#endif

#ifdef OLD_SDL
#  include <stdlib.h>
#  include <string.h>

#  ifndef FORCE_ICONV
#    define FORCE_ICONV
#  endif

#  undef SDL_malloc
#  define SDL_malloc              malloc
#  undef SDL_free
#  define SDL_free                free
#  undef SDL_strlen
#  define SDL_strlen              strlen
#  undef SDL_memcpy
#  define SDL_memcpy              memcpy
#  undef SDL_putenv
#  define SDL_putenv              putenv
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
#pragma GCC poison  malloc free strlen memcpy getenv putenv
/* do not poison the iconv stuff, it causes problems with GNU libiconv */


#define COLORDEPTH 24

#if defined(QVGA)
#  define MINIMALWIDTH 320
#  define MINIMALHEIGHT 240
#  define TOPMARGIN 10
#  define BALLOONWIDTH AVT_LINELENGTH
#  define BALLOON_INNER_MARGIN 10
#  define AVATAR_MARGIN 5
#  define MOVE_DELAY 5
#elif defined(VGA)
#  define MINIMALWIDTH 640
#  define MINIMALHEIGHT 480
#  define TOPMARGIN 25
#  define BALLOONWIDTH AVT_LINELENGTH
#  define BALLOON_INNER_MARGIN 10
#  define AVATAR_MARGIN 10
   /* Delay for moving in or out - the higher, the slower */
#  define MOVE_DELAY 2.5
#else
#  define MINIMALWIDTH 800
#  define MINIMALHEIGHT 600
#  define TOPMARGIN 25
#  define BALLOONWIDTH AVT_LINELENGTH
#  define BALLOON_INNER_MARGIN 15
#  define AVATAR_MARGIN 20
   /* Delay for moving in or out - the higher, the slower */
#  define MOVE_DELAY 1.8
#endif /* !QVGA, !LARGRE */


/* for dynamically loading SDL_image */
#ifndef SDL_IMAGE_LIB
#  if defined (__WIN32__)
#    define SDL_IMAGE_LIB "SDL_image.dll"
#  else	/* not Windows */
#    define SDL_IMAGE_LIB "libSDL_image-1.2.so.0"
#  endif /* not Windows */
#endif /* not SDL_IMAGE_LIB */

#define ICONV_UNINITIALIZED   (avt_iconv_t)(-1)

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
#      if (SDL_BYTEORDER == SDL_BIG_ENDIN)
#        define WCHAR_ENCODING "UTF-16BE"
#      else /* SDL_BYTEORDER != SDL_BIG_ENDIN */
#        define WCHAR_ENCODING "UTF-16LE"
#      endif /* SDL_BYTEORDER != SDL_BIG_ENDIAN */
#    else /* (WCHAR_MAX > 65535U) */
#      if (SDL_BYTEORDER == SDL_BIG_ENDIN)
#        define WCHAR_ENCODING "UTF-32BE"
#      else /* SDL_BYTEORDER != SDL_BIG_ENDIN */
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

/* type for gimp images */
typedef struct
{
  unsigned int width;
  unsigned int height;
  unsigned int bytes_per_pixel;	/* 3:RGB, 4:RGBA */
  unsigned char pixel_data;	/* handle as startpoint */
} gimp_img_t;

/* for dynamically loading SDL_image (SDL-1.2.6 or better) */
static avt_bool_t tried_to_load_SDL_image;
static void *SDL_image_handle;
static SDL_Surface *(*IMG_Load) (const char *file);
static SDL_Surface *(*IMG_Load_RW) (SDL_RWops * src, int freesrc);

/* for an external keyboard handler */
static avt_keyhandler avt_ext_keyhandler = NULL;


static SDL_Surface *screen, *avt_image, *avt_character;
static Uint32 screenflags;	/* flags for the screen */
static int avt_mode;		/* whether fullscreen or window or ... */
static avt_bool_t must_lock;	/* must the screen be locked? */
static SDL_Rect window;		/* if screen is in fact larger */
static avt_bool_t avt_visible;
static avt_bool_t do_stop_on_esc;	/* stop, when Esc is pressed? */
static int scroll_mode;
static SDL_Rect textfield;
static SDL_Rect viewport;	/* sub-window in textfield */
static int textdir_rtl;
/* beginning of line - depending on text direction */
static int linestart;
static int balloonheight;

/* delay values for printing text and flipping the page */
static int text_delay, flip_page_delay;

/* color independent from the screen mode */
static SDL_Color backgroundcolor_RGB = { 0xCC, 0xCC, 0xCC, 0 };

/* conversion descriptors for text input and output */
static avt_iconv_t output_cd = ICONV_UNINITIALIZED;
static avt_iconv_t input_cd = ICONV_UNINITIALIZED;

static struct pos
{
  int x, y;
} cursor;

/* 0 = normal; 1 = quit-request; -1 = error */
int _avt_STATUS;

void (*avt_bell_func) (void) = NULL;
void (*avt_quit_audio_func) (void) = NULL;

/* forward declaration */
static int avt_pause (void);


#ifdef FORCE_ICONV
static size_t
avt_iconv (avt_iconv_t cd,
	   char **inbuf, size_t * inbytesleft,
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
  return "Copyright (c) 2007 Andreas K. Foerster";
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

/* taken from the SDL documentation */
/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
static void
putpixel (SDL_Surface * surface, int x, int y, Uint32 color)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
    {
    case 1:
      *p = color;
      break;
    case 2:
      *(Uint16 *) p = color;
      break;
    case 4:
      *(Uint32 *) p = color;
      break;
    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	{
	  p[0] = (color >> 16) & 0xff;
	  p[1] = (color >> 8) & 0xff;
	  p[2] = color & 0xff;
	}
      else
	{
	  p[0] = color & 0xff;
	  p[1] = (color >> 8) & 0xff;
	  p[2] = (color >> 16) & 0xff;
	}
      break;
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

/* fills the screen with the background color,
 * but doesn't update the screen yet 
 */
static void
avt_free_screen (void)
{
  /* switch clipping off */
  SDL_SetClipRect (screen, NULL);
  /* fill the whole screen with background color */
  SDL_FillRect (screen, NULL,
		SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
			    backgroundcolor_RGB.g, backgroundcolor_RGB.b));
}

void
avt_clear_screen (void)
{
  if (screen)
    {
      avt_free_screen ();
      SDL_UpdateRect (screen, 0, 0, 0, 0);
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
      SDL_UpdateRect (screen, 0, 0, 0, 0);

      /* undefine textfield */
      textfield.x = textfield.y = textfield.w = textfield.h = -1;
      viewport = textfield;
      avt_visible = AVT_TRUE;
    }
}

static void
avt_draw_balloon (void)
{
  Uint32 ballooncolor, backgroundcolor;
  SDL_Rect dst;
  int x, y;
  int xoffs, yoffs;
  int radius;

  if (!avt_visible)
    avt_draw_avatar ();

  backgroundcolor = SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
				backgroundcolor_RGB.g, backgroundcolor_RGB.b);
  ballooncolor = SDL_MapRGB (screen->format, 0xFF, 0xFF, 0xFF);

  textfield.x = window.x + (window.w / 2) - (BALLOONWIDTH * FONTWIDTH / 2);
  textfield.y = window.y + TOPMARGIN + BALLOON_INNER_MARGIN;
  textfield.w = (BALLOONWIDTH * FONTWIDTH);
  textfield.h = balloonheight - (2 * BALLOON_INNER_MARGIN);
  viewport = textfield;

  /* somewat lager rectangle */
  dst.x = textfield.x - BALLOON_INNER_MARGIN;
  dst.y = window.y + TOPMARGIN;
  dst.w = (BALLOONWIDTH * FONTWIDTH) + (2 * BALLOON_INNER_MARGIN);
  dst.h = balloonheight;

  /* sanity check */
  if (dst.x < window.x)
    dst.x = window.x;

  if (dst.w > window.w)
    dst.w = window.w;

  SDL_FillRect (screen, &dst, ballooncolor);

  /* draw corners */
  if (must_lock)
    SDL_LockSurface (screen);

  radius = circle.width / 2;

  /* upper left corner */
  xoffs = dst.x;
  yoffs = dst.y;
  for (y = 0; y < radius; ++y)
    for (x = 0; x < radius; ++x)
      if (circle.data[circle.width * y + x] == 32)
	putpixel (screen, x + xoffs, y + yoffs, backgroundcolor);

  /* upper right corner */
  xoffs = dst.x + dst.w - radius;
  yoffs = dst.y;
  for (y = 0; y < radius; ++y)
    for (x = 0; x < radius; ++x)
      if (circle.data[circle.width * y + x + radius] == 32)
	putpixel (screen, x + xoffs, y + yoffs, backgroundcolor);

  /* lower left corner */
  xoffs = dst.x;
  yoffs = dst.y + dst.h - radius;
  for (y = 0; y < radius; ++y)
    for (x = 0; x < radius; ++x)
      if (circle.data[circle.width * (y + radius) + x] == 32)
	putpixel (screen, x + xoffs, y + yoffs, backgroundcolor);

  /* lower right corner */
  xoffs = dst.x + dst.w - radius;
  yoffs = dst.y + dst.h - radius;
  for (y = 0; y < radius; ++y)
    for (x = 0; x < radius; ++x)
      if (circle.data[circle.width * (y + radius) + x + radius] == 32)
	putpixel (screen, x + xoffs, y + yoffs, backgroundcolor);

  /* draw balloonpointer */
  /* only if there is an avatar image */
  if (avt_image)
    {
      int cut_top = 0;

      /* if the balloonpointer is too large, cut it */
      if (balloonpointer.height > (avt_image->h / 2))
	cut_top = balloonpointer.height - (avt_image->h / 2);

      xoffs = window.x + avt_image->w + (2 * AVATAR_MARGIN) + 20;
      yoffs = window.y + balloonheight + TOPMARGIN;

      /* only draw the balloonpointer, when it fits */
      if (xoffs + balloonpointer.width < window.x + window.w)
	{
	  for (y = cut_top; y < balloonpointer.height; ++y)
	    for (x = 0; x < balloonpointer.width; ++x)
	      if (balloonpointer.data[balloonpointer.width * y + x] != 32)
		putpixel (screen, x + xoffs, y - cut_top + yoffs,
			  ballooncolor);
	}
    }

  if (must_lock)
    SDL_UnlockSurface (screen);

  linestart =
    (textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;

  avt_visible = AVT_TRUE;

  /* cursor at top  */
  cursor.x = linestart;
  cursor.y = viewport.y;

  /* 
   * only allow drawings inside this area from now on 
   * (only for blitting)
   */
  SDL_SetClipRect (screen, &viewport);
  SDL_UpdateRect (screen, window.x, window.y, window.w, window.h);
}

void
avt_text_direction (int direction)
{
  textdir_rtl = direction;

  /* 
   * if there is already a ballon, 
   * recalculate the linestart and put the cursor in the first position
   */
  if (screen && textfield.x >= 0)
    {
      linestart =
	(textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;
      cursor.x = linestart;
    }
}

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

  /* must the screen be locked? */
  must_lock = SDL_MUSTLOCK (screen);

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

  /* make all changes visible */
  SDL_UpdateRect (screen, 0, 0, 0, 0);
}

void
avt_bell (void)
{
  if (avt_bell_func)
    (*avt_bell_func) ();
}

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
  SDL_UpdateRect (screen, 0, 0, 0, 0);
  SDL_Delay (150);

  /* fill the whole screen with background color */
  SDL_FillRect (screen, NULL,
		SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
			    backgroundcolor_RGB.g, backgroundcolor_RGB.b));
  /* restore image */
  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (oldwindowimage, NULL, screen, &window);
  SDL_FreeSurface (oldwindowimage);

  /* make visible again */
  SDL_UpdateRect (screen, 0, 0, 0, 0);
}

static void
avt_toggle_fullscreen (void)
{
  if (avt_mode != AVT_FULLSCREENNOSWITCH)
    {
      /* toggle bit for fullscreenmode */
      screenflags ^= SDL_FULLSCREEN;
      /* set the mode with minimal size */
      avt_resize (MINIMALWIDTH, MINIMALHEIGHT);

      if ((screenflags & SDL_FULLSCREEN) != 0)
	avt_mode = AVT_FULLSCREEN;
      else
	avt_mode = AVT_WINDOW;
    }
}

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
	      avt_resize (MINIMALWIDTH, MINIMALHEIGHT);
	    }
	  break;
	}
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

    case SDL_KEYDOWN:
      switch (event->key.keysym.sym)
	{
	case SDLK_PAUSE:
	  avt_pause ();
	  break;

	case SDLK_F4:
	case SDLK_F11:
	  avt_toggle_fullscreen ();
	  break;

	  /* no "break" default for the following ones: */
	  /* they may fall through to default: */

	case SDLK_ESCAPE:
	  if (event->key.keysym.sym == SDLK_ESCAPE && do_stop_on_esc)
	    {
	      _avt_STATUS = AVT_QUIT;
	      break;
	    }

	  /* Ctrl + Q -> Quit */
	case SDLK_q:
	  if (event->key.keysym.sym == SDLK_q
	      && (event->key.keysym.mod & KMOD_CTRL))
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
    return ((cursor.x - viewport.x) / FONTWIDTH) + 1;
  else
    return -1;
}

int
avt_where_y (void)
{
  if (screen && textfield.x >= 0)
    return ((cursor.y - viewport.y) / LINEHEIGHT) + 1;
  else
    return -1;
}

int
avt_get_max_x (void)
{
  if (screen)
    return AVT_LINELENGTH;
  else
    return -1;
}

int
avt_get_max_y (void)
{
  if (screen)
    return (balloonheight - (2 * BALLOON_INNER_MARGIN)) / LINEHEIGHT;
  else
    return -1;
}

void
avt_move_x (int x)
{
  if (screen && textfield.x >= 0)
    {
      if (x < 1)
	x = 1;
      cursor.x = (x - 1) * FONTWIDTH + viewport.x;

      /* max-pos exeeded? */
      if (cursor.x > viewport.x + viewport.w - FONTWIDTH)
	cursor.x = viewport.x + viewport.w - FONTWIDTH;
    }
}

void
avt_move_y (int y)
{
  if (screen && textfield.x >= 0)
    {
      if (y < 1)
	y = 1;
      cursor.y = (y - 1) * LINEHEIGHT + viewport.y;

      /* max-pos exeeded? */
      if (cursor.y > viewport.y + viewport.h - LINEHEIGHT)
	cursor.y = viewport.y + viewport.h - LINEHEIGHT;
    }
}

void
avt_delete_lines (int line, int num)
{
  SDL_Rect rest, dest, clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  /* check if values are sane */
  if (line < 1 || num < 1 || line > (viewport.h / LINEHEIGHT))
    return;

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
  SDL_FillRect (screen, &clear,
		SDL_MapRGB (screen->format, 0xFF, 0xFF, 0xFF));

  SDL_UpdateRect (screen, viewport.x, viewport.y, viewport.w, viewport.h);
}

void
avt_insert_lines (int line, int num)
{
  SDL_Rect rest, dest, clear;

  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return;

  /* check if values are sane */
  if (line < 1 || num < 1 || line > (viewport.h / LINEHEIGHT))
    return;

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
  SDL_FillRect (screen, &clear,
		SDL_MapRGB (screen->format, 0xFF, 0xFF, 0xFF));

  SDL_UpdateRect (screen, viewport.x, viewport.y, viewport.w, viewport.h);
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

  viewport.x = textfield.x + ((x - 1) * FONTWIDTH);
  viewport.y = textfield.y + ((y - 1) * LINEHEIGHT);
  viewport.w = width * FONTWIDTH;
  viewport.h = height * LINEHEIGHT;

  linestart =
    (textdir_rtl) ? viewport.x + viewport.w - FONTWIDTH : viewport.x;

  cursor.x = linestart;
  cursor.y = viewport.y;

  SDL_SetClipRect (screen, &viewport);
}

void
avt_clear (void)
{
  SDL_Color color;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  /* use background color of characters */
  color = avt_character->format->palette->colors[0];
  SDL_FillRect (screen, &viewport,
		SDL_MapRGB (screen->format, color.r, color.g, color.b));

  SDL_UpdateRect (screen, viewport.x, viewport.y, viewport.w, viewport.h);
  cursor.x = linestart;
  cursor.y = viewport.y;
}

void
avt_clear_eol (void)
{
  SDL_Color color;
  SDL_Rect dst;

  /* not initialized? -> do nothing */
  if (!screen)
    return;

  /* if there's no balloon, draw it */
  if (textfield.x < 0)
    avt_draw_balloon ();

  /* use background color of characters */
  color = avt_character->format->palette->colors[0];

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

  SDL_FillRect (screen, &dst,
		SDL_MapRGB (screen->format, color.r, color.g, color.b));
  SDL_UpdateRect (screen, dst.x, dst.y, dst.w, dst.h);
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
    SDL_UpdateRect (screen, viewport.x, viewport.y, viewport.w, viewport.h);

  avt_wait (flip_page_delay);
  avt_clear ();
  return _avt_STATUS;
}

static void
avt_scroll_up (void)
{
  if (scroll_mode)
    {
      avt_delete_lines (1, 1);
      cursor.x = linestart;
      cursor.y = viewport.y + viewport.h - LINEHEIGHT;
    }
  else
    avt_flip_page ();
}

int
avt_new_line (void)
{
  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return _avt_STATUS;

  cursor.x = linestart;
  cursor.y += LINEHEIGHT;

  /* if the cursor is beyond the end of the viewport,
   * get a new page 
   */
  if (cursor.y > viewport.y + viewport.h - LINEHEIGHT)
    avt_scroll_up ();

  return _avt_STATUS;
}

/* avt_drawchar: draws the raw char - with no interpretation */
#if (FONTWIDTH > 8)
static void
avt_drawchar (wchar_t ch)
{
  extern unsigned short font[];
  extern size_t get_font_offset (wchar_t ch);
  int lx, ly;
  size_t font_offset;
  SDL_Rect dest;
  Uint8 *p, *dest_line;
  unsigned short font_line;
  Uint16 pitch;

  font_offset = get_font_offset (ch);

  pitch = avt_character->pitch;
  p = (Uint8 *) avt_character->pixels;
  for (ly = 0; ly < FONTHEIGHT; ly++)
    {
      dest_line = p + (ly * pitch);
      font_line = font[font_offset + ly];

      for (lx = 0; lx < FONTWIDTH; lx++)
	*(dest_line + lx) = (font_line & (1 << (15 - lx))) ? 1 : 0;
    }

  dest.x = cursor.x;
  dest.y = cursor.y;
  SDL_BlitSurface (avt_character, NULL, screen, &dest);
}

#else /* FONTWIDTH <= 8 */

static void
avt_drawchar (wchar_t ch)
{
  extern unsigned char font[];
  extern size_t get_font_offset (wchar_t ch);
  int lx, ly;
  size_t font_offset;
  SDL_Rect dest;
  Uint8 *p, *dest_line;
  unsigned char font_line;
  Uint16 pitch;

  font_offset = get_font_offset (ch);

  pitch = avt_character->pitch;
  p = (Uint8 *) avt_character->pixels;
  for (ly = 0; ly < FONTHEIGHT; ly++)
    {
      dest_line = p + (ly * pitch);
      font_line = font[font_offset + ly];

      for (lx = 0; lx < FONTWIDTH; lx++)
	*(dest_line + lx) = (font_line & (1 << (7 - lx))) ? 1 : 0;
    }

  dest.x = cursor.x;
  dest.y = cursor.y;
  SDL_BlitSurface (avt_character, NULL, screen, &dest);
}

#endif /* FONTWIDTH <= 8 */

/* make current char visible */
static void
avt_showchar (void)
{
  SDL_UpdateRect (screen, cursor.x, cursor.y, FONTWIDTH, FONTHEIGHT);
}

/* advance position - only in the textfield */
int
avt_forward (void)
{
  /* no textfield? do nothing */
  if (!screen || textfield.x < 0)
    return _avt_STATUS;

  if (textdir_rtl)		/* right to left */
    {
      cursor.x -= FONTWIDTH;
      if (cursor.x < viewport.x)
	avt_new_line ();
    }
  else				/* left to right */
    {
      cursor.x += FONTWIDTH;
      if (cursor.x > viewport.x + viewport.w - FONTWIDTH)
	avt_new_line ();
    }

  return _avt_STATUS;
}

/* advance to next tabstop */
static void
avt_nexttab (void)
{
  int x, i;

  x = avt_where_x ();

  if (textdir_rtl)
    {
      if (x < 8)
	avt_new_line ();
      else			/* TODO: improve */
	for (i = 0; i < 8 - (((viewport.w / FONTWIDTH) - x - 1) % 8); i++)
	  avt_forward ();
    }
  else
    {
      if (x > (viewport.w / FONTWIDTH) - 8)
	avt_new_line ();
      else
	for (i = 0; i < 8 - (x % 8); i++)
	  avt_forward ();
    }
}

static void
avt_clearchar (void)
{
  SDL_Rect dst;
  SDL_Color color;

  dst.x = cursor.x;
  dst.y = cursor.y;
  dst.w = FONTWIDTH;
  dst.h = FONTHEIGHT;

  color = avt_character->format->palette->colors[0];
  SDL_FillRect (screen, &dst,
		SDL_MapRGB (screen->format, color.r, color.g, color.b));
  avt_showchar ();
}

void
avt_backspace (void)
{
  if (screen)
    {
      if (cursor.x != linestart)
	cursor.x =
	  (textdir_rtl) ? cursor.x + FONTWIDTH : cursor.x - FONTWIDTH;

      avt_clearchar ();
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
    return _avt_STATUS;

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  switch (ch)
    {
    case L'\n':
      avt_new_line ();
      break;

    case L'\r':
      cursor.x = linestart;
      break;

    case L'\f':
    case L'\v':
      avt_flip_page ();
      break;

    case L'\t':
      avt_nexttab ();
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
      avt_clearchar ();
      avt_forward ();
      /* 
       * no delay for the space char 
       * it'd be annoying if you have a sequence of spaces 
       */
      break;

    default:
      if (ch > 32)
	{
	  avt_drawchar (ch);
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
    return _avt_STATUS;

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  while (*txt != L'\0')
    {
      avt_put_character (*txt);

      /* premature break */
      if (_avt_STATUS)
	return _avt_STATUS;

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
  if (!screen || !txt || !*txt)
    return _avt_STATUS;

  /* no textfield? => draw balloon */
  if (textfield.x < 0)
    avt_draw_balloon ();

  for (i = 0; i < len; i++)
    {
      avt_put_character (*txt);

      /* premature break */
      if (_avt_STATUS)
	return _avt_STATUS;

      txt++;
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
  char *inbuf_start, *inbuf, *outbuf;
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
    SDL_memcpy (inbuf, &rest_buffer, rest_bytes);

  /* copy the text into the buffer */
  SDL_memcpy (inbuf + rest_bytes, src, size);
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
      SDL_memcpy (&rest_buffer, inbuf, rest_bytes);
    }

  /* free the inbuf */
  SDL_free (inbuf_start);

  /* terminate outbuf */
  if (outbytesleft >= sizeof (wchar_t))
    *((wchar_t *) outbuf) = L'\0';

  return ((dest_size - outbytesleft) / sizeof (wchar_t));
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
      *ch = 0;
      while ((*ch <= 0) && (_avt_STATUS == AVT_NORMAL))
	{
	  SDL_WaitEvent (&event);
	  avt_analyze_event (&event);

	  if (event.type == SDL_KEYDOWN)
	    *ch = (wchar_t) event.key.keysym.unicode;
	}
    }

  return _avt_STATUS;
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

  do
    {
      /* show cursor */
      avt_drawchar (1);
      avt_showchar ();

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
	      cursor.x =
		(textdir_rtl) ? cursor.x - FONTWIDTH : cursor.x + FONTWIDTH;
	      avt_backspace ();
	      avt_backspace ();
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
	      cursor.x =
		(textdir_rtl) ? cursor.x - FONTWIDTH : cursor.x + FONTWIDTH;
	      avt_backspace ();
	      s[len] = ch;
	      len++;
	      s[len] = L'\0';
	      avt_drawchar (ch);
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
  cursor.x = (textdir_rtl) ? cursor.x - FONTWIDTH : cursor.x + FONTWIDTH;
  avt_backspace ();

  avt_new_line ();

  return _avt_STATUS;
}

int
avt_ask_mb (char *s, const int size)
{
  wchar_t ws[AVT_LINELENGTH + 1];
  char *inbuf;
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
      Uint32 backgroundcolor;
      SDL_Rect mywindow;

      backgroundcolor =
	SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
		    backgroundcolor_RGB.g, backgroundcolor_RGB.b);

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
	      SDL_FillRect (screen, &dst, backgroundcolor);
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
      Uint32 backgroundcolor;
      Sint16 start_position;
      SDL_Rect mywindow;

      backgroundcolor =
	SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
		    backgroundcolor_RGB.g, backgroundcolor_RGB.b);

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
      SDL_FillRect (screen, &dst, backgroundcolor);

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
	      SDL_FillRect (screen, &dst, backgroundcolor);
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
  button = SDL_LoadBMP_RW (SDL_RWFromMem ((void *) keybtn, keybtn_size), 1);

  /* alignment: right bottom */
  dst.x = window.x + window.w - button->w - AVATAR_MARGIN;
  dst.y = window.y + window.h - button->h - AVATAR_MARGIN;
  dst.w = button->w;
  dst.h = button->h;

  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (button, NULL, screen, &dst);
  SDL_UpdateRect (screen, dst.x, dst.y, dst.w, dst.h);
  SDL_FreeSurface (button);
  button = NULL;

  /* show mouse pointer */
  SDL_WarpMouse (dst.x + dst.w - 5, dst.y + dst.h - 5);
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
	  dst.x -= window.x;
	  dst.y -= window.y;
	  avt_resize (event.resize.w, event.resize.h);
	  dst.x += window.x;
	  dst.y += window.y;
	  break;

	case SDL_KEYDOWN:
	  nokey = AVT_FALSE;
	  if (event.key.keysym.sym == SDLK_ESCAPE)
	    _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  nokey = AVT_FALSE;
	  break;

	default:
	  break;
	}
    }

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

  /* delete button */
  SDL_SetClipRect (screen, &window);
  SDL_FillRect (screen, &dst,
		SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
			    backgroundcolor_RGB.g, backgroundcolor_RGB.b));
  SDL_UpdateRect (screen, dst.x, dst.y, dst.w, dst.h);

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return _avt_STATUS;
}


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
      colors[1].r = 0x00;
      colors[1].g = 0x00;
      colors[1].b = 0x00;
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
	  avt_drawchar (*m);
	  cursor.x += FONTWIDTH;
	  if (cursor.x > window.x + window.w + FONTWIDTH)
	    break;
	  m++;
	}

      SDL_UpdateRect (screen, dst.x, dst.y, dst.w, dst.h);
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

	case SDL_VIDEORESIZE:
	  dst.x -= window.x;
	  dst.y -= window.y;
	  avt_resize (event.resize.w, event.resize.h);
	  dst.x += window.x;
	  dst.y += window.y;
	  break;

	case SDL_KEYDOWN:
	  nokey = AVT_FALSE;
	  if (event.key.keysym.sym == SDLK_ESCAPE)
	    _avt_STATUS = AVT_QUIT;
	  break;

	case SDL_MOUSEBUTTONDOWN:
	  nokey = AVT_FALSE;
	  break;
	}
    }

  /* hide mouse pointer */
  SDL_ShowCursor (SDL_DISABLE);

  /* clear message */
  if (*message)
    {
      SDL_SetClipRect (screen, &window);
      SDL_FillRect (screen, &dst,
		    SDL_MapRGB (screen->format, backgroundcolor_RGB.r,
				backgroundcolor_RGB.g,
				backgroundcolor_RGB.b));
      SDL_UpdateRect (screen, dst.x, dst.y, dst.w, dst.h);
    }

  if (textfield.x >= 0)
    SDL_SetClipRect (screen, &viewport);

  return _avt_STATUS;
}

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

/* free avt_image_t images */
void
avt_free_image (avt_image_t * image)
{
  SDL_FreeSurface ((SDL_Surface *) image);
}

/*
 * try to load the library SDL_image dynamically 
 * (uncompressed BMP files can always be loaded)
 */
static void
load_SDL_image (void)
{
/* loadso.h is only available with SDL 1.2.6 or higher */
#ifdef _SDL_loadso_h
  if (!tried_to_load_SDL_image)	/* avoid loading it twice! */
    {
      SDL_image_handle = SDL_LoadObject (SDL_IMAGE_LIB);
      if (SDL_image_handle)
	{
	  IMG_Load =
	    (SDL_Surface * (*)(const char *))
	    SDL_LoadFunction (SDL_image_handle, "IMG_Load");
	  IMG_Load_RW =
	    (SDL_Surface * (*)(SDL_RWops *, int))
	    SDL_LoadFunction (SDL_image_handle, "IMG_Load_RW");
	}
    }
#endif /* _SDL_loadso_h */

  /* don't try to load it again - even if loading failed */
  tried_to_load_SDL_image = AVT_TRUE;
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
  SDL_UpdateRect (screen, 0, 0, 0, 0);
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

  if (!tried_to_load_SDL_image)
    load_SDL_image ();

  /* try to load image with IMG_Load or SDL_LoadBMP */
  /* (SDL_LoadBMP isn't a function but a macro) */
  if (IMG_Load)
    image = (*IMG_Load) (file);
  else
    image = SDL_LoadBMP (file);

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

  if (!tried_to_load_SDL_image)
    load_SDL_image ();

  /* try to load image with IMG_Load_RW or SDL_LoadBMP_RW */
  if (IMG_Load)
    image = (*IMG_Load_RW) (SDL_RWFromMem (img, imgsize), 1);
  else
    image = SDL_LoadBMP_RW (SDL_RWFromMem (img, imgsize), 1);

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
       * needed for the fbcon driver */
      SDL_putenv ("SDL_NOMOUSE=1");

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

  if (!tried_to_load_SDL_image)
    load_SDL_image ();

  /* try to load image with IMG_Load_RW or SDL_LoadBMP_RW */
  if (IMG_Load)
    image = (*IMG_Load_RW) (SDL_RWFromMem (img, imgsize), 1);
  else
    image = SDL_LoadBMP_RW (SDL_RWFromMem (img, imgsize), 1);

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

  if (!tried_to_load_SDL_image)
    load_SDL_image ();

  /* try to load image with IMG_Load or SDL_LoadBMP */
  if (IMG_Load)
    image = (*IMG_Load) (file);
  else
    image = SDL_LoadBMP (file);

  /* if it's not yet transparent, make it transparent */
  if (image)
    if (!(image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
      avt_make_transparent (image);

  return (avt_image_t *) image;
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
      if (textfield.x >= 0)
	{
	  avt_visible = AVT_FALSE;
	  avt_draw_balloon ();
	}
      else if (avt_visible)
	avt_show_avatar ();
      else
	avt_clear_screen ();
    }
}

void
avt_stop_on_esc (avt_bool_t stop)
{
  do_stop_on_esc = (stop != 0);
}

void
avt_register_keyhandler (avt_keyhandler handler)
{
  avt_ext_keyhandler = handler;
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

void
avt_quit (void)
{
  if (avt_quit_audio_func)
    (*avt_quit_audio_func) ();

  if (SDL_image_handle)
    {
#ifdef _SDL_loadso_h
      SDL_UnloadObject (SDL_image_handle);
#endif /* _SDL_loadso_h */
      SDL_image_handle = NULL;
      IMG_Load = NULL;
      tried_to_load_SDL_image = AVT_FALSE;	/* try again next time */
    }

  /* close conversion descriptors */
  if (output_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (output_cd);
  if (input_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (input_cd);
  output_cd = input_cd = ICONV_UNINITIALIZED;

  SDL_FreeSurface (avt_character);
  avt_character = NULL;
  SDL_FreeSurface (avt_image);
  avt_image = NULL;
  avt_bell_func = NULL;
  SDL_Quit ();
  screen = NULL;		/* it was freed by SDL_Quit */
  avt_visible = AVT_FALSE;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;
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
  do_stop_on_esc = AVT_TRUE;
  scroll_mode = 1;
  avt_visible = AVT_FALSE;
  textfield.x = textfield.y = textfield.w = textfield.h = -1;
  viewport = textfield;
  textdir_rtl = AVT_LEFT_TO_RIGHT;
  text_delay = AVT_DEFAULT_TEXT_DELAY;
  flip_page_delay = AVT_DEFAULT_FLIP_PAGE_DELAY;

  if (avt_init_SDL ())
    {
      avt_free_image (image);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  SDL_SetError ("$Id: avatar.c,v 2.63 2008-01-11 09:47:29 akf Exp $");
  SDL_ClearError ();
  SDL_WM_SetCaption (title, icontitle);
  avt_register_icon ();

  /*
   * Initialize the display, accept any format
   */
  screenflags = SDL_SWSURFACE | SDL_ANYFORMAT | SDL_RESIZABLE;

  /* FIXME: why doesn't that work under Windows? */
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

  /* must the screen be locked? */
  must_lock = SDL_MUSTLOCK (screen);

  /* window may be smaller than the screen */
  window.w = MINIMALWIDTH;
  window.h = MINIMALHEIGHT;
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  /*
   * hide mouse cursor
   * (must be after SDL_SetVideoMode)
   */
  SDL_ShowCursor (SDL_DISABLE);

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

  /* set color table for character canvas */
  {
    SDL_Color colors[2];

    /* white background */
    colors[0].r = 0xFF;
    colors[0].g = 0xFF;
    colors[0].b = 0xFF;
    /* black foreground */
    colors[1].r = 0x00;
    colors[1].g = 0x00;
    colors[1].b = 0x00;
    SDL_SetColors (avt_character, colors, 0, 2);
  }

  /* import the avatar image */
  if (image)
    {
      /* convert image to display-format for faster drawing */
      if (((SDL_Surface *) image)->flags & SDL_SRCALPHA)
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

  /*
   * calculate balloonheight from window height, image height,
   * and AVATAR_MARGIN
   */
  if (avt_image)
    {
      balloonheight = window.h - avt_image->h - 2 * TOPMARGIN - AVATAR_MARGIN;
      /* align with LINEHEIGHT */
      balloonheight -=
	(balloonheight - (2 * BALLOON_INNER_MARGIN)) % LINEHEIGHT;

      /* check, whether image is too high */
      if (balloonheight < (3 * LINEHEIGHT) + (2 * BALLOON_INNER_MARGIN))
	{
	  SDL_SetError ("Avatar image too large");
	  _avt_STATUS = AVT_ERROR;
	  SDL_FreeSurface (avt_image);
	  avt_image = NULL;
	  SDL_Quit ();
	  screen = NULL;
	  return _avt_STATUS;
	}
    }
  else				/* no avatar? -> whole screen is the balloon */
    {
      balloonheight = window.h - 2 * TOPMARGIN;
    }

  /* needed to get the character of the typed key */
  SDL_EnableUNICODE (1);

  /* ignore what we don't use */
  SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState (SDL_MOUSEBUTTONUP, SDL_IGNORE);
  SDL_EventState (SDL_KEYUP, SDL_IGNORE);

  /* visual flash for the bell */
  /* when you initialize the audio stuff, you get an audio "bell" */
  avt_bell_func = avt_flash;

  return _avt_STATUS;
}
