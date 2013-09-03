/*
 * AKFAvatar SDL backend
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99
 *
 * other software:
 * required:
 *  SDL1.2.11 or later
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

#define _ISOC99_SOURCE
#define _XOPEN_SOURCE 600

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "SDL.h"

#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <iso646.h>

// include images
#include "akfavatar.xpm"
#include "mpointer.xbm"
#include "mpointer_mask.xbm"


#if SDL_VERSION_ATLEAST(1, 3, 0)
#define SDL2
#endif

// FIXME: just for porting
#ifndef SDL2
// undefine to deactivate imageloaders including SDL_Image
#define IMAGELOADERS
#endif


#if defined(LINK_SDL_IMAGE) and defined(IMAGELOADERS)
#  include "SDL_image.h"
#endif

#define COLORDEPTH  (CHAR_BIT * sizeof (avt_color))

#define AVT_TIMEOUT 1
#define AVT_PUSH_KEY 2

#ifndef SDL2
// only defined in later SDL versions
#ifndef SDL_BUTTON_WHEELUP
#  define SDL_BUTTON_WHEELUP 4
#endif

#ifndef SDL_BUTTON_WHEELDOWN
#  define SDL_BUTTON_WHEELDOWN 5
#endif
#endif // SDL1

#ifdef SDL2
static SDL_Window *sdl_window;
static SDL_Renderer *sdl_renderer;
static SDL_Texture *sdl_screen;
static const struct avt_charenc *utf8;
#else // SDL-1.2
static SDL_Surface *sdl_screen;
#endif

static SDL_Cursor *mpointer;

static Uint32 screenflags;	// flags for the screen

static short int mode;		// whether fullscreen or window or ...
static int windowmode_width, windowmode_height;

static bool reserve_single_keys;
static avt_char pointer_button_key;	// key simulated for mouse button 1-3
static avt_char pointer_motion_key;	// key simulated be pointer motion

// forward declaration
static int avt_pause (void);
static void avt_analyze_event (SDL_Event * event);
//-----------------------------------------------------------------------------


#ifdef SDL2

// this shall be the only function to update the window/screen
static void
update_area_sdl (avt_graphic * screen, int x, int y, int width, int height)
{
  SDL_Rect rect;
  int screen_width, screen_height;

  screen_width = screen->width;
  screen_height = screen->height;

  if (x < 0)
    {
      width -= (-x);
      x = 0;
    }

  if (x + width > screen_width)
    width = screen_width - x;

  if (y < 0)
    {
      height -= (-y);
      y = 0;
    }

  if (y + height > screen_height)
    height = screen_height - y;

  if (width <= 0 or height <= 0 or x > screen_width or y > screen_height)
    return;

  rect.x = x;
  rect.y = y;
  rect.w = width;
  rect.h = height;

  SDL_UpdateTexture (sdl_screen, &rect,
		     screen->pixels + (y * screen_width) + x,
		     MINIMALWIDTH * sizeof (avt_color));

  if (width < screen_width or height < screen_height)
    SDL_RenderCopy (sdl_renderer, sdl_screen, &rect, &rect);
  else				// update all
    {
      int c = avt_get_background_color ();
      SDL_SetRenderDrawColor (sdl_renderer, avt_red (c), avt_green (c),
			      avt_blue (c), SDL_ALPHA_OPAQUE);
      SDL_RenderClear (sdl_renderer);
      SDL_RenderCopy (sdl_renderer, sdl_screen, &rect, &rect);
    }

  SDL_RenderPresent (sdl_renderer);
}

#else // SDL-1.2

// this shall be the only function to update the window/screen
static void
update_area_sdl (avt_graphic * screen, int x, int y, int width, int height)
{
  (void) screen;

  // sdl_screen already has the pixel-information of screen
  // other implementations might need to copy pixels here
  SDL_UpdateRect (sdl_screen, x, y, width, height);
}

#endif // SDL-1.2

#ifdef IMAGELOADERS

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

#define load_image_done(void)	// empty

#else // not LINK_SDL_IMAGE

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
      load_image.rw = NULL;

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

#ifdef SDL2

// import an SDL_Surface into the internal format
static inline avt_graphic *
avt_import_sdl_surface (SDL_Surface * s)
{
  Uint32 flags;
  avt_graphic *gr;
  SDL_Surface *d;
  SDL_PixelFormat format = {
    SDL_PIXELFORMAT_ARGB8888,
    NULL,
    CHAR_BIT * sizeof (avt_color), sizeof (avt_color),
    {0, 0}
    ,
    0x00FF0000, 0x0000FF00, 0x000000FF, 0,
    0, 0, 0, 0,
    16, 8, 0, 0,
    0, NULL
  };

  flags = 0;			// unused

  // convert into the internally used pixel format
  d = SDL_ConvertSurface (s, &format, flags);

  gr = avt_new_graphic (d->w, d->h);
  if (gr)
    {
      gr->transparent = (SDL_GetColorKey (s, &gr->color_key) == 0);
      memcpy (gr->pixels, d->pixels, d->w * d->h * sizeof (avt_color));
    }

  SDL_FreeSurface (d);
  // s is freed by the caller

  return gr;
}

#else // SDL-1.2

// import an SDL_Surface into the internal format
static inline avt_graphic *
avt_import_sdl_surface (SDL_Surface * s)
{
  Uint32 flags;
  avt_graphic *gr;
  SDL_Surface *d;
  SDL_PixelFormat format = {
    NULL,
    CHAR_BIT * sizeof (avt_color), sizeof (avt_color),
    0, 0, 0, 0,
    16, 8, 0, 0,
    0x00FF0000, 0x0000FF00, 0x000000FF, 0,
    0, SDL_ALPHA_OPAQUE
  };

  flags = SDL_SWSURFACE bitor (s->flags bitand SDL_SRCCOLORKEY);

  // convert into the internally used pixel format
  d = SDL_ConvertSurface (s, &format, flags);

  gr = avt_new_graphic (d->w, d->h);
  if (gr)
    {
      gr->transparent = ((s->flags bitand SDL_SRCCOLORKEY) != 0);
      gr->color_key = s->format->colorkey;
      memcpy (gr->pixels, d->pixels, d->w * d->h * sizeof (avt_color));
    }

  SDL_FreeSurface (d);
  // s is freed by the caller

  return gr;
}
#endif // SDL-1.2

static inline avt_graphic *
avt_load_image_rw (SDL_RWops * RW)
{
  SDL_Surface *image;
  avt_graphic *result;

  if (not sdl_screen or not RW)
    return NULL;

  image = NULL;
  result = NULL;

  if (not load_image.initialized)
    load_image_initialize ();

  if (load_image.rw)
    image = load_image.rw (RW, 0);

  SDL_RWclose (RW);

  if (image)
    {
      result = avt_import_sdl_surface (image);
      SDL_FreeSurface (image);
    }

  return result;
}

static avt_graphic *
load_image_file_sdl (const char *filename)
{
  return avt_load_image_rw (SDL_RWFromFile (filename, "rb"));
}

static avt_graphic *
load_image_stream_sdl (avt_stream * stream)
{
  return avt_load_image_rw (SDL_RWFromFP ((FILE *) stream, 0));
}

static avt_graphic *
load_image_memory_sdl (void *data, size_t size)
{
  return avt_load_image_rw (SDL_RWFromMem (data, size));
}

#else // not IMAGELOADERS
#define load_image_done(void)	// empty
#endif // not IMAGELOADERS


#ifdef SDL2

extern void
avt_toggle_fullscreen (void)
{
  if (not sdl_screen)
    return;

  // toggle bit for fullscreenmode
  screenflags = screenflags xor SDL_WINDOW_FULLSCREEN_DESKTOP;

  SDL_SetWindowFullscreen (sdl_window, screenflags);

  if ((screenflags bitand SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
    mode = AVT_FULLSCREEN;
  else
    mode = AVT_WINDOW;
}

extern void
avt_switch_mode (int new_mode)
{
  if (sdl_screen and new_mode != mode)
    {
      mode = new_mode;
      switch (mode)
	{
	case AVT_FULLSCREENNOSWITCH:
	case AVT_FULLSCREEN:
	  if ((screenflags bitand SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)
	    {
	      screenflags = screenflags bitor SDL_WINDOW_FULLSCREEN_DESKTOP;
	      SDL_SetWindowFullscreen (sdl_window, screenflags);
	    }
	  break;

	case AVT_WINDOW:
	  if ((screenflags bitand SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
	    {
	      screenflags =
		screenflags bitand compl (SDL_WINDOW_FULLSCREEN_DESKTOP);
	      SDL_SetWindowFullscreen (sdl_window, screenflags);
	    }
	  break;
	}
    }
}

#else // SDL-1.2

static void
resize_sdl (avt_graphic * screen, int width, int height)
{
  SDL_Event event;

  // resize screen
  sdl_screen = SDL_SetVideoMode (width, height, COLORDEPTH, screenflags);
  screen->pixels = (avt_color *) sdl_screen->pixels;
  screen->width = sdl_screen->w;
  screen->height = sdl_screen->h;

  // set size of windowmode
  if ((screenflags & SDL_FULLSCREEN) == 0)
    {
      windowmode_width = width;
      windowmode_height = height;
    }

  // ignore one resize event here to avoid recursive calling
  while (SDL_PollEvent (&event) and event.type != SDL_VIDEORESIZE)
    avt_analyze_event (&event);
}

extern void
avt_toggle_fullscreen (void)
{
  if (not sdl_screen)
    return;

  if (mode != AVT_FULLSCREENNOSWITCH)
    {
      // toggle bit for fullscreenmode
      screenflags = screenflags xor SDL_FULLSCREEN;

      if ((screenflags bitand SDL_FULLSCREEN) != 0)
	{
	  screenflags = screenflags bitor SDL_NOFRAME;
	  avt_resize (MINIMALWIDTH, MINIMALHEIGHT);
	  mode = AVT_FULLSCREEN;
	}
      else
	{
	  screenflags = screenflags bitand compl SDL_NOFRAME;
	  avt_resize (windowmode_width, windowmode_height);
	  mode = AVT_WINDOW;
	}
    }
}

// switch to fullscreen or window mode
extern void
avt_switch_mode (int new_mode)
{
  if (sdl_screen and new_mode != mode)
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
	      avt_resize (MINIMALWIDTH, MINIMALHEIGHT);
	    }
	  break;

	case AVT_WINDOW:
	  if ((screenflags bitand SDL_FULLSCREEN) != 0)
	    {
	      screenflags =
		screenflags bitand compl (SDL_FULLSCREEN bitor SDL_NOFRAME);
	      avt_resize (windowmode_width, windowmode_height);
	    }
	  break;
	}
    }
}

#endif // SDL-1.2

static inline void
avt_analyze_key (Sint32 keycode, Uint16 mod)
{
  switch (keycode)
    {
    case SDLK_PAUSE:
      avt_pause ();
      break;

    case SDLK_ESCAPE:
      if (reserve_single_keys)
	avt_add_key (AVT_KEY_ESCAPE);
      else
	_avt_STATUS = AVT_QUIT;
      break;

    case SDLK_q:
      if (mod & KMOD_LALT)
	_avt_STATUS = AVT_QUIT;
      break;

    case SDLK_F11:
      if (reserve_single_keys)
	avt_add_key (AVT_KEY_F11);
      else
	avt_toggle_fullscreen ();
      break;

    case SDLK_RETURN:
      if (mod & KMOD_LALT)
	avt_toggle_fullscreen ();
      else
	avt_add_key (AVT_KEY_ENTER);
      break;

    case SDLK_f:
      if ((mod & KMOD_CTRL) and (mod & KMOD_LALT))
	avt_toggle_fullscreen ();
      break;

    case SDLK_UP:
      avt_add_key (AVT_KEY_UP);
      break;

    case SDLK_DOWN:
      avt_add_key (AVT_KEY_DOWN);
      break;

    case SDLK_RIGHT:
      avt_add_key (AVT_KEY_RIGHT);
      break;

    case SDLK_LEFT:
      avt_add_key (AVT_KEY_LEFT);
      break;

    case SDLK_INSERT:
      avt_add_key (AVT_KEY_INSERT);
      break;

    case SDLK_DELETE:
      avt_add_key (AVT_KEY_DELETE);
      break;

    case SDLK_HOME:
      avt_add_key (AVT_KEY_HOME);
      break;

    case SDLK_END:
      avt_add_key (AVT_KEY_END);
      break;

    case SDLK_PAGEUP:
      avt_add_key (AVT_KEY_PAGEUP);
      break;

    case SDLK_PAGEDOWN:
      avt_add_key (AVT_KEY_PAGEDOWN);
      break;

#ifndef SDL2

    case SDLK_KP0:
      avt_add_key (AVT_KEY_INSERT);
      break;

    case SDLK_KP1:
      avt_add_key (AVT_KEY_END);
      break;

    case SDLK_KP2:
      avt_add_key (AVT_KEY_DOWN);
      break;

    case SDLK_KP3:
      avt_add_key (AVT_KEY_PAGEDOWN);
      break;

    case SDLK_KP4:
      avt_add_key (AVT_KEY_LEFT);
      break;

    case SDLK_KP6:
      avt_add_key (AVT_KEY_RIGHT);
      break;

    case SDLK_KP7:
      avt_add_key (AVT_KEY_HOME);
      break;

    case SDLK_KP8:
      avt_add_key (AVT_KEY_UP);
      break;

    case SDLK_KP9:
      avt_add_key (AVT_KEY_PAGEUP);
      break;

    case SDLK_KP_PERIOD:
      avt_add_key (AVT_KEY_DELETE);
      break;
#endif

    case SDLK_BACKSPACE:
      avt_add_key (AVT_KEY_BACKSPACE);
      break;

    case SDLK_HELP:
      avt_add_key (AVT_KEY_HELP);
      break;

    case SDLK_MENU:
      avt_add_key (AVT_KEY_MENU);
      break;

#ifndef SDL2
    case SDLK_EURO:
      avt_add_key (0x20AC);
      break;
#endif

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
      avt_add_key (AVT_KEY_F1 + (keycode - SDLK_F1));
      break;
    }				// switch (key.sym)
}

static void
avt_analyze_event (SDL_Event * event)
{
  switch (event->type)
    {
    case SDL_QUIT:
      _avt_STATUS = AVT_QUIT;
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (event->button.button <= 3)
	{
	  if (not avt_check_buttons (event->button.x, event->button.y))
	    {
	      if (pointer_button_key)
		avt_add_key (pointer_button_key);
	    }
	}
#ifndef SDL2
      else if (SDL_BUTTON_WHEELDOWN == event->button.button)
	avt_add_key (AVT_KEY_DOWN);
      else if (SDL_BUTTON_WHEELUP == event->button.button)
	avt_add_key (AVT_KEY_UP);
#endif
      break;

    case SDL_MOUSEMOTION:
      if (pointer_motion_key and pointer_motion_key != avt_last_key ())
	avt_add_key (pointer_motion_key);
      break;

#ifdef SDL2
    case SDL_WINDOWEVENT:
      switch (event->window.event)
	{
	case SDL_WINDOWEVENT_SHOWN:
	case SDL_WINDOWEVENT_EXPOSED:
	case SDL_WINDOWEVENT_SIZE_CHANGED:
	  avt_update_all ();	// redraw
	  break;
	}
      break;

    case SDL_KEYDOWN:
      avt_analyze_key (event->key.keysym.sym, event->key.keysym.mod);
      break;

    case SDL_TEXTINPUT:
      {
	char *text = event->text.text;
	while (*text)
	  {
	    avt_char ch;
	    size_t s = utf8->decode (utf8, &ch, text);
	    avt_add_key (ch);
	    text += s;
	  }
      }
      break;

    case SDL_MOUSEWHEEL:
      if (event->wheel.y < 0)
	for (int i = -(event->wheel.y); i > 0; --i)
	  avt_add_key (AVT_KEY_DOWN);
      else if (event->wheel.y > 0)
	for (int i = event->wheel.y; i > 0; --i)
	  avt_add_key (AVT_KEY_UP);

      if (event->wheel.x < 0)
	for (int i = -(event->wheel.x); i > 0; --i)
	  avt_add_key (AVT_KEY_LEFT);
      else if (event->wheel.x > 0)
	for (int i = event->wheel.x; i > 0; --i)
	  avt_add_key (AVT_KEY_RIGHT);
      break;

#else // SDL-1.2

    case SDL_VIDEORESIZE:
      avt_resize (event->resize.w, event->resize.h);
      break;

#define ESC  (27)

    case SDL_KEYDOWN:
      if (event->key.keysym.unicode
	  and event->key.keysym.unicode != ESC
	  and not (event->key.keysym.mod bitand KMOD_LALT))
	avt_add_key (event->key.keysym.unicode);
      else
	avt_analyze_key (event->key.keysym.sym, event->key.keysym.mod);
      break;
#endif
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
avt_update (void)
{
  SDL_Event event;

  if (sdl_screen)
    {
      while (SDL_PollEvent (&event))
	avt_analyze_event (&event);
    }

  return _avt_STATUS;
}

// send a timeout event
static Uint32
avt_timeout (Uint32 intervall, void *param)
{
  SDL_Event event;

  (void) intervall;
  (void) param;

  event.type = SDL_USEREVENT;
  event.user.code = AVT_TIMEOUT;
  event.user.data1 = event.user.data2 = NULL;
  SDL_PushEvent (&event);

  return 0;
}

extern int
avt_wait (size_t milliseconds)
{
  // for times longer than half a second it should check for events

  if (sdl_screen and _avt_STATUS == AVT_NORMAL)
    {
      if (milliseconds <= 500)	// short delay
	{
	  if (_avt_STATUS == AVT_NORMAL)
	    avt_delay (milliseconds);

	  avt_update ();
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

extern void
avt_push_key (avt_char key)
{
  SDL_Event event;

  avt_add_key (key);

  /*
   * Send some event to satisfy wait_key,
   * but no keyboard event to avoid endless loops!
   * Pushing a key also should have no side effects!
   */
  event.type = SDL_USEREVENT;
  event.user.code = AVT_PUSH_KEY;
  SDL_PushEvent (&event);
}


