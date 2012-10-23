/*
 * AKFAvatar SDL backend
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99
 *
 * other software:
 * required:
 *  SDL1.2.11 or later (but not 1.3!)
 * optional (deprecated):
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

#define inline

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "SDL.h"

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <iso646.h>
#include <string.h>
#include <strings.h>

// include images
#include "akfavatar.xpm"
#include "mpointer.xbm"
#include "mpointer_mask.xbm"

#ifdef LINK_SDL_IMAGE
#  include "SDL_image.h"
#endif

#define LINEHEIGHT (fontheight)	// + something, if you want

// Note errno is only used for iconv and may not be the external errno!

#ifndef USE_SDL_ICONV
#  include <errno.h>
#  include <iconv.h>
#  define avt_iconv_t             iconv_t
#  define avt_iconv_open          iconv_open
#  define avt_iconv_close         iconv_close
#  define avt_iconv               iconv
#else // USE_SDL_ICONV
static int errno;
#  define avt_iconv_t             SDL_iconv_t
#  define avt_iconv_open          SDL_iconv_open
#  define avt_iconv_close         SDL_iconv_close
   // avt_iconv implemented below
#endif // USE_SDL_ICONV

#define COLORDEPTH  (CHAR_BIT * sizeof (avt_color))

#if defined(VGA)
#  define MINIMALWIDTH 640
#  define MINIMALHEIGHT 480
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 10
#  define AVATAR_MARGIN 10
   // Delay for moving in or out - the higher, the slower
#  define MOVE_DELAY 2.5
#else
#  define MINIMALWIDTH 800
#  define MINIMALHEIGHT 600
#  define TOPMARGIN 25
#  define BALLOON_INNER_MARGIN 15
#  define AVATAR_MARGIN 20
   // Delay for moving in or out - the higher, the slower
#  define MOVE_DELAY 1.8
#endif // not VGA

// only defined in later SDL versions
#ifndef SDL_BUTTON_WHEELUP
#  define SDL_BUTTON_WHEELUP 4
#endif

#ifndef SDL_BUTTON_WHEELDOWN
#  define SDL_BUTTON_WHEELDOWN 5
#endif

// Note: any event should have an associated key, so key == event

#define AVT_KEYBUFFER_SIZE  512

struct avt_key_buffer
{
  unsigned short int position, end;
  avt_char buffer[AVT_KEYBUFFER_SIZE];
};

static SDL_Surface *sdl_screen;
static struct avt_settings *avt;
static short int mode;		// whether fullscreen or window or ...
static SDL_Cursor *mpointer;
static struct avt_area windowmode_size;	// size of the whole window (screen)
static uint32_t screenflags;	// flags for the screen
static int fontwidth, fontheight;
static struct avt_key_buffer avt_keys;

// forward declaration
static int avt_pause (void);
static void avt_analyze_event (SDL_Event * event);
//-----------------------------------------------------------------------------


// use data for pixels
// data may olny be freed after avt_free_graphic is called on this
static avt_graphic *
avt_data_to_graphic (void *data, short width, short height)
{
  avt_graphic *gr;

  gr = (avt_graphic *) malloc (sizeof (*gr));

  if (gr)
    {
      gr->width = width;
      gr->height = height;
      gr->transparent = false;
      gr->free_pixels = false;
      gr->color_key = 0xFFFFFFFF;
      gr->pixels = (avt_color *) data;
    }

  return gr;
}

static avt_graphic *
avt_new_graphic (short width, short height)
{
  avt_graphic *gr;

  gr = avt_data_to_graphic (NULL, width, height);

  if (gr)
    {
      gr->pixels = (avt_color *) malloc (width * height * sizeof (avt_color));

      if (not gr->pixels)
	{
	  avt_free_graphic (gr);
	  return NULL;
	}

      gr->free_pixels = true;
    }

  return gr;
}

// import an SDL_Surface into the internal format
// This is ugly!
static avt_graphic *
avt_import_sdl_surface (SDL_Surface * s)
{
  avt_graphic *gr;
  SDL_Surface *d;

  d = SDL_DisplayFormat (s);	// TODO: use SDL_PixelFormat
  gr = avt_new_graphic (d->w, d->h);
  if (gr)
    memcpy (gr->pixels, d->pixels, d->w * d->h * sizeof (avt_color));

  SDL_FreeSurface (d);
  // s is freed by the caller

  return gr;
}

// this shall be the only function to update the window/screen
// all values as 0 shall update everything
extern void
avt_update_area (int x, int y, int width, int height)
{
  // sdl_screen has the pixel-information of screen
  // other implementations might need to copy it
  SDL_UpdateRect (sdl_screen, x, y, width, height);
}

// for dynamically loading SDL_image
#ifndef AVT_SDL_IMAGE_LIB
#  if defined (__WIN32__)
#    define AVT_SDL_IMAGE_LIB "SDL_image.dll"
#  else	// not Windows
#    define AVT_SDL_IMAGE_LIB "libSDL_image-1.2.so.0"
#  endif // not Windows
#endif // not AVT_SDL_IMAGE_LIB

/*
 * object for image-loading
 * SDL_image can be dynamically loaded (SDL-1.2.6 or better)
 */