static void
wait_key_sdl (void)
{
  SDL_Event event;

  if (sdl_screen)
    {
      while (_avt_STATUS == AVT_NORMAL and not avt_key_pressed ())
	{
	  SDL_WaitEvent (&event);
	  avt_analyze_event (&event);
	}
    }
}

extern void
avt_reserve_single_keys (bool onoff)
{
  reserve_single_keys = onoff;
}

extern avt_char
avt_set_pointer_motion_key (avt_char key)
{
  avt_char old;

  old = pointer_motion_key;
  pointer_motion_key = key;

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

  old = pointer_button_key;
  pointer_button_key = key;

  return old;
}

extern void
avt_get_pointer_position (int *x, int *y)
{
  int mx, my;

  mx = my = 0;

  if (sdl_screen)
    {
#ifdef SDL2
      // compute scale
      int wx, wy;
      SDL_GetWindowSize (sdl_window, &wx, &wy);
      SDL_GetMouseState (&mx, &my);

      mx = mx * MINIMALWIDTH / wx;
      my = my * MINIMALHEIGHT / wy;
#else
      SDL_GetMouseState (&mx, &my);
#endif
    }

  if (x)
    *x = mx;

  if (y)
    *y = my;
}

extern void
avt_set_mouse_visible (bool visible)
{
  if (sdl_screen)
    {
      if (visible)
	SDL_ShowCursor (SDL_ENABLE);
      else
	SDL_ShowCursor (SDL_DISABLE);
    }
}