static struct
{
  bool initialized;
  void *handle;			// handle for dynamically loaded SDL_image
  SDL_Surface *(*rw) (SDL_RWops * src, int freesrc);
} load_image;


#ifdef LINK_SDL_IMAGE

/*
 * assign functions from linked SDL_image
 */
static void
load_image_initialize (void)
{
  if (not load_image.initialized)
    {
      load_image.handle = NULL;
      load_image.rw = IMG_Load_RW;

      load_image.initialized = true;
    }
}

// speedup
static inline void
load_image_init (void)
{
  if (not load_image.initialized)
    load_image_initialize ();
}

#define load_image_done(void)	// empty

#else // not LINK_SDL_IMAGE

// helper functions

static SDL_Surface *
avt_load_image_RW (SDL_RWops * src, int freesrc)
{
  SDL_Surface *image;

  image = NULL;

  if (src)
    {
      image = SDL_LoadBMP_RW (src, 0);

      if (freesrc)
	SDL_RWclose (src);
    }

  return image;
}

/*
 * try to load the library SDL_image dynamically
 * (XPM and uncompressed BMP files can always be loaded)
 */
static void
load_image_initialize (void)
{
  if (not load_image.initialized)	// avoid loading it twice!
    {
      // first load defaults from plain SDL
      load_image.handle = NULL;
      load_image.rw = avt_load_image_RW;

#ifndef NO_SDL_IMAGE
// loadso.h is only available with SDL 1.2.6 or higher
#ifdef _SDL_loadso_h
      load_image.handle = SDL_LoadObject (AVT_SDL_IMAGE_LIB);
      if (load_image.handle)
	{
	  load_image.rw =
	    (SDL_Surface * (*)(SDL_RWops *, int))
	    SDL_LoadFunction (load_image.handle, "IMG_Load_RW");
	}
#endif // _SDL_loadso_h
#endif // NO_SDL_IMAGE

      load_image.initialized = true;
    }
}

// speedup
static inline void
load_image_init (void)
{
  if (not load_image.initialized)
    load_image_initialize ();
}

#ifndef _SDL_loadso_h
#  define load_image_done(void)	// empty
#else // _SDL_loadso_h
static void
load_image_done (void)
{
  if (load_image.handle)
    {
      SDL_UnloadObject (load_image.handle);
      load_image.handle = NULL;
      load_image.rw = NULL;
      load_image.initialized = false;	// try again next time
    }
}
#endif // _SDL_loadso_h

#endif // not LINK_SDL_IMAGE


// sdl image loaders

static inline avt_graphic *
avt_load_image_rw (SDL_RWops * RW)
{
  SDL_Surface *image;
  avt_graphic *result;

  if (not RW)
    return NULL;

  result = NULL;

  load_image_init ();
  image = load_image.rw (RW, 0);

  SDL_RWclose (RW);

  if (image)
    {
      result = avt_import_sdl_surface (image);
      SDL_FreeSurface (image);
    }

  return result;
}

extern avt_graphic *
avt_load_image_file_sdl (const char *filename)
{
  return avt_load_image_rw (SDL_RWFromFile (filename, "rb"));
}

extern avt_graphic *
avt_load_image_stream_sdl (avt_stream * stream)
{
  return avt_load_image_rw (SDL_RWFromFP ((FILE *) stream, 0));
}

extern avt_graphic *
avt_load_image_memory_sdl (void *data, size_t size)
{
  return avt_load_image_rw (SDL_RWFromMem (data, size));
}

// TODO: get it working again
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
      errno = EBADMSG;		// ???
      r = (size_t) (-1);
      break;
    }

  return r;
}
#endif // USE_SDL_ICONV

// FIXME
static void
avt_resize_sdl (int w, int h)
{
  avt_graphic *oldwindowimage;
  struct avt_area oldwindow;
  SDL_Event event;

  // minimal size
  if (w < MINIMALWIDTH)
    w = MINIMALWIDTH;
  if (h < MINIMALHEIGHT)
    h = MINIMALHEIGHT;

  // save the window
  oldwindow = window;
  oldwindowimage = avt_get_window ();

  // resize screen
  sdl_screen = SDL_SetVideoMode (w, h, COLORDEPTH, screenflags);
  screen->pixels = sdl_screen->pixels;
  screen->width = sdl_screen->w;
  screen->height = sdl_screen->h;

  avt_free_screen ();

  // new position of the window on the screen
  if (screen->width > window.width)
    window.x = (screen->width / 2) - (window.width / 2);
  else
    window.x = 0;

  if (screen->height > window.height)
    window.y = (screen->height / 2) - (window.height / 2);
  else
    window.y = 0;

  // restore image
  avt_put_graphic (oldwindowimage, screen, window.x, window.y);
  avt_free_graphic (oldwindowimage);

  // recalculate textfield & viewport positions
  if (avt->textfield.x >= 0)
    {
      avt->textfield.x = avt->textfield.x - oldwindow.x + window.x;
      avt->textfield.y = avt->textfield.y - oldwindow.y + window.y;

      avt->viewport.x = avt->viewport.x - oldwindow.x + window.x;
      avt->viewport.y = avt->viewport.y - oldwindow.y + window.y;

      if (avt->textdir_rtl)
	avt->linestart = avt->viewport.x + avt->viewport.width - fontwidth;
      else
	avt->linestart = avt->viewport.x;

      avt->cursor.x = avt->cursor.x - oldwindow.x + window.x;
      avt->cursor.y = avt->cursor.y - oldwindow.y + window.y;
    }

  // set windowmode_size
  if ((screenflags & SDL_FULLSCREEN) == 0)
    {
      windowmode_size.width = w;
      windowmode_size.height = h;
    }

  // make all changes visible
  avt_update_all ();

  // ignore one resize event here to avoid recursive calling
  while (SDL_PollEvent (&event) and event.type != SDL_VIDEORESIZE)
    avt_analyze_event (&event);
}

extern void
avt_toggle_fullscreen (void)
{
  if (mode != AVT_FULLSCREENNOSWITCH)
    {
      // toggle bit for fullscreenmode
      screenflags = screenflags xor SDL_FULLSCREEN;

      if ((screenflags bitand SDL_FULLSCREEN) != 0)
	{
	  screenflags = screenflags bitor SDL_NOFRAME;
	  avt_resize_sdl (window.width, window.height);
	  mode = AVT_FULLSCREEN;
	}
      else
	{
	  screenflags = screenflags bitand compl SDL_NOFRAME;
	  avt_resize_sdl (windowmode_size.width, windowmode_size.height);
	  mode = AVT_WINDOW;
	}
    }
}

// switch to fullscreen or window mode
extern void
avt_switch_mode (int new_mode)
{
  if (screen and new_mode != mode)
    {
      mode = new_mode;
      switch (mode)
	{
	case AVT_FULLSCREENNOSWITCH:
	case AVT_FULLSCREEN:
	  if ((screenflags bitand SDL_FULLSCREEN) == 0)
	    {
	      screenflags =
		screenflags bitor SDL_FULLSCREEN bitor SDL_NOFRAME;
	      avt_resize_sdl (window.width, window.height);
	    }
	  break;

	case AVT_WINDOW:
	  if ((screenflags bitand SDL_FULLSCREEN) != 0)
	    {
	      screenflags =
		screenflags bitand compl (SDL_FULLSCREEN bitor SDL_NOFRAME);
	      avt_resize_sdl (windowmode_size.width, windowmode_size.height);
	    }
	  break;
	}
    }
}