extern int
avt_get_mode (void)
{
  return mode;
}

extern char *
avt_get_error (void)
{
  return (char *) SDL_GetError ();
}

extern void
avt_set_error (const char *message)
{
  if (message and * message)
    SDL_SetError ("%s", message);
  else
    SDL_ClearError ();
}

static int
avt_init_SDL (void)
{
  // only if not already initialized
  if (SDL_WasInit (SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0)
    {
#ifndef SDL2
      /* don't try to use the mouse
       * might be needed for the fbcon driver
       * the mouse still works, it is just not required
       */
      SDL_putenv ("SDL_NOMOUSE=1");
#endif

      if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	_avt_STATUS = AVT_ERROR;
    }

  return _avt_STATUS;
}


static void
quit_sdl (void)
{
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
#ifdef SDL2
  SDL_SetWindowTitle (sdl_window, title);
#else
  // assumes UTF-8
  SDL_WM_SetCaption (title, shortname);
#endif
}

static inline void
reverse_bits (unsigned char *bytes, size_t length)
{
  while (length--)
    {
      register unsigned char b = *bytes;

      *bytes = (b bitand 0x80) >> 7
	bitor (b bitand 0x40) >> 5
	bitor (b bitand 0x20) >> 3
	bitor (b bitand 0x10) >> 1
	bitor (b bitand 0x08) << 1
	bitor (b bitand 0x04) << 3
	bitor (b bitand 0x02) << 5 bitor (b bitand 0x01) << 7;

      ++bytes;
    }
}

static inline void
avt_set_mouse_pointer (void)
{
  // we need the bits reversed :-(
  reverse_bits (mpointer_bits, sizeof (mpointer_bits));
  reverse_bits (mpointer_mask_bits, sizeof (mpointer_mask_bits));

  mpointer = SDL_CreateCursor (mpointer_bits, mpointer_mask_bits,
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

#ifdef SDL2

  if (gr->transparent)
    SDL_SetColorKey (icon, SDL_TRUE, gr->color_key);

  SDL_SetWindowIcon (sdl_window, icon);

#else // SDL-1.2

  if (gr->transparent)
    SDL_SetColorKey (icon, SDL_SRCCOLORKEY, gr->color_key);

  SDL_WM_SetIcon (icon, NULL);

#endif

  SDL_FreeSurface (icon);
  avt_free_graphic (gr);
}

extern int
avt_start (const char *title, const char *shortname, int window_mode)
{
  struct avt_backend *backend;

  // already initialized?
  if (sdl_screen)
    {
      avt_set_error ("AKFAvatar already initialized");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

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

#ifdef SDL2
  screenflags = SDL_WINDOW_RESIZABLE;

  if (mode >= 1)
    screenflags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

  sdl_window =
    SDL_CreateWindow (title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		      MINIMALWIDTH, MINIMALHEIGHT, screenflags);

  if (not sdl_window)
    {
      avt_set_error ("error creating window");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  avt_set_icon (akfavatar_xpm);

  SDL_SetWindowMinimumSize (sdl_window, MINIMALWIDTH, MINIMALHEIGHT);

  sdl_renderer = SDL_CreateRenderer (sdl_window, -1, 0);

  if (not sdl_renderer)
    {
      avt_set_error ("error creating renderer");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize (sdl_renderer, MINIMALWIDTH, MINIMALHEIGHT);

  sdl_screen = SDL_CreateTexture (sdl_renderer, SDL_PIXELFORMAT_ARGB8888,
				  SDL_TEXTUREACCESS_STREAMING,
				  MINIMALWIDTH, MINIMALHEIGHT);

  if (not sdl_screen)
    {
      avt_set_error ("error creating screen texture");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // set up a new graphic to draw to
  backend = avt_start_common (avt_new_graphic (MINIMALWIDTH, MINIMALHEIGHT));

  // size of the window (not to be confused with the variable window)
  windowmode_width = MINIMALWIDTH;
  windowmode_height = MINIMALHEIGHT;

  utf8 = avt_utf8 ();

  SDL_StartTextInput ();

#else // SDL-1.2

  avt_set_title (title, shortname);

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

  avt_set_icon (akfavatar_xpm);
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

  // needed to get the character of the typed key
  SDL_EnableUNICODE (1);

  // key repeat mode
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  // set up a graphic with the same pixel data
  backend = avt_start_common (avt_data_to_graphic (sdl_screen->pixels,
						   sdl_screen->w,
						   sdl_screen->h));

  // size of the window (not to be confused with the variable window)
  windowmode_width = sdl_screen->w;
  windowmode_height = sdl_screen->h;

#endif /* SDL-1.2 */

  if (not backend or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  backend->update_area = update_area_sdl;
  backend->quit = quit_sdl;
  backend->wait_key = wait_key_sdl;
#ifndef SDL2
  backend->resize = resize_sdl;
#endif

#ifdef IMAGELOADERS
  backend->graphic_file = load_image_file_sdl;
  backend->graphic_stream = load_image_stream_sdl;
  backend->graphic_memory = load_image_memory_sdl;
#endif

  avt_set_mouse_pointer ();

  // ignore what we don't use
  SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState (SDL_KEYUP, SDL_IGNORE);

  return _avt_STATUS;
}