// push key into buffer
extern void
avt_push_key (avt_char key)
{
  SDL_Event event;
  int new_end;

  new_end = (avt_keys.end + 1) % AVT_KEYBUFFER_SIZE;

  // if buffer is not full
  if (new_end != avt_keys.position)
    {
      avt_keys.buffer[avt_keys.end] = key;
      avt_keys.end = new_end;

      /*
       * Send some event to satisfy avt_wait_key,
       * but no keyboard event to avoid endless loops!
       * Pushing a key also should have no side effects!
       */
      event.type = SDL_USEREVENT;
      event.user.code = AVT_PUSH_KEY;
      SDL_PushEvent (&event);
    }
}

static inline void
avt_analyze_key (SDL_keysym key)
{
  // return immediately to avoid the external key handler

  switch (key.sym)
    {
    case SDLK_PAUSE:
      avt_pause ();
      break;

    case SDLK_ESCAPE:
      if (avt->reserve_single_keys)
	avt_push_key (AVT_KEY_ESCAPE);
      else
	{
	  _avt_STATUS = AVT_QUIT;
	  return;
	}
      break;

    case SDLK_q:
      if (key.mod & KMOD_LALT)
	{
	  _avt_STATUS = AVT_QUIT;
	  return;
	}
      else
	{
	  if (key.unicode)
	    avt_push_key (key.unicode);
	}
      break;

    case SDLK_F11:
      if (avt->reserve_single_keys)
	avt_push_key (AVT_KEY_F11);
      else
	{
	  avt_toggle_fullscreen ();
	  return;
	}
      break;

    case SDLK_RETURN:
      if (key.mod & KMOD_LALT)
	{
	  avt_toggle_fullscreen ();
	  return;
	}
      else
	avt_push_key (AVT_KEY_ENTER);
      break;

    case SDLK_f:
      if ((key.mod & KMOD_CTRL) and (key.mod & KMOD_LALT))
	{
	  avt_toggle_fullscreen ();
	  return;
	}
      else
	{
	  if (key.unicode)
	    avt_push_key (key.unicode);
	}
      break;

    case SDLK_UP:
    case SDLK_KP8:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_UP);
      break;

    case SDLK_DOWN:
    case SDLK_KP2:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_DOWN);
      break;

    case SDLK_RIGHT:
    case SDLK_KP6:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_RIGHT);
      break;

    case SDLK_LEFT:
    case SDLK_KP4:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_LEFT);
      break;

    case SDLK_INSERT:
    case SDLK_KP0:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_INSERT);
      break;

    case SDLK_DELETE:
    case SDLK_KP_PERIOD:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_DELETE);
      break;

    case SDLK_HOME:
    case SDLK_KP7:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_HOME);
      break;

    case SDLK_END:
    case SDLK_KP1:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_END);
      break;

    case SDLK_PAGEUP:
    case SDLK_KP9:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_PAGEUP);
      break;

    case SDLK_PAGEDOWN:
    case SDLK_KP3:
      if (key.unicode)
	avt_push_key (key.unicode);
      else
	avt_push_key (AVT_KEY_PAGEDOWN);
      break;

    case SDLK_BACKSPACE:
      avt_push_key (AVT_KEY_BACKSPACE);
      break;

    case SDLK_HELP:
      avt_push_key (AVT_KEY_HELP);
      break;

    case SDLK_MENU:
      avt_push_key (AVT_KEY_MENU);
      break;

    case SDLK_EURO:
      avt_push_key (0x20AC);
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
      avt_push_key (AVT_KEY_F1 + (key.sym - SDLK_F1));
      break;

    default:
      if (key.unicode)
	avt_push_key (key.unicode);
      break;
    }				// switch (key.sym)

  if (avt->ext_keyhandler)
    avt->ext_keyhandler (key.sym, key.mod, key.unicode);
}

static void
avt_call_mouse_handler (SDL_Event * event)
{
  int x, y;

  if (avt->textfield.x >= 0)
    {
      // if there is a textfield, use the character position
      x = (event->button.x - avt->textfield.x) / fontwidth + 1;
      y = (event->button.y - avt->textfield.y) / LINEHEIGHT + 1;

      // check if x and y are valid
      if (x >= 1 and x <= AVT_LINELENGTH
	  and y >= 1 and y <= (avt->textfield.height / LINEHEIGHT))
	avt->ext_mousehandler (event->button.button,
			       (event->button.state == SDL_PRESSED), x, y);
    }
  else				// no textfield
    {
      x = event->button.x - window.x;
      y = event->button.y - window.y;
      if (x >= 0 and x <= window.width and y >= 0 and y <= window.height)
	avt->ext_mousehandler (event->button.button,
			       (event->button.state == SDL_PRESSED), x, y);
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
      avt_resize_sdl (event->resize.w, event->resize.h);
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (event->button.button <= 3)
	{
	  if (not avt_check_buttons (event->button.x, event->button.y))
	    {
	      if (avt->pointer_button_key)
		avt_push_key (avt->pointer_button_key);
	    }
	}
      else if (SDL_BUTTON_WHEELDOWN == event->button.button)
	avt_push_key (AVT_KEY_DOWN);
      else if (SDL_BUTTON_WHEELUP == event->button.button)
	avt_push_key (AVT_KEY_UP);

      if (avt->ext_mousehandler)
	avt_call_mouse_handler (event);
      break;

    case SDL_MOUSEBUTTONUP:
      if (avt->ext_mousehandler)
	avt_call_mouse_handler (event);
      break;

    case SDL_MOUSEMOTION:
      if (avt->pointer_motion_key)
	avt_push_key (avt->pointer_motion_key);
      break;

    case SDL_KEYDOWN:
      avt_analyze_key (event->key.keysym);
      break;
    }				// switch (event->type)
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
  while (pause and not _avt_STATUS);

  if (audio_initialized and not _avt_STATUS)
    SDL_PauseAudio (pause);

  return _avt_STATUS;
}

extern int
avt_checkevent (void)
{
  SDL_Event event;

  while (SDL_PollEvent (&event))
    avt_analyze_event (&event);

  return _avt_STATUS;
}

// send a timeout event
static uint32_t
avt_timeout (uint32_t intervall, void *param)
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
  if (screen and _avt_STATUS == AVT_NORMAL)
    {
      if (milliseconds <= 500)	// short delay
	{
	  SDL_Delay (milliseconds);
	  avt_checkevent ();
	}
      else			// longer
	{
	  SDL_Event event;
	  SDL_TimerID t;

	  t = SDL_AddTimer (milliseconds, avt_timeout, NULL);

	  if (not t)
	    {
	      // extremely unlikely error
	      avt_set_error ("AddTimer doesn't work");
	      _avt_STATUS = AVT_ERROR;
	      return _avt_STATUS;
	    }

	  while (_avt_STATUS == AVT_NORMAL)
	    {
	      SDL_WaitEvent (&event);
	      if (event.type == SDL_USEREVENT
		  and event.user.code == AVT_TIMEOUT)
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


static inline void
avt_wait_key (void)
{
  SDL_Event event;

  while (_avt_STATUS == AVT_NORMAL and avt_keys.position == avt_keys.end)
    {
      SDL_WaitEvent (&event);
      avt_analyze_event (&event);
    }
}

extern int
avt_key (avt_char * ch)
{
  avt_wait_key ();

  if (ch)
    *ch = avt_keys.buffer[avt_keys.position];

  avt_keys.position = (avt_keys.position + 1) % AVT_KEYBUFFER_SIZE;

  return _avt_STATUS;
}

extern bool
avt_key_pressed (void)
{
  return (avt_keys.position != avt_keys.end);
}

extern void
avt_clear_keys (void)
{
  avt_keys.position = avt_keys.end = 0;
}

extern avt_char
avt_set_pointer_motion_key (avt_char key)
{
  avt_char old;

  old = avt->pointer_motion_key;
  avt->pointer_motion_key = key;

  if (key)
    SDL_EventState (SDL_MOUSEMOTION, SDL_ENABLE);
  else
    SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);

  return old;
}

// key for pointer buttons 1-3
extern avt_char
avt_set_pointer_buttons_key (avt_char key)
{
  avt_char old;

  old = avt->pointer_button_key;
  avt->pointer_button_key = key;

  return old;
}

extern void
avt_get_pointer_position (int *x, int *y)
{
  SDL_GetMouseState (x, y);
}

extern void
avt_set_mouse_visible (bool visible)
{
  if (visible)
    SDL_ShowCursor (SDL_ENABLE);
  else
    SDL_ShowCursor (SDL_DISABLE);
}

extern int
avt_get_mode (void)
{
  return mode;
}

extern char *
avt_get_error (void)
{
  return SDL_GetError ();
}

extern void
avt_set_error (const char *message)
{
  SDL_SetError ("%s", message);
}

static int
avt_init_SDL (void)
{
  // only if not already initialized
  if (SDL_WasInit (SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0)
    {
      /* don't try to use the mouse
       * might be needed for the fbcon driver
       * the mouse still works, it is just not required
       */
      SDL_putenv ("SDL_NOMOUSE=1");

      avt_set_error ("15ce822f94d7e8e4281f1c2bcdd7c56d");

      if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	_avt_STATUS = AVT_ERROR;
    }

  return _avt_STATUS;
}


extern void
avt_quit (void)
{
  avt_quit_common ();
  load_image_done ();

  if (sdl_screen)
    {
      SDL_FreeCursor (mpointer);
      mpointer = NULL;
      SDL_Quit ();
      sdl_screen = NULL;	// it was freed by SDL_Quit
    }
}

extern void
avt_set_title (const char *title, const char *shortname)
{
  char *encoding;

  encoding = avt_get_mb_encoding ();

  // check if encoding was set
  if (not encoding)
    {
      avt_mb_encoding (MB_DEFAULT_ENCODING);
      encoding = avt_get_mb_encoding ();
    }

  // check if it's already in correct encoding default="UTF-8"
  if (strcasecmp ("UTF-8", encoding) == 0
      or strcasecmp ("UTF8", encoding) == 0
      or strcasecmp ("CP65001", encoding) == 0)
    SDL_WM_SetCaption (title, shortname);
  else				// convert them to UTF-8
    {
      char my_title[260];
      char my_shortname[84];

      if (title and * title)
	{
	  if (avt_recode_buffer ("UTF-8", encoding,
				 my_title, sizeof (my_title),
				 title, strlen (title)) == (size_t) (-1))
	    {
	      memcpy (my_title, title, sizeof (my_title));
	      my_title[sizeof (my_title) - 1] = '\0';
	    }
	}

      if (shortname and * shortname)
	{
	  if (avt_recode_buffer ("UTF-8", encoding,
				 my_shortname, sizeof (my_shortname),
				 shortname,
				 strlen (shortname)) == (size_t) (-1))
	    {
	      memcpy (my_shortname, shortname, sizeof (my_shortname));
	      my_shortname[sizeof (my_shortname) - 1] = '\0';
	    }
	}

      SDL_WM_SetCaption (my_title, my_shortname);
    }
}


#define reverse_byte(b) \
  (((b) & 0x80) >> 7 | \
   ((b) & 0x40) >> 5 | \
   ((b) & 0x20) >> 3 | \
   ((b) & 0x10) >> 1 | \
   ((b) & 0x08) << 1 | \
   ((b) & 0x04) << 3 | \
   ((b) & 0x02) << 5 | \
   ((b) & 0x01) << 7)

// FIXME
// width must be a multiple of CHAR_BIT
#define xbm_bytes(img)  ((img##_width / CHAR_BIT) * img##_height)

static inline void
avt_set_mouse_pointer (void)
{
  unsigned char mp[xbm_bytes (mpointer)];
  unsigned char mp_mask[xbm_bytes (mpointer_mask)];

  // we need the bytes reversed :-(

  for (int i = 0; i < xbm_bytes (mpointer); i++)
    {
      register unsigned char b = mpointer_bits[i];
      mp[i] = reverse_byte (b);
    }

  for (int i = 0; i < xbm_bytes (mpointer_mask); i++)
    {
      register unsigned char b = mpointer_mask_bits[i];
      mp_mask[i] = reverse_byte (b);
    }

  mpointer = SDL_CreateCursor (mp, mp_mask,
			       mpointer_width, mpointer_height,
			       mpointer_x_hot, mpointer_y_hot);

  SDL_SetCursor (mpointer);
}

static inline void
avt_set_icon (char **xpm)
{
  SDL_Surface *icon;
  avt_graphic *gr;

  gr = avt_load_image_xpm (xpm);
  icon = SDL_CreateRGBSurfaceFrom (gr->pixels,
				   gr->width, gr->height,
				   CHAR_BIT * sizeof (avt_color),
				   gr->width * sizeof (avt_color),
				   0x00FF0000, 0x0000FF00, 0x000000FF, 0);

  SDL_WM_SetIcon (icon, NULL);

  SDL_FreeSurface (icon);
  avt_free_graphic (gr);
}

extern int
avt_start (const char *title, const char *shortname, int window_mode)
{
  // already initialized?
  if (screen)
    {
      avt_set_error ("AKFAvatar already initialized");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  avt_get_font_dimensions (&fontwidth, &fontheight, NULL);

  mode = window_mode;

  if (avt_init_SDL ())
    {
      avt_set_error ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (not title)
    title = "AKFAvatar";

  if (not shortname)
    shortname = title;

  avt_set_title (title, shortname);
  avt_set_icon (akfavatar_xpm);

  // Initialize the display
  screenflags = SDL_SWSURFACE | SDL_RESIZABLE;

#ifndef __WIN32__
  if (mode == AVT_AUTOMODE)
    {
      SDL_Rect **modes;

      /*
       * if maximum fullscreen mode is exactly the minimal size,
       * then default to fullscreen, else default to window
       */
      modes = SDL_ListModes (NULL, screenflags | SDL_FULLSCREEN);
      if (modes != (SDL_Rect **) (0) and modes != (SDL_Rect **) (-1))
	if (modes[0]->w == MINIMALWIDTH and modes[0]->h == MINIMALHEIGHT)
	  screenflags |= SDL_FULLSCREEN | SDL_NOFRAME;
    }
#endif

  SDL_ClearError ();

  if (mode >= 1)
    screenflags |= SDL_FULLSCREEN | SDL_NOFRAME;

  if (mode == AVT_FULLSCREENNOSWITCH)
    {
      sdl_screen = SDL_SetVideoMode (0, 0, COLORDEPTH, screenflags);

      // fallback if 0,0 is not supported yet (before SDL-1.2.10)
      if (sdl_screen and (sdl_screen->w == 0 or sdl_screen->h == 0))
	sdl_screen =
	  SDL_SetVideoMode (MINIMALWIDTH, MINIMALHEIGHT, COLORDEPTH,
			    screenflags);
    }
  else
    sdl_screen =
      SDL_SetVideoMode (MINIMALWIDTH, MINIMALHEIGHT, COLORDEPTH, screenflags);

  if (not sdl_screen)
    {
      avt_set_error ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // assure we really get what we need
  if (SDL_MUSTLOCK (sdl_screen)
      or sdl_screen->format->BitsPerPixel != COLORDEPTH)
    {
      avt_set_error ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // set up a graphic with the same pixel data
  avt = avt_start_common (avt_data_to_graphic
			  (sdl_screen->pixels, sdl_screen->w, sdl_screen->h));

  if (not avt)
    {
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // size of the window (not to be confused with the variable window
  windowmode_size.x = windowmode_size.y = 0;	// unused
  windowmode_size.width = sdl_screen->w;
  windowmode_size.height = sdl_screen->h;

  avt_set_mouse_pointer ();

  // needed to get the character of the typed key
  SDL_EnableUNICODE (1);

  // key repeat mode
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  // ignore what we don't use
  SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState (SDL_KEYUP, SDL_IGNORE);

  return _avt_STATUS;
}
