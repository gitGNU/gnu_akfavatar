/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99
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

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"
#include "SDL.h"
#include "version.h"
#include "rgb.h"		// only for DEFAULT_COLOR

#include <stdint.h>
#include <iso646.h>

// include images
#include "akfavatar.xpm"
#include "btn.xpm"
#include "balloonpointer.xbm"
#include "thinkpointer.xbm"
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

#define BASE_BUTTON_WIDTH 32
#define BASE_BUTTON_HEIGHT 32

/*
 * Most iconv implementations support "" for the systems encoding.
 * You should redefine this macro if and only if it is not supported.
 */
#ifndef SYSTEMENCODING
#  define SYSTEMENCODING  ""
#endif

#define BUTTON_DISTANCE 10

#define AVT_COLOR_BLACK         0x000000
#define AVT_COLOR_WHITE         0xFFFFFF
#define AVT_COLOR_FLORAL_WHITE  0xFFFAF0
#define AVT_COLOR_TAN           0xD2B48C

#define AVT_BUTTON_COLOR        0x665533
#define AVT_CURSOR_COLOR        0xF28919
#define AVT_BALLOON_COLOR       AVT_COLOR_FLORAL_WHITE

#define AVT_XBM_INFO(img)  img##_bits, img##_width, img##_height

// these definitions are deprecated
#if defined(VGA)
#  define FONTWIDTH 7
#  define FONTHEIGHT 14
#  define UNDERLINE 13
#  define NOT_BOLD 0
#else
#  define FONTWIDTH 9
#  define FONTHEIGHT 18
#  define UNDERLINE 15
#  define NOT_BOLD 0
#endif

#define LINEHEIGHT (fontheight)	// + something, if you want

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
#endif // OLD_SDL

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

// don't use any libc commands directly!
#pragma GCC poison  malloc calloc free strlen memcpy memset getenv putenv
#pragma GCC poison  strstr atoi atol strtol

// for static linking avoid to drag in unneeded object files
#pragma GCC poison  avt_colorname avt_palette avt_colors
#pragma GCC poison  avt_avatar_image_default


#define COLORDEPTH 32

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

#define BALLOONPOINTER_OFFSET 20

#define ICONV_UNINITIALIZED   (avt_iconv_t)(-1)

/* try to guess WCHAR_ENCODING,
 * based on WCHAR_MAX or __WCHAR_MAX__ if it is available
 * note: newer SDL versions include stdint.h if available
 */
#if not defined(WCHAR_MAX) and defined(__WCHAR_MAX__)
#  define WCHAR_MAX __WCHAR_MAX__
#endif

#ifndef WCHAR_ENCODING

#  ifndef WCHAR_MAX
#    error "please define WCHAR_ENCODING (no autodetection possible)"
#  endif

#  if (WCHAR_MAX <= 255)
#    define WCHAR_ENCODING "ISO-8859-1"
#  elif (WCHAR_MAX <= 65535U)
#    if (AVT_BIG_ENDIAN == AVT_BYTE_ORDER)
#      define WCHAR_ENCODING "UTF-16BE"
#    else // SDL_BYTEORDER != SDL_BIG_ENDIAN
#      define WCHAR_ENCODING "UTF-16LE"
#    endif // SDL_BYTEORDER != SDL_BIG_ENDIAN
#  else	// (WCHAR_MAX > 65535U)
#    if (AVT_BIG_ENDIAN == AVT_BYTE_ORDER)
#      define WCHAR_ENCODING "UTF-32BE"
#    else // little endian
#      define WCHAR_ENCODING "UTF-32LE"
#    endif // little endian
#  endif // (WCHAR_MAX > 65535U)

#endif // not WCHAR_ENCODING

/*
 * this will be used, when somebody forgets to set the
 * encoding
 */
#define MB_DEFAULT_ENCODING "UTF-8"

// only defined in later SDL versions
#ifndef SDL_BUTTON_WHEELUP
#  define SDL_BUTTON_WHEELUP 4
#endif

#ifndef SDL_BUTTON_WHEELDOWN
#  define SDL_BUTTON_WHEELDOWN 5
#endif

// type for gimp images
#ifndef DISABLE_DEPRECATED
typedef struct
{
  unsigned int width;
  unsigned int height;
  unsigned int bytes_per_pixel;	// 3:RGB, 4:RGBA
  unsigned char pixel_data;	// handle as startpoint
} gimp_img_t;
#endif

enum avt_button_type
{
  btn_cancel, btn_yes, btn_no, btn_right, btn_left, btn_down, btn_up,
  btn_fastforward, btn_fastbackward, btn_stop, btn_pause, btn_help,
  btn_eject, btn_circle
};

static SDL_Surface *screen;
static SDL_Surface *circle;
static SDL_Surface *base_button;
static SDL_Surface *raw_image;
static SDL_Cursor *mpointer;
static SDL_Rect window;		// if screen is in fact larger
static SDL_Rect windowmode_size;	// size of the whole window (screen)
static uint32_t screenflags;	// flags for the screen
static int fontwidth, fontheight, fontunderline;

// conversion descriptors for text input and output
static avt_iconv_t output_cd = ICONV_UNINITIALIZED;
static avt_iconv_t input_cd = ICONV_UNINITIALIZED;


struct avt_position
{
  short x, y;
};

struct avt_settings
{
  SDL_Surface *avatar_image;
  SDL_Surface *cursor_character;
  SDL_Surface *pointer;
  wchar_t *name;

  // for an external keyboard/mouse handlers
  avt_keyhandler ext_keyhandler;
  avt_mousehandler ext_mousehandler;

  // delay values for printing text and flipping the page
  int text_delay, flip_page_delay;

  int ballooncolor;
  int backgroundcolornr;
  int text_color;
  int text_background_color;
  int cursor_color;		// color for cursor and menu-bar
  int bitmap_color;		// color for bitmaps

  avt_char pointer_motion_key;	// key simulated be pointer motion
  avt_char pointer_button_key;	// key simulated for mouse button 1-3

  // colors mapped for the screen
  uint32_t background_color;

  bool newline_mode;		// when off, you need an extra CR
  bool underlined, bold, inverse;	// text underlined, bold?
  bool auto_margin;		// automatic new lines?
  bool avatar_visible;		// avatar visible?
  bool text_cursor_visible;	// shall the text cursor be visible?
  bool text_cursor_actually_visible;	// is it actually visible?
  bool reserve_single_keys;	// reserve single keys?
  bool markup;			// markup-syntax activated?
  bool hold_updates;		// holding updates back?
  bool tab_stops[AVT_LINELENGTH];

  // origin mode
  // Home: textfield (false) or viewport (true)
  // avt_initialize sets it to true for backwards compatibility
  bool origin_mode;

  char encoding[100];

  short int mode;		// whether fullscreen or window or ...
  short int avatar_mode;
  short int scroll_mode;
  short int textdir_rtl;
  short int linestart;		// beginning of line - depending on text direction
  short int balloonheight, balloonmaxheight, balloonwidth;

  struct avt_position cursor, saved_position;

  SDL_Rect textfield;
  SDL_Rect viewport;		// sub-window in textfield
};


static struct avt_settings avt = {
  .backgroundcolornr = DEFAULT_COLOR,
  .ballooncolor = AVT_BALLOON_COLOR,
  .cursor_color = AVT_CURSOR_COLOR,
  .textdir_rtl = AVT_LEFT_TO_RIGHT
};


// Note: any event should have an associated key, so key == event

#define AVT_KEYBUFFER_SIZE  512

struct avt_key_buffer
{
  unsigned short int position, end;
  avt_char buffer[AVT_KEYBUFFER_SIZE];
};

static struct avt_key_buffer avt_keys;

struct avt_button
{
  short int x, y;
  avt_char key;
  SDL_Surface *background;
};

#define MAX_BUTTONS 15

static struct avt_button avt_buttons[MAX_BUTTONS];

#if defined(__GNUC__) and not defined(__WIN32__)
#  define AVT_HIDDEN __attribute__((__visibility__("hidden")))
#else
#  define AVT_HIDDEN
#endif // __GNUC__

#ifdef DISABLE_DEPRECATED
#  define DEPRECATED_EXTERN  static
#  define avt_image_t  SDL_Surface
#else
#  define DEPRECATED_EXTERN  extern
#endif

// 0 = normal; 1 = quit-request; -1 = error
int _avt_STATUS AVT_HIDDEN;

void (*avt_alert_func) (void) AVT_HIDDEN = NULL;
void (*avt_quit_audio_func) (void) AVT_HIDDEN = NULL;

// forward declaration
static int avt_pause (void);
static void avt_drawchar (avt_char ch, SDL_Surface * surface);
static SDL_Surface *avt_save_background (SDL_Rect area);
static void avt_analyze_event (SDL_Event * event);


// Fast putpixel with no checks
// surface must have 32 bits per pixel!
// surface must eventually be locked
static inline void
avt_putpixel (SDL_Surface * s, int x, int y, int color)
{
  uint8_t *p;

  p = ((uint8_t *) s->pixels) + y * s->pitch + (x * sizeof (uint32_t));
  *((uint32_t *) p) = color;
}

static inline void
avt_fill_area (SDL_Surface * s, int x, int y, int width, int height,
	       int color)
{
  SDL_Rect dst;

  dst.x = x;
  dst.y = y;
  dst.w = width;
  dst.h = height;

  SDL_FillRect (s, &dst, color);
}

static inline void
avt_fill (SDL_Surface * s, int color)
{
  SDL_FillRect (s, NULL, color);
}

static inline void
avt_update_area (int x, int y, int width, int height)
{
  SDL_UpdateRect (screen, x, y, width, height);
}

static inline void
avt_update_rect (SDL_Rect rect)
{
  avt_update_area (rect.x, rect.y, rect.w, rect.h);
}

static inline void
avt_update_trect (SDL_Rect rect)
{
  if (not avt.hold_updates)
    avt_update_area (rect.x, rect.y, rect.w, rect.h);
}

static inline void
avt_update_all (void)
{
  avt_update_area (0, 0, 0, 0);
}

static inline void
avt_release_raw_image (void)
{
  if (raw_image)
    {
      SDL_FreeSurface (raw_image);
      raw_image = NULL;
    }
}

static inline void
bell (void)
{
  if (avt_alert_func)
    (*avt_alert_func) ();
}

static int
calculate_balloonmaxheight (void)
{
  int avatar_height;

  avatar_height = avt.avatar_image ? avt.avatar_image->h + AVATAR_MARGIN : 0;

  avt.balloonmaxheight = (window.h - avatar_height - (2 * TOPMARGIN)
			  - (2 * BALLOON_INNER_MARGIN)) / LINEHEIGHT;

  // check, whether image is too high
  // at least 10 lines
  if (avt.balloonmaxheight < 10)
    {
      SDL_SetError ("Avatar image too large");
      _avt_STATUS = AVT_ERROR;
      SDL_FreeSurface (avt.avatar_image);
      avt.avatar_image = NULL;
    }

  return _avt_STATUS;
}

// set the inner window for the avatar-display
static void
avt_avatar_window (void)
{
  // window may be smaller than the screen
  window.w = MINIMALWIDTH;
  window.h = MINIMALHEIGHT;
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;
  SDL_SetClipRect (screen, &window);
  calculate_balloonmaxheight ();
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


// X-Pixmap (XPM) support

// number of printable ASCII codes
#define XPM_NR_CODES (126 - 32 + 1)

// for xpm codes
union xpm_codes
{
  uint32_t nr;
  union xpm_codes *next;
};

static void
avt_free_xpm_tree (union xpm_codes *tree, int depth, int cpp)
{
  union xpm_codes *e;

  if (depth < cpp)
    {
      for (int i = 0; i < XPM_NR_CODES; i++)
	{
	  e = (tree + i)->next;
	  if (e != NULL)
	    avt_free_xpm_tree (e, depth + 1, cpp);
	}
    }

  SDL_free (tree);
}

static inline SDL_Color
avt_sdlcolor (int colornr)
{
  SDL_Color color;

  color.r = avt_red (colornr);
  color.g = avt_green (colornr);
  color.b = avt_blue (colornr);

  return color;
}

// use this for internal stuff!
static SDL_Surface *
avt_load_image_xpm (char **xpm)
{
  SDL_Surface *img;
  int width, height, ncolors, cpp;
  int colornr;
  union xpm_codes *codes;
  uint32_t *colors;
  SDL_Color *colors256;
  int code_nr;

  codes = NULL;
  colors = NULL;
  colors256 = NULL;
  img = NULL;

  // check if we actually have data to process
  if (not xpm or not * xpm)
    goto done;

  /* read value line
   * there may be more values in the line, but we just
   * need the first four
   */
  if (SDL_sscanf (xpm[0], "%d %d %d %d", &width, &height, &ncolors, &cpp) < 4
      or width < 1 or height < 1 or ncolors < 1)
    {
      SDL_SetError ("error in XPM data");
      goto done;
    }

  // create target surface
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

  if (not img)
    {
      SDL_SetError ("out of memory");
      goto done;
    }

  // get memory for codes table
  if (cpp > 1)
    {
      codes = (union xpm_codes *) SDL_calloc (XPM_NR_CODES, sizeof (codes));
      if (not codes)
	{
	  SDL_SetError ("out of memory");
	  SDL_free (img);
	  img = NULL;
	  goto done;
	}
    }

  // get memory for colors table (palette)
  if (ncolors <= 256)
    colors256 = (SDL_Color *) SDL_calloc (256, sizeof (SDL_Color));
  else
    colors = (uint32_t *) SDL_calloc (ncolors, sizeof (uint32_t));

  /*
   * note: for colors256 the colors will be scattered around the palette
   * so we need a full sized palette
   *
   * colors is a different type so we have to call SDL_MapRGB only once
   * for each color
   */

  if (not colors and not colors256)
    {
      SDL_SetError ("out of memory");
      SDL_free (img);
      img = NULL;
      goto done;
    }

  code_nr = 0;

  // process colors
  for (colornr = 1; colornr <= ncolors; colornr++, code_nr++)
    {
      char *p;			// pointer for scanning through the string

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
      else			// store characters in codes table
	{
	  char c;
	  union xpm_codes *table;

	  c = '\0';
	  table = codes;
	  for (int i = 0; i < cpp - 1; i++)
	    {
	      c = xpm[colornr][i];

	      if (c < 32 or c > 126)
		break;

	      table = (table + (c - 32));

	      if (not table->next)
		table->next =
		  (union xpm_codes *) SDL_calloc (XPM_NR_CODES,
						  sizeof (*codes));

	      table = table->next;
	    }

	  if (c < 32 or c > 126)
	    break;

	  c = xpm[colornr][cpp - 1];

	  if (c < 32 or c > 126)
	    break;

	  (table + (c - 32))->nr = colornr - 1;
	}

      // scan for color definition
      p = &xpm[colornr][cpp];	// skip color-characters
      while (*p and (*p != 'c' or not avt_isblank (*(p + 1))
		     or not avt_isblank (*(p - 1))))
	p++;

      // no color definition found? search for grayscale definition
      if (not * p)
	{
	  p = &xpm[colornr][cpp];	// skip color-characters
	  while (*p and (*p != 'g' or not avt_isblank (*(p + 1))
			 or not avt_isblank (*(p - 1))))
	    p++;
	}

      // no grayscale definition found? search for g4 definition
      if (not * p)
	{
	  p = &xpm[colornr][cpp];	// skip color-characters
	  while (*p
		 and (*p != '4' or * (p - 1) != 'g'
		      or not avt_isblank (*(p + 1))
		      or not avt_isblank (*(p - 2))))
	    p++;
	}

      // search for monochrome definition
      if (not * p)
	{
	  p = &xpm[colornr][cpp];	// skip color-characters
	  while (*p and (*p != 'm' or not avt_isblank (*(p + 1))
			 or not avt_isblank (*(p - 1))))
	    p++;
	}

      if (*p)
	{
	  int colornr;
	  size_t color_name_pos;
	  char color_name[80];

	  // skip to color name/definition
	  p++;
	  while (*p and avt_isblank (*p))
	    p++;

	  // copy colorname up to next space
	  color_name_pos = 0;
	  while (*p and not avt_isblank (*p)
		 and color_name_pos < sizeof (color_name) - 1)
	    color_name[color_name_pos++] = *p++;
	  color_name[color_name_pos] = '\0';

	  colornr = AVT_COLOR_BLACK;

	  if (color_name[0] == '#')
	    colornr = SDL_strtol (&color_name[1], NULL, 16);
	  else if (SDL_strcasecmp (color_name, "None") == 0)
	    {
	      SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL, code_nr);

	      // some weird color, that hopefully doesn't conflict
	      colornr = 0x1A2A3A;
	    }
	  else if (SDL_strcasecmp (color_name, "black") == 0)
	    colornr = AVT_COLOR_BLACK;
	  else if (SDL_strcasecmp (color_name, "white") == 0)
	    colornr = AVT_COLOR_WHITE;

	  /*
	   * Note: don't use avt_colorname,
	   * or the palette is always needed
	   */

	  if (ncolors <= 256)
	    {
	      colors256[code_nr] = avt_sdlcolor (colornr);
	    }
	  else			// ncolors > 256
	    {
	      *(colors + colornr - 1) =
		SDL_MapRGB (img->format, avt_red (colornr),
			    avt_green (colornr), avt_blue (colornr));
	    }
	}
    }

  // put colormap into the image
  if (ncolors <= 256)
    {
      SDL_SetPalette (img, SDL_LOGPAL, colors256, 0, 256);
      SDL_free (colors256);
    }

  // process pixeldata
  if (SDL_MUSTLOCK (img))
    SDL_LockSurface (img);
  if (cpp == 1)			// the easiest case
    {
      for (int line = 0; line < height; line++)
	{
	  // check for premture end of data
	  if (xpm[ncolors + 1 + line] == NULL)
	    break;

	  SDL_memcpy ((uint8_t *) img->pixels + (line * img->pitch),
		      xpm[ncolors + 1 + line], width);
	}
    }
  else				// cpp != 1
    {
      uint8_t *pix;
      char *xpm_line;
      int bpp;

      // Bytes per Pixel of img
      bpp = img->format->BytesPerPixel;

      for (int line = 0; line < height; line++)
	{
	  // point to beginning of the line
	  pix = (uint8_t *) img->pixels + (line * img->pitch);
	  xpm_line = xpm[ncolors + 1 + line];

	  // check for premture end of data
	  if (xpm_line == NULL)
	    break;

	  for (int pos = 0; pos < width; pos++, pix += bpp)
	    {
	      union xpm_codes *table;
	      char c;

	      c = '\0';
	      // find code in codes table
	      table = codes;
	      for (int i = 0; i < cpp - 1; i++)
		{
		  c = xpm_line[pos * cpp + i];
		  if (c < 32 or c > 126)
		    break;
		  table = (table + (c - 32))->next;
		}

	      if (c < 32 or c > 126)
		break;

	      c = xpm_line[pos * cpp + cpp - 1];
	      if (c < 32 or c > 126)
		break;

	      code_nr = (table + (c - 32))->nr;

	      if (ncolors <= 256)
		*pix = code_nr;
	      else
		*(uint32_t *) pix = *(colors + code_nr);
	    }
	}
    }
  if (SDL_MUSTLOCK (img))
    SDL_UnlockSurface (img);

done:
  if (colors)
    SDL_free (colors);

  // clean up codes table
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

  if (not src)
    return NULL;

  img = NULL;
  xpm = NULL;
  line = NULL;
  end = error = false;

  start = SDL_RWtell (src);

  // check if it has an XPM header
  if (SDL_RWread (src, head, sizeof (head), 1) < 1
      or SDL_memcmp (head, "/* XPM */", 9) != 0)
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
  if (not line)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  linecount = 512;		// can be extended later
  xpm = (char **) SDL_malloc (linecount * sizeof (*xpm));
  if (not xpm)
    {
      SDL_SetError ("out of memory");
      error = end = true;
    }

  while (not end)
    {
      // skip to next quote
      do
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    end = true;
	}
      while (not end and c != '"');

      // read line
      linepos = 0;
      c = '\0';
      while (not end and c != '"')
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = true;	// shouldn't happen here

	  if (c != '"')
	    line[linepos++] = c;

	  if (linepos >= linecapacity)
	    {
	      linecapacity += 100;
	      line = (char *) SDL_realloc (line, linecapacity);
	      if (not line)
		error = end = true;
	    }
	}

      // copy line
      if (not end)
	{
	  line[linepos++] = '\0';
	  xpm[linenr] = (char *) SDL_malloc (linepos);
	  SDL_memcpy (xpm[linenr], line, linepos);
	  linenr++;
	  if (linenr >= linecount)	// leave one line reserved
	    {
	      linecount += 512;
	      xpm = (char **) SDL_realloc (xpm, linecount * sizeof (*xpm));
	      if (not xpm)
		error = end = true;
	    }
	}
    }

  /* last line must be NULL,
   * so premature end of data can be detected later
   */
  if (xpm)
    xpm[linenr] = NULL;

  if (not error)
    img = avt_load_image_xpm (xpm);

  // free xpm
  if (xpm)
    {
      // linenr points to next (uninitialized) line
      for (unsigned int i = 0; i < linenr; i++)
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
		    int colornr)
{
  SDL_Surface *img;
  SDL_Color color[2];
  int bpl;			// Bytes per line
  uint8_t *line;

  img = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height, 1, 0, 0, 0, 0);

  if (not img)
    {
      SDL_SetError ("out of memory");
      return NULL;
    }

  // Bytes per line
  bpl = (width + 7) / 8;

  if (SDL_MUSTLOCK (img))
    SDL_LockSurface (img);

  line = (uint8_t *) img->pixels;

  for (int y = 0; y < height; y++)
    {
      for (int byte = 0; byte < bpl; byte++)
	{
	  uint8_t val, res;

	  val = *bits;
	  res = 0;

	  // bits must be reversed
	  for (int bit = 7; bit >= 0; bit--)
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

  color[0].r = compl avt_red (colornr);
  color[0].g = compl avt_green (colornr);
  color[0].b = compl avt_blue (colornr);
  color[1] = avt_sdlcolor (colornr);
  SDL_SetPalette (img, SDL_LOGPAL, color, 0, 2);
  SDL_SetColorKey (img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);

  return img;
}

static SDL_Surface *
avt_load_image_xbm_RW (SDL_RWops * src, int freesrc, int color)
{
  unsigned char *bits;
  int width, height;
  int start;
  unsigned int bytes, bmpos;
  char line[1024];
  SDL_Surface *img;
  bool end, error;
  bool X10;

  if (not src)
    return NULL;

  img = NULL;
  bits = NULL;
  X10 = false;
  end = error = false;
  width = height = bytes = bmpos = 0;

  start = SDL_RWtell (src);

  // check if it starts with #define
  if (SDL_RWread (src, line, 1, sizeof (line) - 1) < 1
      or SDL_memcmp (line, "#define", 7) != 0)
    {
      if (freesrc)
	SDL_RWclose (src);
      else
	SDL_RWseek (src, start, RW_SEEK_SET);

      return NULL;
    }

  // make it usable as a string
  line[sizeof (line) - 1] = '\0';

  // search for width and height
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

  if (width and height)
    {
      bytes = ((width + 7) / 8) * height;
      // one byte larger for safety with old X10 format
      bits = (unsigned char *) SDL_malloc (bytes + 1);
    }

  // this catches different errors
  if (not bits)
    {
      SDL_SetError ("out of memory");
      error = end = true;
    }

  // search start of bitmap part
  if (not end and not error)
    {
      char c;

      SDL_RWseek (src, start, RW_SEEK_SET);

      do
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = true;
	}
      while (c != '{' and not error);

      if (error)		// no '{' found
	goto done;

      // skip newline
      SDL_RWread (src, &c, sizeof (c), 1);
    }

  while (not end and not error)
    {
      char c;
      unsigned int linepos;

      // read line
      linepos = 0;
      c = '\0';
      while (not end and linepos < sizeof (line) and c != '\n')
	{
	  if (SDL_RWread (src, &c, sizeof (c), 1) < 1)
	    error = end = true;

	  if (c != '\n' and c != '}')
	    line[linepos++] = c;

	  if (c == '}')
	    end = true;
	}
      line[linepos] = '\0';

      // parse line
      if (line[0] != '\0')
	{
	  char *p;
	  char *endptr;
	  long value;
	  bool end_of_line;

	  p = line;
	  end_of_line = false;
	  while (not end_of_line and bmpos < bytes)
	    {
	      value = SDL_strtol (p, &endptr, 0);
	      if (endptr == p)
		end_of_line = true;
	      else
		{
		  if (not X10)
		    bits[bmpos++] = value;
		  else		// X10
		    {
		      unsigned short *v;
		      // image is assumed to be in native endianess
		      v = (unsigned short *) (bits + bmpos);
		      *v = value;
		      bmpos += sizeof (*v);
		    }

		  p = endptr + 1;	// skip comma
		}
	    }
	}
    }

  if (not error)
    img = avt_load_image_xbm (bits, width, height, color);

done:
  // free bits
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
  SDL_Surface *img;

  img = NULL;

  if (src)
    {
      img = SDL_LoadBMP_RW (src, 0);

      if (img == NULL)
	img = avt_load_image_xpm_RW (src, 0);

      if (img == NULL)
	img = avt_load_image_xbm_RW (src, 0, avt.bitmap_color);

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


extern const char *
avt_copyright (void)
{
  return "Copyright (c) " COPYRIGHTYEAR " Andreas K. Foerster";
}

extern const char *
avt_license (void)
{
  return "GPLv3+: GNU GPL version 3 or later "
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

// taken from the SDL documentation
/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static int
avt_getpixel (SDL_Surface * surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  // Here p is the address to the pixel we want to retrieve
  uint8_t *p = (uint8_t *) surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
    {
    case 1:
      return *p;

    case 2:
      return *(uint16_t *) p;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	return p[0] << 16 | p[1] << 8 | p[2];
      else
	return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
      return *(uint32_t *) p;

    default:
      return 0;			// shouldn't happen, but avoids warnings
    }
}

// avoid using the libc
static int
avt_strwidth (const wchar_t * m)
{
  int l = 0;

  while (*m++)
    l++;

  return l;
}

// shows or clears the text cursor in the current position
// note: this function is rather time consuming
static void
avt_show_text_cursor (bool on)
{
  SDL_Rect dst;

  if (on != avt.text_cursor_actually_visible and not avt.hold_updates)
    {
      dst.x = avt.cursor.x;
      dst.y = avt.cursor.y;
      dst.w = fontwidth;
      dst.h = fontheight;

      if (on)
	{
	  int bg_color;

	  // save character under cursor
	  SDL_BlitSurface (screen, &dst, avt.cursor_character, NULL);

	  // assume lower right corner is the background color
	  bg_color = avt_getpixel (avt.cursor_character,
				   fontwidth - 1, fontheight - 1);

	  // show text-cursor
	  for (int y = avt.cursor.y + fontheight - 1; y >= avt.cursor.y; y--)
	    for (int x = avt.cursor.x + fontwidth - 1; x >= avt.cursor.x; x--)
	      if (bg_color == avt_getpixel (screen, x, y))
		avt_putpixel (screen, x, y, avt.cursor_color);

	  avt_update_area (avt.cursor.x, avt.cursor.y, fontwidth, fontheight);
	}
      else
	{
	  // restore saved character
	  SDL_BlitSurface (avt.cursor_character, NULL, screen, &dst);
	  avt_update_area (avt.cursor.x, avt.cursor.y, fontwidth, fontheight);
	}

      avt.text_cursor_actually_visible = on;
    }
}

extern void
avt_activate_cursor (bool on)
{
  avt.text_cursor_visible = on;

  if (screen and avt.textfield.x >= 0)
    avt_show_text_cursor (avt.text_cursor_visible);
}

static inline void
avt_no_textfield (void)
{
  avt.textfield.x = avt.textfield.y = avt.textfield.w = avt.textfield.h = -1;
  avt.viewport = avt.textfield;
}

/*
 * fills the screen with the background color,
 * but doesn't update the screen yet
 */
static inline void
avt_free_screen (void)
{
  // switch clipping off
  SDL_SetClipRect (screen, NULL);
  // fill the whole screen with background color
  avt_fill (screen, avt.background_color);
}

extern void
avt_clear_screen (void)
{
  if (screen)
    {
      avt_free_screen ();
      avt_update_all ();
    }

  // undefine textfield / viewport
  avt_no_textfield ();
  avt.avatar_visible = false;
}

#define NAME_PADDING 3

static void
avt_show_name (void)
{
  int x, y;
  int old_text_color, old_background_color;
  wchar_t *p;

  if (screen and avt.avatar_image and avt.name)
    {
      // save old character colors
      old_text_color = avt.text_color;
      old_background_color = avt.text_background_color;

      avt.text_color = AVT_COLOR_BLACK;
      avt.text_background_color = AVT_COLOR_TAN;

      if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	x = ((window.x + window.w) / 2) + (avt.avatar_image->w / 2)
	  + BUTTON_DISTANCE;
      else			// left
	x = window.x + AVATAR_MARGIN + avt.avatar_image->w + BUTTON_DISTANCE;

      if (AVT_HEADER == avt.avatar_mode)
	y = window.y + TOPMARGIN + avt.avatar_image->h
	  - fontheight - 2 * NAME_PADDING;
      else
	y = window.y + window.h - AVATAR_MARGIN
	  - fontheight - 2 * NAME_PADDING;

      // draw sign
      avt_fill_area (screen, x, y,
		     (avt_strwidth (avt.name) * fontwidth) + 2 * NAME_PADDING,
		     fontheight + 2 * NAME_PADDING,
		     avt.text_background_color);

      // show name
      avt.cursor.x = x + NAME_PADDING;
      avt.cursor.y = y + NAME_PADDING;

      p = avt.name;
      while (*p)
	{
	  avt_drawchar ((avt_char) * p++, screen);
	  avt.cursor.x += fontwidth;
	}

      // restore old character colors
      avt.text_color = old_text_color;
      avt.text_background_color = old_background_color;
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
      // fill the screen with background color
      // (not only the window!)
      avt_free_screen ();

      avt_release_raw_image ();	// not needed anymore

      SDL_SetClipRect (screen, &window);
      avt_avatar_window ();

      if (avt.avatar_image)
	{
	  if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	    dst.x = ((window.x + window.w) / 2) - (avt.avatar_image->w / 2);
	  else			// left
	    dst.x = window.x + AVATAR_MARGIN;

	  if (AVT_HEADER == avt.avatar_mode)
	    dst.y = window.y + TOPMARGIN;
	  else			// bottom
	    dst.y = window.y + window.h - avt.avatar_image->h - AVATAR_MARGIN;

	  dst.w = avt.avatar_image->w;
	  dst.h = avt.avatar_image->h;
	  SDL_BlitSurface (avt.avatar_image, NULL, screen, &dst);
	}

      if (avt.name)
	avt_show_name ();
    }
}

extern void
avt_show_avatar (void)
{
  if (screen)
    {
      avt_draw_avatar ();
      avt_update_all ();

      // undefine textfield
      avt.textfield.x = avt.textfield.y = avt.textfield.w = avt.textfield.h =
	-1;
      avt.viewport = avt.textfield;
      avt.avatar_visible = true;
    }
}

static void
avt_draw_balloon2 (int offset, uint32_t ballooncolor)
{
  SDL_Rect shape;

  // full size
  shape.x = avt.textfield.x - BALLOON_INNER_MARGIN + offset;
  shape.w = avt.textfield.w + (2 * BALLOON_INNER_MARGIN);
  shape.y = avt.textfield.y - BALLOON_INNER_MARGIN + offset;
  shape.h = avt.textfield.h + (2 * BALLOON_INNER_MARGIN);

  // horizontal shape
  avt_fill_area (screen, shape.x, shape.y + (circle_height / 2),
		 shape.w, shape.h - circle_height, ballooncolor);

  // vertical shape
  avt_fill_area (screen, shape.x + (circle_width / 2), shape.y,
		 shape.w - circle_width, shape.h, ballooncolor);

  // draw corners
  {
    SDL_Rect circle_piece, corner_pos;

    // prepare circle piece
    // the size is always the same
    circle_piece.w = circle_width / 2;
    circle_piece.h = circle_height / 2;

    // upper left corner
    circle_piece.x = 0;
    circle_piece.y = 0;
    corner_pos.x = shape.x;
    corner_pos.y = shape.y;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    // upper right corner
    circle_piece.x = ((circle_width + 7) / 8) / 2;
    circle_piece.y = 0;
    corner_pos.x = shape.x + shape.w - circle_piece.w;
    corner_pos.y = shape.y;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    // lower left corner
    circle_piece.x = 0;
    circle_piece.y = circle_height / 2;
    corner_pos.x = shape.x;
    corner_pos.y = shape.y + shape.h - circle_piece.h;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);

    // lower right corner
    circle_piece.x = ((circle_width + 7) / 8) / 2;
    circle_piece.y = circle_height / 2;
    corner_pos.x = shape.x + shape.w - circle_piece.w;
    corner_pos.y = shape.y + shape.h - circle_piece.h;
    SDL_BlitSurface (circle, &circle_piece, screen, &corner_pos);
  }

  // draw balloonpointer
  // only if there is an avatar image
  if (avt.avatar_image
      and AVT_FOOTER != avt.avatar_mode and AVT_HEADER != avt.avatar_mode)
    {
      SDL_Rect pointer_shape, pointer_pos;

      pointer_shape.x = pointer_shape.y = 0;
      pointer_shape.w = avt.pointer->w;
      pointer_shape.h = avt.pointer->h;

      // if the balloonpointer is too large, cut it
      if (pointer_shape.h > (avt.avatar_image->h / 2))
	{
	  pointer_shape.y = pointer_shape.h - (avt.avatar_image->h / 2);
	  pointer_shape.h -= pointer_shape.y;
	}

      pointer_pos.x =
	window.x + avt.avatar_image->w + (2 * AVATAR_MARGIN) +
	BALLOONPOINTER_OFFSET + offset;
      pointer_pos.y =
	window.y + (avt.balloonmaxheight * LINEHEIGHT) +
	(2 * BALLOON_INNER_MARGIN) + TOPMARGIN + offset;

      // only draw the balloonpointer, when it fits
      if (pointer_pos.x + avt.pointer->w + BALLOONPOINTER_OFFSET
	  + BALLOON_INNER_MARGIN < window.x + window.w)
	SDL_BlitSurface (avt.pointer, &pointer_shape, screen, &pointer_pos);
    }
}

static void
avt_draw_balloon (void)
{
  SDL_Color shadow_color, balloon_color;
  int16_t centered_y;

  if (not avt.avatar_visible)
    avt_draw_avatar ();

  SDL_SetClipRect (screen, &window);

  avt.textfield.w = (avt.balloonwidth * fontwidth);
  avt.textfield.h = (avt.balloonheight * fontheight);
  centered_y = window.y + (window.h / 2) - (avt.textfield.h / 2);

  if (not avt.avatar_image)
    avt.textfield.y = centered_y;	// middle of the window
  else
    {
      // align with balloon
      if (AVT_HEADER == avt.avatar_mode)
	avt.textfield.y = window.y + avt.avatar_image->h + AVATAR_MARGIN
	  + TOPMARGIN + BALLOON_INNER_MARGIN;
      else
	avt.textfield.y =
	  window.y + ((avt.balloonmaxheight - avt.balloonheight) * LINEHEIGHT)
	  + TOPMARGIN + BALLOON_INNER_MARGIN;

      // in separate or heading mode it might also be better to center it
      if ((AVT_FOOTER == avt.avatar_mode and avt.textfield.y > centered_y)
	  or (AVT_HEADER == avt.avatar_mode and avt.textfield.y < centered_y))
	avt.textfield.y = centered_y;
    }

  // horizontally centered as default
  avt.textfield.x =
    window.x + (window.w / 2) - (avt.balloonwidth * fontwidth / 2);

  // align horizontally with balloonpointer
  if (avt.avatar_image
      and AVT_FOOTER != avt.avatar_mode and AVT_HEADER != avt.avatar_mode)
    {
      // left border not aligned with balloon pointer?
      if (avt.textfield.x >
	  window.x + avt.avatar_image->w + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET)
	avt.textfield.x =
	  window.x + avt.avatar_image->w + (2 * AVATAR_MARGIN) +
	  BALLOONPOINTER_OFFSET;

      // right border not aligned with balloon pointer?
      if (avt.textfield.x + avt.textfield.w <
	  window.x + avt.avatar_image->w + avt.pointer->w
	  + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET)
	{
	  avt.textfield.x =
	    window.x + avt.avatar_image->w - avt.textfield.w + avt.pointer->w
	    + (2 * AVATAR_MARGIN) + BALLOONPOINTER_OFFSET;

	  // align with right window-border
	  if (avt.textfield.x >
	      window.x + window.w - (avt.balloonwidth * fontwidth) -
	      (2 * BALLOON_INNER_MARGIN))
	    avt.textfield.x =
	      window.x + window.w - (avt.balloonwidth * fontwidth) -
	      (2 * BALLOON_INNER_MARGIN);
	}
    }

  avt.viewport = avt.textfield;

  // shadow color is a little darker than the background color
  shadow_color = avt_sdlcolor (avt.backgroundcolornr);
  shadow_color.r = (shadow_color.r > 0x20) ? shadow_color.r - 0x20 : 0;
  shadow_color.g = (shadow_color.g > 0x20) ? shadow_color.g - 0x20 : 0;
  shadow_color.b = (shadow_color.b > 0x20) ? shadow_color.b - 0x20 : 0;

  SDL_SetColors (circle, &shadow_color, 1, 1);
  SDL_SetColors (avt.pointer, &shadow_color, 1, 1);

  // first draw shadow
  avt_draw_balloon2 (SHADOWOFFSET,
		     SDL_MapRGB (screen->format, shadow_color.r,
				 shadow_color.g, shadow_color.b));

  // real balloon
  balloon_color = avt_sdlcolor (avt.ballooncolor);
  SDL_SetColors (circle, &balloon_color, 1, 1);
  SDL_SetColors (avt.pointer, &balloon_color, 1, 1);

  avt_draw_balloon2 (0, SDL_MapRGB (screen->format,
				    avt_red (avt.ballooncolor),
				    avt_green (avt.ballooncolor),
				    avt_blue (avt.ballooncolor)));

  avt.linestart =
    (avt.textdir_rtl) ? avt.viewport.x + avt.viewport.w -
    fontwidth : avt.viewport.x;

  avt.avatar_visible = true;

  // cursor at top 
  avt.cursor.x = avt.linestart;
  avt.cursor.y = avt.viewport.y;

  // reset saved position
  avt.saved_position = avt.cursor;

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  // update everything
  // (there may be leftovers from large images)
  avt_update_all ();

  /*
   * only allow drawings inside this area from now on
   * (only for blitting)
   */
  SDL_SetClipRect (screen, &avt.viewport);
}

extern void
avt_text_direction (int direction)
{
  SDL_Rect area;

  avt.textdir_rtl = direction;

  /*
   * if there is already a ballon,
   * recalculate the linestart and put the cursor in the first position
   */
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.linestart =
	(avt.textdir_rtl) ? area.x + area.w - fontwidth : area.x;
      avt.cursor.x = avt.linestart;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_set_balloon_width (int width)
{
  if (width != avt.balloonwidth)
    {
      if (width < AVT_LINELENGTH and width > 0)
	avt.balloonwidth = (width > 7) ? width : 7;
      else
	avt.balloonwidth = AVT_LINELENGTH;

      // if balloon is visible, redraw it
      if (avt.textfield.x >= 0)
	{
	  avt.avatar_visible = false;	// force to redraw everything
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_balloon_height (int height)
{
  if (height != avt.balloonheight)
    {
      if (height > 0 and height < avt.balloonmaxheight)
	avt.balloonheight = height;
      else
	avt.balloonheight = avt.balloonmaxheight;

      // if balloon is visible, redraw it
      if (avt.textfield.x >= 0)
	{
	  avt.avatar_visible = false;	// force to redraw everything
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_balloon_size (int height, int width)
{
  if (height != avt.balloonheight or width != avt.balloonwidth)
    {
      if (height > 0 and height < avt.balloonmaxheight)
	avt.balloonheight = height;
      else
	avt.balloonheight = avt.balloonmaxheight;

      if (width < AVT_LINELENGTH and width > 0)
	avt.balloonwidth = (width > 7) ? width : 7;
      else
	avt.balloonwidth = AVT_LINELENGTH;

      // if balloon is visible, redraw it
      if (avt.textfield.x >= 0)
	{
	  avt.avatar_visible = false;	// force to redraw everything
	  avt_draw_balloon ();
	}
    }
}

extern void
avt_set_avatar_mode (int mode)
{
  if (not screen)
    {
      avt.avatar_mode = mode;
      return;
    }

  if (mode != avt.avatar_mode)
    {
      switch (mode)
	{
	case AVT_SAY:
	  SDL_FreeSurface (avt.pointer);
	  avt.pointer =
	    avt_load_image_xbm (AVT_XBM_INFO (balloonpointer),
				avt.ballooncolor);
	  avt.avatar_mode = AVT_SAY;
	  break;

	case AVT_THINK:
	  SDL_FreeSurface (avt.pointer);
	  avt.pointer =
	    avt_load_image_xbm (AVT_XBM_INFO (thinkpointer),
				avt.ballooncolor);
	  avt.avatar_mode = AVT_THINK;
	  break;

	case AVT_FOOTER:
	  avt.avatar_mode = AVT_FOOTER;
	  break;

	case AVT_HEADER:
	  avt.avatar_mode = AVT_HEADER;
	  break;
	}
    }

  // if balloon is visible, remove it
  if (avt.textfield.x >= 0)
    avt_show_avatar ();
}

static void
avt_resize (int w, int h)
{
  SDL_Surface *oldwindowimage;
  SDL_Rect oldwindow;
  SDL_Event event;

  // minimal size
  if (w < MINIMALWIDTH)
    w = MINIMALWIDTH;
  if (h < MINIMALHEIGHT)
    h = MINIMALHEIGHT;

  // save the window
  oldwindow = window;
  oldwindowimage = avt_save_background (window);

  // resize screen
  screen = SDL_SetVideoMode (w, h, COLORDEPTH, screenflags);

  avt_free_screen ();

  // new position of the window on the screen
  window.x = screen->w > window.w ? (screen->w / 2) - (window.w / 2) : 0;
  window.y = screen->h > window.h ? (screen->h / 2) - (window.h / 2) : 0;

  // restore image
  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (oldwindowimage, NULL, screen, &window);
  SDL_FreeSurface (oldwindowimage);

  // recalculate textfield & viewport positions
  if (avt.textfield.x >= 0)
    {
      avt.textfield.x = avt.textfield.x - oldwindow.x + window.x;
      avt.textfield.y = avt.textfield.y - oldwindow.y + window.y;

      avt.viewport.x = avt.viewport.x - oldwindow.x + window.x;
      avt.viewport.y = avt.viewport.y - oldwindow.y + window.y;

      avt.linestart =
	(avt.textdir_rtl) ? avt.viewport.x + avt.viewport.w -
	fontwidth : avt.viewport.x;

      avt.cursor.x = avt.cursor.x - oldwindow.x + window.x;
      avt.cursor.y = avt.cursor.y - oldwindow.y + window.y;
      SDL_SetClipRect (screen, &avt.viewport);
    }

  // set windowmode_size
  if ((screenflags & SDL_FULLSCREEN) == 0)
    {
      windowmode_size.w = w;
      windowmode_size.h = h;
    }

  // make all changes visible
  avt_update_all ();

  // ignore one resize event here to avoid recursive calling
  while (SDL_PollEvent (&event) and event.type != SDL_VIDEORESIZE)
    avt_analyze_event (&event);
}

extern void
avt_bell (void)
{
  bell ();
}

// saves the background of the area
// the result should be freed with SDL_FreeSurface
static SDL_Surface *
avt_save_background (SDL_Rect area)
{
  SDL_Surface *result;

  result =
    SDL_CreateRGBSurface (SDL_SWSURFACE, area.w, area.h,
			  screen->format->BitsPerPixel,
			  screen->format->Rmask, screen->format->Gmask,
			  screen->format->Bmask, screen->format->Amask);

  if (not result)
    {
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return NULL;
    }

  SDL_BlitSurface (screen, &area, result, NULL);

  return result;
}

// flashes the screen
extern void
avt_flash (void)
{
  SDL_Surface *oldwindowimage;

  if (not screen)
    return;

  oldwindowimage = avt_save_background (window);

  // switch clipping off
  SDL_SetClipRect (screen, NULL);
  // fill the whole screen with color
  avt_fill (screen, 0xFFFF00);
  avt_update_all ();
  SDL_Delay (150);

  // fill the whole screen with background color
  avt_fill (screen, avt.background_color);

  // restore image
  SDL_SetClipRect (screen, &window);
  SDL_BlitSurface (oldwindowimage, NULL, screen, &window);
  SDL_FreeSurface (oldwindowimage);

  // make visible again
  avt_update_all ();

  // restore the clipping
  if (avt.textfield.x >= 0)
    SDL_SetClipRect (screen, &avt.viewport);
}

extern void
avt_toggle_fullscreen (void)
{
  if (avt.mode != AVT_FULLSCREENNOSWITCH)
    {
      // toggle bit for fullscreenmode
      screenflags = screenflags xor SDL_FULLSCREEN;

      if ((screenflags bitand SDL_FULLSCREEN) != 0)
	{
	  screenflags = screenflags bitor SDL_NOFRAME;
	  avt_resize (window.w, window.h);
	  avt.mode = AVT_FULLSCREEN;
	}
      else
	{
	  screenflags = screenflags bitand compl SDL_NOFRAME;
	  avt_resize (windowmode_size.w, windowmode_size.h);
	  avt.mode = AVT_WINDOW;
	}
    }
}

// switch to fullscreen or window mode
extern void
avt_switch_mode (int mode)
{
  if (screen and mode != avt.mode)
    {
      avt.mode = mode;
      switch (mode)
	{
	case AVT_FULLSCREENNOSWITCH:
	case AVT_FULLSCREEN:
	  if ((screenflags bitand SDL_FULLSCREEN) == 0)
	    {
	      screenflags =
		screenflags bitor SDL_FULLSCREEN bitor SDL_NOFRAME;
	      avt_resize (window.w, window.h);
	    }
	  break;

	case AVT_WINDOW:
	  if ((screenflags bitand SDL_FULLSCREEN) != 0)
	    {
	      screenflags =
		screenflags bitand compl (SDL_FULLSCREEN bitor SDL_NOFRAME);
	      avt_resize (windowmode_size.w, windowmode_size.h);
	    }
	  break;
	}
    }
}

extern int
avt_get_mode (void)
{
  return avt.mode;
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
      if (avt.reserve_single_keys)
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
      if (avt.reserve_single_keys)
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

  if (avt.ext_keyhandler)
    avt.ext_keyhandler (key.sym, key.mod, key.unicode);
}

static void
avt_call_mouse_handler (SDL_Event * event)
{
  int x, y;

  if (avt.textfield.x >= 0)
    {
      // if there is a textfield, use the character position
      x = (event->button.x - avt.textfield.x) / fontwidth + 1;
      y = (event->button.y - avt.textfield.y) / LINEHEIGHT + 1;

      // check if x and y are valid
      if (x >= 1 and x <= AVT_LINELENGTH
	  and y >= 1 and y <= (avt.textfield.h / LINEHEIGHT))
	avt.ext_mousehandler (event->button.button,
			      (event->button.state == SDL_PRESSED), x, y);
    }
  else				// no textfield
    {
      x = event->button.x - window.x;
      y = event->button.y - window.y;
      if (x >= 0 and x <= window.w and y >= 0 and y <= window.h)
	avt.ext_mousehandler (event->button.button,
			      (event->button.state == SDL_PRESSED), x, y);
    }
}

static bool
avt_check_buttons (int x, int y)
{
  struct avt_button *button;

  x -= window.x;
  y -= window.y;

  for (int nr = 0; nr < MAX_BUTTONS; nr++)
    {
      button = &avt_buttons[nr];

      if (button->background
	  and (y >= button->y) and (y <= (button->y + BASE_BUTTON_HEIGHT))
	  and (x >= button->x) and (x <= (button->x + BASE_BUTTON_WIDTH)))
	{
	  avt_push_key (button->key);
	  return true;
	}
    }

  return false;
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

    case SDL_MOUSEBUTTONDOWN:
      if (event->button.button <= 3)
	{
	  if (not avt_check_buttons (event->button.x, event->button.y))
	    {
	      if (avt.pointer_button_key)
		avt_push_key (avt.pointer_button_key);
	    }
	}
      else if (SDL_BUTTON_WHEELDOWN == event->button.button)
	avt_push_key (AVT_KEY_DOWN);
      else if (SDL_BUTTON_WHEELUP == event->button.button)
	avt_push_key (AVT_KEY_UP);

      if (avt.ext_mousehandler)
	avt_call_mouse_handler (event);
      break;

    case SDL_MOUSEBUTTONUP:
      if (avt.ext_mousehandler)
	avt_call_mouse_handler (event);
      break;

    case SDL_MOUSEMOTION:
      if (avt.pointer_motion_key)
	avt_push_key (avt.pointer_motion_key);
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

static inline int
avt_checkevent (void)
{
  SDL_Event event;

  while (SDL_PollEvent (&event))
    avt_analyze_event (&event);

  return _avt_STATUS;
}

// checks for events
extern int
avt_update (void)
{
  if (screen)
    avt_checkevent ();

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
	      SDL_SetError ("AddTimer doesn't work");
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

extern int
avt_where_x (void)
{
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.origin_mode)
	return ((avt.cursor.x - avt.viewport.x) / fontwidth) + 1;
      else
	return ((avt.cursor.x - avt.textfield.x) / fontwidth) + 1;
    }
  else
    return -1;
}

extern int
avt_where_y (void)
{
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.origin_mode)
	return ((avt.cursor.y - avt.viewport.y) / LINEHEIGHT) + 1;
      else
	return ((avt.cursor.y - avt.textfield.y) / LINEHEIGHT) + 1;
    }
  else
    return -1;
}

extern bool
avt_home_position (void)
{
  if (not screen or avt.textfield.x < 0)
    return true;		// about to be set to home position
  else
    return (avt.cursor.y == avt.viewport.y and avt.cursor.x == avt.linestart);
}

// this always means the full textfield
extern int
avt_get_max_x (void)
{
  if (screen)
    return avt.balloonwidth;
  else
    return -1;
}

// this always means the full textfield
extern int
avt_get_max_y (void)
{
  if (screen)
    return avt.balloonheight;
  else
    return -1;
}

extern void
avt_move_x (int x)
{
  SDL_Rect area;

  if (screen and avt.textfield.x >= 0)
    {
      if (x < 1)
	x = 1;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.cursor.x = (x - 1) * fontwidth + area.x;

      // max-pos exeeded?
      if (avt.cursor.x > area.x + area.w - fontwidth)
	avt.cursor.x = area.x + area.w - fontwidth;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_move_y (int y)
{
  SDL_Rect area;

  if (screen and avt.textfield.x >= 0)
    {
      if (y < 1)
	y = 1;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.cursor.y = (y - 1) * LINEHEIGHT + area.y;

      // max-pos exeeded?
      if (avt.cursor.y > area.y + area.h - LINEHEIGHT)
	avt.cursor.y = area.y + area.h - LINEHEIGHT;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_move_xy (int x, int y)
{
  SDL_Rect area;

  if (screen and avt.textfield.x >= 0)
    {
      if (x < 1)
	x = 1;

      if (y < 1)
	y = 1;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (false);

      if (avt.origin_mode)
	area = avt.viewport;
      else
	area = avt.textfield;

      avt.cursor.x = (x - 1) * fontwidth + area.x;
      avt.cursor.y = (y - 1) * LINEHEIGHT + area.y;

      // max-pos exeeded?
      if (avt.cursor.x > area.x + area.w - fontwidth)
	avt.cursor.x = area.x + area.w - fontwidth;

      if (avt.cursor.y > area.y + area.h - LINEHEIGHT)
	avt.cursor.y = area.y + area.h - LINEHEIGHT;

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_save_position (void)
{
  avt.saved_position = avt.cursor;
}

extern void
avt_restore_position (void)
{
  avt.cursor = avt.saved_position;
}

extern void
avt_insert_spaces (int num)
{
  SDL_Rect rest, dest;

  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  // get the rest of the viewport
  rest.x = avt.cursor.x;
  rest.w =
    avt.viewport.w - (avt.cursor.x - avt.viewport.x) - (num * fontwidth);
  rest.y = avt.cursor.y;
  rest.h = LINEHEIGHT;

  dest.x = avt.cursor.x + (num * fontwidth);
  dest.y = avt.cursor.y;
  SDL_BlitSurface (screen, &rest, screen, &dest);

  avt_fill_area (screen, avt.cursor.x, avt.cursor.y,
		 num * fontwidth, LINEHEIGHT, avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  // update line
  if (not avt.hold_updates)
    avt_update_area (avt.viewport.x, avt.cursor.y, avt.viewport.w,
		     fontheight);
}

extern void
avt_delete_characters (int num)
{
  SDL_Rect rest, dest;

  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  // get the rest of the viewport
  rest.x = avt.cursor.x + (num * fontwidth);
  rest.w =
    avt.viewport.w - (avt.cursor.x - avt.viewport.x) - (num * fontwidth);
  rest.y = avt.cursor.y;
  rest.h = LINEHEIGHT;

  dest.x = avt.cursor.x;
  dest.y = avt.cursor.y;
  SDL_BlitSurface (screen, &rest, screen, &dest);

  avt_fill_area (screen, avt.viewport.x + avt.viewport.w - (num * fontwidth),
		 avt.cursor.y, num * fontwidth, LINEHEIGHT,
		 avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  // update line
  if (not avt.hold_updates)
    avt_update_area (avt.viewport.x, avt.cursor.y, avt.viewport.w,
		     fontheight);
}

extern void
avt_erase_characters (int num)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  int x = (avt.textdir_rtl) ? avt.cursor.x - (num * fontwidth) : avt.cursor.x;

  avt_fill_area (screen, x, avt.cursor.y, num * fontwidth, LINEHEIGHT,
		 avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  // update area
  if (not avt.hold_updates)
    avt_update_area (x, avt.cursor.y, num * fontwidth, LINEHEIGHT);
}

extern void
avt_delete_lines (int line, int num)
{
  SDL_Rect rest, dest;

  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (not avt.origin_mode)
    line -= (avt.viewport.y - avt.textfield.y) / LINEHEIGHT;

  // check if values are sane
  if (line < 1 or num < 1 or line > (avt.viewport.h / LINEHEIGHT))
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  // get the rest of the viewport
  rest.x = avt.viewport.x;
  rest.w = avt.viewport.w;
  rest.y = avt.viewport.y + ((line - 1 + num) * LINEHEIGHT);
  rest.h = avt.viewport.h - ((line - 1 + num) * LINEHEIGHT);

  dest.x = avt.viewport.x;
  dest.y = avt.viewport.y + ((line - 1) * LINEHEIGHT);
  SDL_BlitSurface (screen, &rest, screen, &dest);

  avt_fill_area (screen, avt.viewport.x,
		 avt.viewport.y + avt.viewport.h - (num * LINEHEIGHT),
		 avt.viewport.w, num * LINEHEIGHT, avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  avt_update_trect (avt.viewport);
}

extern void
avt_insert_lines (int line, int num)
{
  SDL_Rect rest, dest;

  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return;

  if (not avt.origin_mode)
    line -= (avt.viewport.y - avt.textfield.y) / LINEHEIGHT;

  // check if values are sane
  if (line < 1 or num < 1 or line > (avt.viewport.h / LINEHEIGHT))
    return;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  // get the rest of the viewport
  rest.x = avt.viewport.x;
  rest.w = avt.viewport.w;
  rest.y = avt.viewport.y + ((line - 1) * LINEHEIGHT);
  rest.h = avt.viewport.h - ((line - 1 + num) * LINEHEIGHT);

  dest.x = avt.viewport.x;
  dest.y = avt.viewport.y + ((line - 1 + num) * LINEHEIGHT);
  SDL_BlitSurface (screen, &rest, screen, &dest);

  avt_fill_area (screen, avt.viewport.x,
		 avt.viewport.y + ((line - 1) * LINEHEIGHT),
		 avt.viewport.w, num * LINEHEIGHT, avt.text_background_color);

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  avt_update_trect (avt.viewport);
}

extern void
avt_viewport (int x, int y, int width, int height)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt.viewport.x = avt.textfield.x + ((x - 1) * fontwidth);
  avt.viewport.y = avt.textfield.y + ((y - 1) * LINEHEIGHT);
  avt.viewport.w = width * fontwidth;
  avt.viewport.h = height * LINEHEIGHT;

  avt.linestart =
    (avt.textdir_rtl) ? avt.viewport.x + avt.viewport.w -
    fontwidth : avt.viewport.x;

  avt.cursor.x = avt.linestart;
  avt.cursor.y = avt.viewport.y;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  if (avt.origin_mode)
    SDL_SetClipRect (screen, &avt.viewport);
  else
    SDL_SetClipRect (screen, &avt.textfield);
}

extern void
avt_newline_mode (bool mode)
{
  avt.newline_mode = mode;
}


extern bool
avt_get_newline_mode (void)
{
  return avt.newline_mode;
}

extern void
avt_set_auto_margin (bool mode)
{
  avt.auto_margin = mode;
}

extern bool
avt_get_auto_margin (void)
{
  return avt.auto_margin;
}

extern void
avt_set_origin_mode (bool mode)
{
  SDL_Rect area;

  avt.origin_mode = mode;

  if (avt.text_cursor_visible and avt.textfield.x >= 0)
    avt_show_text_cursor (false);

  if (avt.origin_mode)
    area = avt.viewport;
  else
    area = avt.textfield;

  avt.linestart = (avt.textdir_rtl) ? area.x + area.w - fontwidth : area.x;

  // cursor to position 1,1
  // when origin mode is off, then it may be outside the viewport (sic)
  avt.cursor.x = avt.linestart;
  avt.cursor.y = area.y;

  // reset saved position
  avt.saved_position = avt.cursor;

  if (avt.text_cursor_visible and avt.textfield.x >= 0)
    avt_show_text_cursor (true);

  if (avt.textfield.x >= 0)
    SDL_SetClipRect (screen, &area);
}

extern bool
avt_get_origin_mode (void)
{
  return avt.origin_mode;
}

extern void
avt_clear (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  // use background color of characters
  SDL_FillRect (screen, &avt.viewport, avt.text_background_color);

  avt.cursor.x = avt.linestart;

  if (avt.origin_mode)
    avt.cursor.y = avt.viewport.y;
  else
    avt.cursor.y = avt.textfield.y;

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  avt_update_trect (avt.viewport);
}

extern void
avt_clear_up (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  avt_fill_area (screen, avt.viewport.x, avt.viewport.y + fontheight,
		 avt.viewport.w, avt.cursor.y, avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    avt_update_area (avt.viewport.x, avt.viewport.y + fontheight,
		     avt.viewport.w, avt.cursor.y);
}

extern void
avt_clear_down (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt_fill_area (screen, avt.viewport.x, avt.cursor.y, avt.viewport.w,
		 avt.viewport.h - (avt.cursor.y - avt.viewport.y),
		 avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    avt_update_area (avt.viewport.x, avt.cursor.y, avt.viewport.w,
		     avt.viewport.h - (avt.cursor.y - avt.viewport.y));
}

extern void
avt_clear_eol (void)
{
  int x, width;

  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.textdir_rtl)		// right to left
    {
      x = avt.viewport.x;
      width = avt.cursor.x + fontwidth - avt.viewport.x;
    }
  else				// left to right
    {
      x = avt.cursor.x;
      width = avt.viewport.w - (avt.cursor.x - avt.viewport.x);
    }

  avt_fill_area (screen, x, avt.cursor.y, width, fontheight,
		 avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    avt_update_area (x, avt.cursor.y, width, fontheight);
}

// clear beginning of line
extern void
avt_clear_bol (void)
{
  int x, width;

  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  if (avt.textdir_rtl)		// right to left
    {
      x = avt.cursor.x;
      width = avt.viewport.w - (avt.cursor.x - avt.viewport.x);
    }
  else				// left to right
    {
      x = avt.viewport.x;
      width = avt.cursor.x + fontwidth - avt.viewport.x;
    }

  avt_fill_area (screen, x, avt.cursor.y, width, fontheight,
		 avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    avt_update_area (x, avt.cursor.y, width, fontheight);
}

extern void
avt_clear_line (void)
{
  // not initialized? -> do nothing
  if (not screen)
    return;

  // if there's no balloon, draw it
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  avt_fill_area (screen, avt.viewport.x, avt.cursor.y,
		 avt.viewport.w, fontheight, avt.text_background_color);

  if (avt.text_cursor_visible)
    {
      avt.text_cursor_actually_visible = false;
      avt_show_text_cursor (true);
    }

  if (not avt.hold_updates)
    avt_update_area (avt.viewport.x, avt.cursor.y, avt.viewport.w,
		     fontheight);
}

extern int
avt_flip_page (void)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return _avt_STATUS;

  // do nothing when the textfield is already empty
  if (avt.cursor.x == avt.linestart and avt.cursor.y == avt.viewport.y)
    return _avt_STATUS;

  /* the viewport must be updated,
     if it's not updated letter by letter */
  if (not avt.text_delay)
    avt_update_trect (avt.viewport);

  avt_wait (avt.flip_page_delay);
  avt_clear ();
  return _avt_STATUS;
}

static void
avt_scroll_up (void)
{
  switch (avt.scroll_mode)
    {
    case -1:
      // move cursor outside of balloon
      avt.cursor.y += LINEHEIGHT;
      break;
    case 1:
      avt_delete_lines (((avt.viewport.y - avt.textfield.y) / LINEHEIGHT) + 1,
			1);

      if (avt.origin_mode)
	avt.cursor.y = avt.viewport.y + avt.viewport.h - LINEHEIGHT;
      else
	avt.cursor.y = avt.textfield.y + avt.textfield.h - LINEHEIGHT;

      if (avt.newline_mode)
	avt.cursor.x = avt.linestart;
      break;
    case 0:
      avt_flip_page ();
      break;
    }
}

static void
avt_carriage_return (void)
{
  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  avt.cursor.x = avt.linestart;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);
}

extern int
avt_new_line (void)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return _avt_STATUS;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (false);

  if (avt.newline_mode)
    avt.cursor.x = avt.linestart;

  /* if the cursor is at the last line of the viewport
   * scroll up
   */
  if (avt.cursor.y == avt.viewport.y + avt.viewport.h - LINEHEIGHT)
    avt_scroll_up ();
  else
    avt.cursor.y += LINEHEIGHT;

  if (avt.text_cursor_visible)
    avt_show_text_cursor (true);

  return _avt_STATUS;
}

// avt_drawchar: draws the raw char - with no interpretation
// surface must be 32 bit per pixel
static void
avt_drawchar (avt_char ch, SDL_Surface * surface)
{
  const uint8_t *font_line;
  uint16_t line;

  font_line = (const uint8_t *) avt_get_font_char ((int) ch);

  if (not font_line)
    font_line = (const uint8_t *) avt_get_font_char (0);

  // fill with background color
  avt_fill_area (surface, avt.cursor.x, avt.cursor.y,
		 fontwidth, fontheight, avt.text_background_color);

  for (int y = 0; y < fontheight; y++)
    {
      if (fontwidth > 8)
	{
	  line = *(const uint16_t *) font_line;
	  font_line += 2;
	}
      else
	{
	  line = *font_line << 8;
	  font_line++;
	}

      if (avt.underlined and y == fontunderline)
	line = 0xFFFF;

      if (avt.inverse)
	line = compl line;

      for (int x = 0; x < fontwidth; x++)
	if (line bitand (1 << (15 - x)))
	  {
	    avt_putpixel (surface, avt.cursor.x + x,
			  avt.cursor.y + y, avt.text_color);

	    if (avt.bold and not NOT_BOLD)
	      avt_putpixel (surface, avt.cursor.x + x + 1,
			    avt.cursor.y + y, avt.text_color);
	  }
    }				// for (int y...
}

#ifndef DISABLE_DEPRECATED
extern void
avt_get_font_size (int *width, int *height)
{
  *width = fontwidth ? fontwidth : FONTWIDTH;
  *height = fontheight ? fontheight : FONTHEIGHT;
}
#endif

extern bool
avt_is_printable (avt_char ch)
{
  return (bool) (avt_get_font_char ((int) ch) != NULL);
}

// make current char visible
static void
avt_showchar (void)
{
  if (not avt.hold_updates)
    {
      avt_update_area (avt.cursor.x, avt.cursor.y, fontwidth, fontheight);
      avt.text_cursor_actually_visible = false;
    }
}

// advance position - only in the textfield
extern int
avt_forward (void)
{
  // no textfield? do nothing
  if (not screen or avt.textfield.x < 0)
    return _avt_STATUS;

  avt.cursor.x =
    (avt.textdir_rtl) ? avt.cursor.x - fontwidth : avt.cursor.x + fontwidth;

  if (avt.text_cursor_visible)
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
  if (screen and avt.textfield.x >= 0 and avt.auto_margin)
    {
      if (avt.cursor.x < avt.viewport.x
	  or avt.cursor.x > avt.viewport.x + avt.viewport.w - fontwidth)
	{
	  if (not avt.newline_mode)
	    avt_carriage_return ();
	  avt_new_line ();
	}

      if (avt.text_cursor_visible)
	avt_show_text_cursor (true);
    }
}

extern void
avt_reset_tab_stops (void)
{
  for (int i = 0; i < AVT_LINELENGTH; i++)
    if (i % 8 == 0)
      avt.tab_stops[i] = true;
    else
      avt.tab_stops[i] = false;
}

extern void
avt_clear_tab_stops (void)
{
  SDL_memset (&avt.tab_stops, false, sizeof (avt.tab_stops));
}

extern void
avt_set_tab (int x, bool onoff)
{
  avt.tab_stops[x - 1] = onoff;
}

// advance to next tabstop
extern void
avt_next_tab (void)
{
  int x;
  int i;

  // here we count zero based
  x = avt_where_x () - 1;

  if (avt.textdir_rtl)		// right to left
    {
      for (i = x; i >= 0; i--)
	{
	  if (avt.tab_stops[i])
	    break;
	}
      avt_move_x (i);
    }
  else				// left to right
    {
      for (i = x + 1; i < AVT_LINELENGTH; i++)
	{
	  if (avt.tab_stops[i])
	    break;
	}
      avt_move_x (i + 1);
    }
}

// go to last tabstop
extern void
avt_last_tab (void)
{
  int x;
  int i;

  // here we count zero based
  x = avt_where_x () - 1;

  if (avt.textdir_rtl)		// right to left
    {
      for (i = x; i < AVT_LINELENGTH; i++)
	{
	  if (avt.tab_stops[i])
	    break;
	}
      avt_move_x (i);
    }
  else				// left to right
    {
      for (i = x + 1; i >= 0; i--)
	{
	  if (avt.tab_stops[i])
	    break;
	}
      avt_move_x (i + 1);
    }
}

static void
avt_clearchar (void)
{
  avt_fill_area (screen, avt.cursor.x, avt.cursor.y, fontwidth, fontheight,
		 avt.text_background_color);
  avt_showchar ();
}

extern void
avt_backspace (void)
{
  if (screen and avt.textfield.x >= 0)
    {
      if (avt.cursor.x != avt.linestart)
	{
	  if (avt.text_cursor_visible)
	    avt_show_text_cursor (false);

	  avt.cursor.x =
	    (avt.textdir_rtl) ? avt.cursor.x + fontwidth : avt.cursor.x -
	    fontwidth;
	}

      if (avt.text_cursor_visible)
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
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  switch (ch)
    {
    case 0x000A:		// LF: Line Feed
    case 0x000B:		// VT: Vertical Tab
    case 0x0085:		// NEL: NExt Line
    case 0x2028:		// LS: Line Separator
    case 0x2029:		// PS: Paragraph Separator
      avt_new_line ();
      break;

    case 0x000D:		// CR: Carriage Return
      avt_carriage_return ();
      break;

    case 0x000C:		// FF: Form Feed
      avt_flip_page ();
      break;

    case 0x0009:		// HT: Horizontal Tab
      avt_next_tab ();
      break;

    case 0x0008:		// BS: Back Space
      avt_backspace ();
      break;

    case 0x0007:		// BEL
      bell ();
      break;

      /* ignore BOM here
       * must be handled outside of the library
       */
    case 0xFEFF:
      break;

      // LRM/RLM: only supported at the beginning of a line
    case 0x200E:		// LEFT-TO-RIGHT MARK (LRM)
      avt_text_direction (AVT_LEFT_TO_RIGHT);
      break;

    case 0x200F:		// RIGHT-TO-LEFT MARK (RLM)
      avt_text_direction (AVT_RIGHT_TO_LEFT);
      break;

      // other ignorable (invisible) characters
    case 0x200B:
    case 0x200C:
    case 0x200D:
      break;

    case 0x0020:		// SP: space
      if (avt.auto_margin)
	check_auto_margin ();
      if (not avt.underlined and not avt.inverse)
	avt_clearchar ();
      else			// underlined or inverse
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
      if (ch > 0x0020 or ch == 0x0000)
	{
	  if (avt.markup and ch == 0x005F)	// '_'
	    avt.underlined = not avt.underlined;
	  else if (avt.markup and ch == 0x002A)	// '*'
	    avt.bold = not avt.bold;
	  else			// not a markup character
	    {
	      if (avt.auto_margin)
		check_auto_margin ();
	      avt_drawchar (ch, screen);
	      avt_showchar ();
	      if (avt.text_delay)
		SDL_Delay (avt.text_delay);
	      avt_forward ();
	      avt_checkevent ();
	    }			// if not markup
	}			// if (ch > 0x0020)
    }				// switch

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

  // check if all conditions are met
  if (not * txt or not * (txt + 1) or not * (txt + 2) or * (txt + 1) != L'\b')
    r = -1;
  else
    {
      if (*txt == L'_')
	{
	  avt.underlined = true;
	  if (avt_put_char ((avt_char) * (txt + 2)))
	    r = -1;
	  avt.underlined = false;
	}
      else if (*txt == *(txt + 2))
	{
	  avt.bold = true;
	  if (avt_put_char ((avt_char) * txt))
	    r = -1;
	  avt.bold = false;
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
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // nothing to do, when there is no text 
  if (not txt or not * txt)
    return avt_checkevent ();

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  while (*txt)
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
	  if (0xD800 <= *txt and * txt <= 0xDBFF)	// UTF-16 high surrogate
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
  // nothing to do, when txt == NULL
  // but do allow a text to start with zeros here
  if (not screen or not txt or _avt_STATUS != AVT_NORMAL)
    return avt_checkevent ();

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  for (size_t i = 0; i < len; i++, txt++)
    {
      if (*(txt + 1) == L'\b' and i < len - 1)
	{
	  if (avt_overstrike (txt))
	    break;
	  txt += 2;
	  i += 2;
	}
      else
	{
	  avt_char c = (avt_char) * txt;
	  if (0xD800 <= *txt and * txt <= 0xDBFF)	// UTF-16 high surrogate
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

  if (not txt or not * txt or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  width = 0;
  height = 1;
  line_length = 0;
  pos = 1;
  p = txt;

  while ((len > 0 and pos <= len) or (len == 0 and * p))
    {
      switch (*p)
	{
	case L'\n':
	case L'\v':
	case L'\x0085':	// NEL: NExt Line
	case L'\x2028':	// LS: Line Separator
	case L'\x2029':	// PS: Paragraph Separator
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
	  // FIXME
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
	  // no width
	  break;

	  // '_' is tricky because of overstrike
	  // must be therefore before '*'
	case L'_':
	  if (avt.markup)
	    {
	      if (*(p + 1) == L'\b')
		line_length++;
	      break;
	    }
	  // else fall through

	case L'*':
	  if (avt.markup)
	    break;
	  // else fall through

	default:
	  if ((*p >= 32 or * p == 0) and (*p < 0xD800 or * p > 0xDBFF))
	    {
	      line_length++;
	      if (avt.auto_margin and line_length > AVT_LINELENGTH)
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

  if (len)
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
  if (not encoding)
    encoding = "";

  /*
   * check if it is the result of avt_get_mb_encoding()
   * or the same encoding
   */
  if (encoding == avt.encoding or SDL_strcmp (encoding, avt.encoding) == 0)
    return _avt_STATUS;

  SDL_strlcpy (avt.encoding, encoding, sizeof (avt.encoding));

  // if encoding is "" and SYSTEMENCODING is not ""
  if (encoding[0] == '\0' and SYSTEMENCODING[0] != '\0')
    encoding = SYSTEMENCODING;

  // output

  //  if it is already open, close it first
  if (output_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (output_cd);

  // initialize the conversion framework
  output_cd = avt_iconv_open (WCHAR_ENCODING, encoding);

  // check if it was successfully initialized
  if (output_cd == ICONV_UNINITIALIZED)
    {
      SDL_SetError ("encoding \"%s\" not supported for output", encoding);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // input

  //  if it is already open, close it first
  if (input_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (input_cd);

  // initialize the conversion framework
  input_cd = avt_iconv_open (encoding, WCHAR_ENCODING);

  // check if it was successfully initialized
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
  return avt.encoding;
}

// size in bytes
// returns length (number of characters)
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

  // check if sizes are useful
  if (not dest or not dest_size or not src or not src_size)
    return (size_t) (-1);

  // check if encoding was set
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = src_size;
  inbuf = (char *) src;

  // leave room for terminator
  outbytesleft = dest_size - sizeof (wchar_t);
  outbuf = (char *) dest;

  restbuf = (char *) rest_buffer;

  // if there is a rest from last call, try to complete it
  while (rest_bytes > 0 and inbytesleft > 0)
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

  // do the conversion
  returncode =
    avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  // handle invalid characters
  while (returncode == (size_t) (-1) and errno == EILSEQ)
    {
      inbuf++;
      inbytesleft--;

      *((wchar_t *) outbuf) = L'\xFFFD';

      outbuf += sizeof (wchar_t);
      outbytesleft -= sizeof (wchar_t);
      returncode =
	avt_iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  // check for incomplete sequences and put them into the rest_buffer
  if (returncode == (size_t) (-1) and errno == EINVAL
      and inbytesleft <= sizeof (rest_buffer))
    {
      rest_bytes = inbytesleft;
      SDL_memcpy ((void *) &rest_buffer, inbuf, rest_bytes);
    }

  // ignore E2BIG - just put in as much as fits

  // terminate outbuf
  *((wchar_t *) outbuf) = L'\0';

  return ((dest_size - sizeof (wchar_t) - outbytesleft) / sizeof (wchar_t));
}

// size in bytes
// dest must be freed by caller
extern size_t
avt_mb_decode (wchar_t ** dest, const char *src, size_t src_size)
{
  size_t dest_size;
  size_t length;

  if (not dest)
    return (size_t) (-1);

  *dest = NULL;

  if (not src or not src_size)
    return (size_t) (-1);

  // get enough space
  // a character may be 4 Bytes, also in UTF-16
  // plus the terminator
  dest_size = src_size * 4 + sizeof (wchar_t);

  // minimal string size
  if (dest_size < 8)
    dest_size = 8;

  *dest = (wchar_t *) SDL_malloc (dest_size);

  if (not * dest)
    return (size_t) (-1);

  length = avt_mb_decode_buffer (*dest, dest_size, src, src_size);

  if (length == (size_t) (-1) or length == 0)
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

  // check if sizes are useful
  if (not dest or not dest_size or not src or not len)
    return (size_t) (-1);

  // check if encoding was set
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = len * sizeof (wchar_t);
  inbuf = (char *) src;

  // leave room for terminator
  outbytesleft = dest_size - sizeof (char);
  outbuf = dest;

  // do the conversion
  (void) avt_iconv (input_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
  // ignore errors

  // terminate outbuf
  *outbuf = '\0';

  return (dest_size - sizeof (char) - outbytesleft);
}

extern size_t
avt_mb_encode (char **dest, const wchar_t * src, size_t len)
{
  size_t dest_size, size;

  if (not dest)
    return (size_t) (-1);

  *dest = NULL;

  // check if len is useful
  if (not src or not len)
    return (size_t) (-1);

  // get enough space
  // UTF-8 may need 4 bytes per character
  // +1 for the terminator
  dest_size = len * 4 + 1;
  *dest = (char *) SDL_malloc (dest_size);

  if (not * dest)
    return (size_t) (-1);

  size = avt_mb_encode_buffer (*dest, dest_size, src, len);

  if (size == (size_t) (-1) or size == 0)
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

  // check if sizes are useful
  if (not dest or not dest_size or not src or not src_size)
    return (size_t) (-1);

  // NULL as code means the encoding, which was set

  if (not tocode)
    tocode = avt.encoding;

  if (not fromcode)
    fromcode = avt.encoding;

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

  // do the conversion
  returncode = avt_iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  // jump over invalid characters
  while (returncode == (size_t) (-1) and errno == EILSEQ)
    {
      inbuf++;
      inbytesleft--;
      returncode =
	avt_iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  // ignore E2BIG - just put in as much as fits

  avt_iconv_close (cd);

  // terminate outbuf (4 Bytes were reserved)
  SDL_memset (outbuf, 0, 4);

  return (dest_size - 4 - outbytesleft);
}

// dest must be freed by the caller
extern size_t
avt_recode (const char *tocode, const char *fromcode,
	    char **dest, const char *src, size_t src_size)
{
  avt_iconv_t cd;
  char *outbuf, *inbuf;
  size_t dest_size;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  if (not dest)
    return (size_t) (-1);

  *dest = NULL;

  // check if size is useful
  if (not src or not src_size)
    return (size_t) (-1);

  // NULL as code means the encoding, which was set

  if (not tocode)
    tocode = avt.encoding;

  if (not fromcode)
    fromcode = avt.encoding;

  cd = avt_iconv_open (tocode, fromcode);
  if (cd == (avt_iconv_t) (-1))
    return (size_t) (-1);

  inbuf = (char *) src;
  inbytesleft = src_size;

  /*
   * I reserve 4 Bytes for the terminator,
   * in case of using UTF-32
   */

  // guess it's the same size
  dest_size = src_size + 4;
  *dest = (char *) SDL_malloc (dest_size);

  if (*dest == NULL)
    {
      avt_iconv_close (cd);
      return -1;
    }

  outbuf = *dest;
  outbytesleft = dest_size - 4;	// reserve 4 Bytes for terminator

  // do the conversion
  while (inbytesleft > 0)
    {
      returncode =
	avt_iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

      // check for fatal errors
      if (returncode == (size_t) (-1))
	switch (errno)
	  {
	  case E2BIG:		// needs more memory
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
	      // reserve 4 bytes for terminator again
	    }
	    break;

	  case EILSEQ:
	    inbuf++;		// jump over invalid chars bytewise
	    inbytesleft--;
	    break;

	  case EINVAL:		// incomplete sequence
	  default:
	    inbytesleft = 0;	// cannot continue
	    break;
	  }
    }

  avt_iconv_close (cd);

  // terminate outbuf (4 Bytes were reserved)
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
  if (screen and _avt_STATUS == AVT_NORMAL)
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

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // check if encoding was set
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  if (len)
    inbytesleft = len;
  else
    inbytesleft = SDL_strlen (txt);

  inbuf = (char *) txt;

  // if there is a rest from last call, try to complete it
  while (rest_bytes > 0 and rest_bytes < sizeof (rest_buffer)
	 and inbytesleft > 0)
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

      if (nconv != (size_t) (-1))	// no error
	{
	  avt_say_len (wctext,
		       (sizeof (wctext) - outbytesleft) / sizeof (wchar_t));
	  rest_bytes = 0;
	}
      else if (err != EINVAL)	// any error, but incomplete sequence
	{
	  avt_put_char (0xFFFD);	// broken character
	  rest_bytes = 0;
	}
    }

  // convert and display the text
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
	  if (err == EILSEQ)	// broken character
	    {
	      avt_put_char (0xFFFD);
	      inbuf++;
	      inbytesleft--;
	    }
	  else if (err == EINVAL)	// incomplete sequence
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

  if (screen and _avt_STATUS == AVT_NORMAL)
    {
      if (not len)
	len = SDL_strlen (txt);

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
  if (screen and _avt_STATUS == AVT_NORMAL)
    avt_tell_mb_len (txt, SDL_strlen (txt));

  return _avt_STATUS;
}

extern void
avt_clear_keys (void)
{
  avt_keys.position = avt_keys.end = 0;
}

#define avt_clear_keys(void)  avt_keys.position = avt_keys.end = 0

extern bool
avt_key_pressed (void)
{
  return (avt_keys.position != avt_keys.end);
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

static avt_char
avt_set_pointer_motion_key (avt_char key)
{
  avt_char old;

  old = avt.pointer_motion_key;
  avt.pointer_motion_key = key;

  if (key)
    SDL_EventState (SDL_MOUSEMOTION, SDL_ENABLE);
  else
    SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);

  return old;
}

// key for pointer buttons 1-3
static avt_char
avt_set_pointer_buttons_key (avt_char key)
{
  avt_char old;

  old = avt.pointer_button_key;
  avt.pointer_button_key = key;

  return old;
}

static inline void
avt_get_pointer_position (int *x, int *y)
{
  SDL_GetMouseState (x, y);
}

static void
update_menu_bar (int menu_start, int menu_end, int line_nr, int old_line,
		 SDL_Surface * plain_menu, SDL_Surface * bar)
{
  SDL_Rect s, t;

  if (line_nr != old_line)
    {
      // restore oldline
      if (old_line >= menu_start and old_line <= menu_end)
	{
	  s.x = 0;
	  s.y = (old_line - 1) * LINEHEIGHT;
	  s.w = avt.viewport.w;
	  s.h = LINEHEIGHT;
	  t.x = avt.viewport.x;
	  t.y = avt.viewport.y + s.y;
	  SDL_BlitSurface (plain_menu, &s, screen, &t);
	  avt_update_area (t.x, t.y, avt.viewport.w, LINEHEIGHT);
	}

      // show bar
      if (line_nr >= menu_start and line_nr <= menu_end)
	{
	  t.x = avt.viewport.x;
	  t.y = avt.viewport.y + ((line_nr - 1) * LINEHEIGHT);
	  SDL_BlitSurface (bar, NULL, screen, &t);
	  avt_update_area (t.x, t.y, avt.viewport.w, LINEHEIGHT);
	}
    }
}

extern int
avt_choice (int *result, int start_line, int items, int key,
	    bool back, bool forward)
{
  SDL_Surface *plain_menu, *bar;
  SDL_Color barcolor;
  int last_key;
  int end_line;
  int line_nr, old_line;

  if (screen and _avt_STATUS == AVT_NORMAL)
    {
      // get a copy of the viewport
      plain_menu = avt_save_background (avt.viewport);

      // prepare transparent bar
      bar = SDL_CreateRGBSurface (SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL,
				  avt.viewport.w, LINEHEIGHT, 8, 0, 0, 0,
				  128);

      if (not bar)
	{
	  SDL_FreeSurface (plain_menu);
	  SDL_SetError ("out of memory");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}

      // set color for bar and make it transparent
      barcolor = avt_sdlcolor (avt.cursor_color);
      avt_fill (bar, 0);
      SDL_SetColors (bar, &barcolor, 0, 1);
      SDL_SetAlpha (bar, SDL_SRCALPHA | SDL_RLEACCEL, 128);

      end_line = start_line + items - 1;

      if (key)
	last_key = key + items - 1;
      else
	last_key = 0;

      line_nr = -1;
      old_line = 0;
      *result = -1;

      avt_clear_keys ();
      avt_set_pointer_motion_key (0xF802);
      avt_set_pointer_buttons_key (AVT_KEY_ENTER);

      while ((*result == -1) and (_avt_STATUS == AVT_NORMAL))
	{
	  avt_char ch;

	  avt_key (&ch);

	  if (key and (ch >= key) and (ch <= last_key))
	    *result = (int) (ch - key + 1);
	  else if (AVT_KEY_DOWN == ch)
	    {
	      if (line_nr != end_line)
		{
		  if (line_nr < start_line or line_nr > end_line)
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
	  else if (AVT_KEY_UP == ch)
	    {
	      if (line_nr != start_line)
		{
		  if (line_nr < start_line or line_nr > end_line)
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
	  else if (back and (AVT_KEY_PAGEUP == ch))
	    *result = 1;
	  else if (forward and (AVT_KEY_PAGEDOWN == ch))
	    *result = items;
	  else if ((AVT_KEY_ENTER == ch or AVT_KEY_RIGHT == ch)
		   and line_nr >= start_line and line_nr <= end_line)
	    *result = line_nr - start_line + 1;
	  else if (0xF802 == ch)	// mouse motion
	    {
	      int x, y;
	      avt_get_pointer_position (&x, &y);

	      if (x >= avt.viewport.x
		  and x <= avt.viewport.x + avt.viewport.w
		  and y >= avt.viewport.y + ((start_line - 1) * LINEHEIGHT)
		  and y < avt.viewport.y + (end_line * LINEHEIGHT))
		line_nr = ((y - avt.viewport.y) / LINEHEIGHT) + 1;

	      if (line_nr != old_line)
		{
		  update_menu_bar (start_line, end_line, line_nr, old_line,
				   plain_menu, bar);
		  old_line = line_nr;
		}
	    }
	}			// while

      avt_set_pointer_motion_key (0);
      avt_set_pointer_buttons_key (0);
      avt_clear_keys ();

      SDL_FreeSurface (plain_menu);
      SDL_FreeSurface (bar);
    }

  return _avt_STATUS;
}

extern void
avt_lock_updates (bool lock)
{
  avt.hold_updates = lock;

  // side effect: set text_delay to 0
  if (avt.hold_updates)
    avt.text_delay = 0;

  // if hold_updates is not set update the textfield
  avt_update_trect (avt.textfield);
}

static void
avt_button_inlay (SDL_Rect btn_rect, const unsigned char *bits,
		  int width, int height, int color)
{
  SDL_Surface *inlay;
  SDL_Rect inlay_rect;
  int radius;

  radius = btn_rect.w / 2;
  inlay = avt_load_image_xbm (bits, width, height, color);
  inlay_rect.w = inlay->w;
  inlay_rect.h = inlay->h;
  inlay_rect.x = btn_rect.x + radius - (inlay_rect.w / 2);
  inlay_rect.y = btn_rect.y + radius - (inlay_rect.h / 2);
  SDL_BlitSurface (inlay, NULL, screen, &inlay_rect);
  SDL_FreeSurface (inlay);
}

// coordinates are relative to window
static void
avt_show_button (int x, int y, enum avt_button_type type,
		 avt_char key, int color)
{
  SDL_Rect btn_rect;
  int buttonnr;
  struct avt_button *button;

  // find free button number
  buttonnr = 0;
  while (buttonnr < MAX_BUTTONS and avt_buttons[buttonnr].background)
    buttonnr++;

  if (buttonnr == MAX_BUTTONS)
    {
      SDL_SetError ("too many buttons");
      _avt_STATUS = AVT_ERROR;
      return;
    }

  button = &avt_buttons[buttonnr];

  button->x = x;
  button->y = y;
  button->key = key;

  btn_rect.x = x + window.x;
  btn_rect.y = y + window.y;
  btn_rect.w = BASE_BUTTON_WIDTH;
  btn_rect.h = BASE_BUTTON_HEIGHT;

  SDL_SetClipRect (screen, &window);

  button->background = avt_save_background (btn_rect);

  SDL_BlitSurface (base_button, NULL, screen, &btn_rect);

  switch (type)
    {
    case btn_cancel:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_cancel), color);
      break;

    case btn_yes:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_yes), color);
      break;

    case btn_no:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_no), color);
      break;

    case btn_right:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_right), color);
      break;

    case btn_left:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_left), color);
      break;

    case btn_up:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_up), color);
      break;

    case btn_down:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_down), color);
      break;

    case btn_fastforward:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_fastforward), color);
      break;

    case btn_fastbackward:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_fastbackward), color);
      break;

    case btn_stop:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_stop), color);
      break;

    case btn_pause:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_pause), color);
      break;

    case btn_help:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_help), color);
      break;

    case btn_eject:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_eject), color);
      break;

    case btn_circle:
      avt_button_inlay (btn_rect, AVT_XBM_INFO (btn_circle), color);
      break;
    }

  avt_update_rect (btn_rect);
}

static void
avt_clear_buttons (void)
{
  SDL_Rect btn_rect;
  struct avt_button *button;

  SDL_SetClipRect (screen, &window);

  for (int nr = 0; nr < MAX_BUTTONS; nr++)
    {
      button = &avt_buttons[nr];

      btn_rect.x = button->x + window.x;
      btn_rect.y = button->y + window.y;
      btn_rect.w = BASE_BUTTON_WIDTH;
      btn_rect.h = BASE_BUTTON_HEIGHT;

      if (button->background)
	{
	  SDL_BlitSurface (button->background, NULL, screen, &btn_rect);
	  avt_update_rect (btn_rect);
	  SDL_FreeSurface (button->background);
	  button->background = NULL;
	}

      button->x = button->y = 0;
      button->key = 0;
    }
}

static inline bool
avt_is_linebreak (wchar_t c)
{
  return (c == L'\n' or c == L'\v' or c == L'\x85'
	  or c == L'\x2028' or c == L'\x2029');
}

// checks for formfeed or text separators
static inline bool
avt_is_pagebreak (wchar_t c)
{
  return (c == L'\f' or (c >= L'\x1C' and c <= L'\x1F'));
}

// checks for invisible characters (including '\a'!)
static inline bool
avt_is_invisible (wchar_t c)
{
  return (c == L'\a'
	  or c == L'\xFEFF' or c == L'\x200E' or c == L'\x200F'
	  or c == L'\x200B' or c == L'\x200C' or c == L'\x200D');
}

static size_t
avt_pager_line (const wchar_t * txt, size_t pos, size_t len,
		size_t horizontal)
{
  const wchar_t *tpos;

  tpos = txt + pos;

  avt.underlined = avt.bold = false;

  // handle pagebreaks
  if (avt_is_pagebreak (*tpos))
    {
      // draw separator line
      for (int i = AVT_LINELENGTH; i > 0; i--)
	avt_put_char (0x2550);

      tpos++;
      pos++;

      // evtl. skip line end
      while (pos < len and (txt[pos] == L'\r' or avt_is_linebreak (txt[pos])))
	pos++;

      return pos;
    }

  // search for linebreak or pagebreak or end of text
  const wchar_t *p = tpos;
  size_t line_length = 0;

  while (pos + line_length < len and not avt_is_linebreak (*p)
	 and not avt_is_pagebreak (*p))
    {
      line_length++;
      p++;
    }

  pos += line_length;

  // skip linebreak, but not a pagebreak
  if (pos < len and avt_is_linebreak (*p))
    pos++;

  // speedup horizontal scrolling
  horizontal *= 4;

  if (line_length > horizontal)
    {
      // skip horizontal characters, iff visible
      for (size_t i = 0; i < horizontal; i++)
	{
	  // FIXME: find solution for tabulators

	  // skip invisible characters
	  while (line_length and avt_is_invisible (*tpos))
	    {
	      tpos++;
	      line_length--;
	    }

	  // handle backspace (used for overstrike text)
	  while (line_length > 2 and tpos[1] == L'\b')
	    {
	      tpos += 2;
	      line_length -= 2;
	    }

	  // hande markup mode
	  while (avt.markup and (*tpos == L'_' or * tpos == L'*'))
	    {
	      if (*tpos == L'_')
		avt.underlined = not avt.underlined;
	      else if (*tpos == L'*')
		avt.bold = not avt.bold;

	      tpos++;
	      line_length--;
	    }

	  if (not line_length)
	    break;

	  tpos++;
	  line_length--;
	}

      if (line_length)
	avt_say_len (tpos, line_length);
    }

  return pos;
}

static size_t
avt_pager_screen (const wchar_t * txt, size_t pos, size_t len,
		  size_t horizontal)
{
  avt.hold_updates = true;
  SDL_FillRect (screen, &avt.textfield, avt.text_background_color);

  for (int line_nr = 0; line_nr < avt.balloonheight; line_nr++)
    {
      avt.cursor.x = avt.linestart;
      avt.cursor.y = line_nr * LINEHEIGHT + avt.textfield.y;
      pos = avt_pager_line (txt, pos, len, horizontal);
    }

  avt.hold_updates = false;
  avt_update_trect (avt.textfield);

  return pos;
}

static size_t
avt_pager_lines_back (const wchar_t * txt, size_t pos, int lines)
{
  if (pos > 0)
    pos--;			// go to last linebreak

  lines--;

  while (lines--)
    {
      if (pos > 0 and avt_is_linebreak (txt[pos]))
	pos--;			// go before last linebreak

      while (pos > 0 and not avt_is_linebreak (txt[pos]))
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
  size_t horizontal;
  struct avt_settings old_settings;
  bool quit;
  void (*old_alert_func) (void);
  struct avt_position button;

  if (not screen)
    return AVT_ERROR;

  // do we actually have something to show?
  if (not txt or not * txt or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // get len if not given
  if (len == 0)
    len = avt_strwidth (txt);

  horizontal = 0;

  // find startline
  pos = 0;
  if (startline > 1)
    {
      int nr;

      nr = startline - 1;
      while (nr > 0 and pos < len)
	{
	  while (pos < len and not avt_is_linebreak (txt[pos]))
	    pos++;
	  pos++;
	  nr--;
	}
    }

  // last screen
  if (pos >= len)
    pos = avt_pager_lines_back (txt, len, avt.balloonheight + 1);

  if (avt.textfield.x < 0)
    avt_draw_balloon ();
  else
    avt.viewport = avt.textfield;

  // show close-button

  // alignment: right bottom
  button.x = window.w - BASE_BUTTON_WIDTH - AVATAR_MARGIN;
  button.y = window.h - BASE_BUTTON_HEIGHT - AVATAR_MARGIN;

  // the button shouldn't be clipped
  if (button.y + window.y < avt.textfield.y + avt.textfield.h
      and button.x + window.x < avt.textfield.x + avt.textfield.w)
    button.x = avt.textfield.x + avt.textfield.w - window.x;
  // this is a workaround: moving it down clashed with a bug in SDL

  avt_show_button (button.x, button.y, btn_cancel,
		   AVT_KEY_ESCAPE, AVT_BUTTON_COLOR);

  // limit to viewport (else more problems with binary files
  SDL_SetClipRect (screen, &avt.viewport);

  old_settings = avt;

  avt.text_cursor_visible = false;
  avt.auto_margin = false;
  avt.reserve_single_keys = false;
  avt.ext_keyhandler = NULL;
  avt.ext_mousehandler = NULL;

  // temporarily disable the alert function
  old_alert_func = avt_alert_func;
  avt_alert_func = NULL;

  avt_set_text_delay (0);
  if (avt.markup)
    {
      avt_normal_text ();
      avt.markup = true;
    }
  else
    avt_normal_text ();

  // show first screen
  pos = avt_pager_screen (txt, pos, len, horizontal);

  // last screen
  if (pos >= len)
    {
      pos = avt_pager_lines_back (txt, len, avt.balloonheight + 1);
      pos = avt_pager_screen (txt, pos, len, horizontal);
    }

  quit = false;
  avt_clear_keys ();

  while (not quit and _avt_STATUS == AVT_NORMAL)
    {
      avt_char ch;
      avt_key (&ch);

      switch (ch)
	{
	case AVT_KEY_ESCAPE:	// needed for the button
	  quit = true;
	  break;

	case AVT_KEY_DOWN:
	  if (pos < len)	// if it's not the end
	    {
	      avt.hold_updates = true;
	      avt_delete_lines (1, 1);
	      avt.cursor.x = avt.linestart;
	      avt.cursor.y =
		(avt.balloonheight - 1) * LINEHEIGHT + avt.textfield.y;
	      pos = avt_pager_line (txt, pos, len, horizontal);
	      avt.hold_updates = false;
	      avt_update_trect (avt.textfield);
	    }
	  break;

	case AVT_KEY_UP:
	  {
	    int start_pos;

	    start_pos =
	      avt_pager_lines_back (txt, pos, avt.balloonheight + 2);

	    if (start_pos == 0)
	      pos = avt_pager_screen (txt, 0, len, horizontal);
	    else
	      {
		avt.hold_updates = true;
		avt_insert_lines (1, 1);
		avt.cursor.x = avt.linestart;
		avt.cursor.y = avt.textfield.y;
		avt_pager_line (txt, start_pos, len, horizontal);
		avt.hold_updates = false;
		avt_update_trect (avt.textfield);
		pos = avt_pager_lines_back (txt, pos, 2);
	      }
	  }
	  break;

	case AVT_KEY_PAGEDOWN:
	case L' ':
	case L'f':
	  if (pos < len)
	    {
	      pos = avt_pager_screen (txt, pos, len, horizontal);
	      if (pos >= len)
		{
		  pos =
		    avt_pager_lines_back (txt, len, avt.balloonheight + 1);
		  pos = avt_pager_screen (txt, pos, len, horizontal);
		}
	    }
	  break;

	case AVT_KEY_PAGEUP:
	case L'b':
	  pos = avt_pager_lines_back (txt, pos, 2 * avt.balloonheight + 1);
	  pos = avt_pager_screen (txt, pos, len, horizontal);
	  break;

	case AVT_KEY_HOME:
	  horizontal = 0;
	  pos = avt_pager_screen (txt, 0, len, horizontal);
	  break;

	case AVT_KEY_END:
	  pos = avt_pager_lines_back (txt, len, avt.balloonheight + 1);
	  pos = avt_pager_screen (txt, pos, len, horizontal);
	  break;

	case L'q':
	  quit = true;		// Q with any combination-key quits
	  break;

	case AVT_KEY_RIGHT:
	  horizontal++;
	  pos = avt_pager_lines_back (txt, pos, avt.balloonheight + 1);
	  pos = avt_pager_screen (txt, pos, len, horizontal);
	  break;

	case AVT_KEY_LEFT:
	  if (horizontal)
	    {
	      horizontal--;
	      pos = avt_pager_lines_back (txt, pos, avt.balloonheight + 1);
	      pos = avt_pager_screen (txt, pos, len, horizontal);
	    }
	  break;
	}			// switch (ch)
    }				// while

  // quit request only quits the pager here
  if (_avt_STATUS == AVT_QUIT)
    _avt_STATUS = AVT_NORMAL;

  avt_clear_buttons ();

  SDL_SetClipRect (screen, &avt.viewport);

  avt_alert_func = old_alert_func;
  avt = old_settings;
  avt_activate_cursor (avt.text_cursor_visible);

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

  if (screen and txt and _avt_STATUS == AVT_NORMAL)
    {
      if (not len)
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

// size in Bytes!
extern int
avt_ask (wchar_t * s, size_t size)
{
  avt_char ch;
  size_t len, maxlen, pos;
  int old_textdir;
  bool insert_mode;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // this function only works with left to right text
  // it would be too hard otherwise
  old_textdir = avt.textdir_rtl;
  avt.textdir_rtl = AVT_LEFT_TO_RIGHT;

  // no textfield? => draw balloon
  if (avt.textfield.x < 0)
    avt_draw_balloon ();

  /* if the cursor is beyond the end of the viewport,
   * get a new page
   */
  if (avt.cursor.y > avt.viewport.y + avt.viewport.h - LINEHEIGHT)
    avt_flip_page ();

  // maxlen is the rest of line
  if (avt.textdir_rtl)
    maxlen = (avt.cursor.x - avt.viewport.x) / fontwidth;
  else
    maxlen = ((avt.viewport.x + avt.viewport.w) - avt.cursor.x) / fontwidth;

  // does it fit in the buffer size?
  if (maxlen > size / sizeof (wchar_t) - 1)
    maxlen = size / sizeof (wchar_t) - 1;

  // clear the input field
  avt_erase_characters (maxlen);

  len = pos = 0;
  insert_mode = true;
  SDL_memset (s, 0, size);
  ch = 0;

  do
    {
      // show cursor
      avt_show_text_cursor (true);

      if (avt_key (&ch) != AVT_NORMAL)
	break;

      switch (ch)
	{
	case AVT_KEY_ENTER:
	  break;

	case AVT_KEY_HOME:
	  avt_show_text_cursor (false);
	  avt.cursor.x -= pos * fontwidth;
	  pos = 0;
	  break;

	case AVT_KEY_END:
	  avt_show_text_cursor (false);
	  if (len < maxlen)
	    {
	      avt.cursor.x += (len - pos) * fontwidth;
	      pos = len;
	    }
	  else
	    {
	      avt.cursor.x += (maxlen - 1 - pos) * fontwidth;
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
	      // delete cursor
	      avt_show_text_cursor (false);
	      avt.cursor.x -= fontwidth;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_RIGHT:
	  if (pos < len and pos < maxlen - 1)
	    {
	      pos++;
	      avt_show_text_cursor (false);
	      avt.cursor.x += fontwidth;
	    }
	  else
	    bell ();
	  break;

	case AVT_KEY_INSERT:
	  insert_mode = not insert_mode;
	  break;

	default:
	  if (pos < maxlen and ch >= 32 and avt_get_font_char (ch) != NULL)
	    {
	      // delete cursor
	      avt_show_text_cursor (false);
	      if (insert_mode and pos < len)
		{
		  avt_insert_spaces (1);
		  if (len < maxlen)
		    {
		      SDL_memmove (&s[pos + 1], &s[pos],
				   (len - pos) * sizeof (wchar_t));
		      len++;
		    }
		  else		// len >= maxlen
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
		  pos--;	// cursor stays where it is
		  bell ();
		}
	      else
		avt.cursor.x =
		  (avt.textdir_rtl) ? avt.cursor.x -
		  fontwidth : avt.cursor.x + fontwidth;
	    }
	}
    }
  while ((ch != AVT_KEY_ENTER) and (_avt_STATUS == AVT_NORMAL));

  s[len] = L'\0';

  // delete cursor
  avt_show_text_cursor (false);

  if (not avt.newline_mode)
    avt_carriage_return ();

  avt_new_line ();

  avt.textdir_rtl = old_textdir;

  return _avt_STATUS;
}

extern int
avt_ask_mb (char *s, size_t size)
{
  wchar_t ws[AVT_LINELENGTH + 1];
  char *inbuf;
  size_t inbytesleft, outbytesleft;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // check if encoding was set
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  avt_ask (ws, sizeof (ws));

  s[0] = '\0';

  // if a halt is requested, don't bother with the conversion
  if (_avt_STATUS)
    return _avt_STATUS;

  // prepare the buffer
  inbuf = (char *) &ws;
  inbytesleft = (avt_strwidth (ws) + 1) * sizeof (wchar_t);
  outbytesleft = size;

  // do the conversion
  avt_iconv (input_cd, &inbuf, &inbytesleft, &s, &outbytesleft);

  return _avt_STATUS;
}

extern int
avt_move_in (void)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // fill the screen with background color
  // (not only the window!)
  avt_clear_screen ();

  if (avt.avatar_image)
    {
      SDL_Rect dst;
      int16_t destination;
      uint32_t start_time;
      SDL_Rect mywindow;

      /*
       * mywindow is like window,
       * but to the edge of the screen on the right
       */
      mywindow = window;
      mywindow.w = screen->w - mywindow.x;

      dst.x = screen->w;

      if (AVT_HEADER == avt.avatar_mode)
	dst.y = mywindow.y + TOPMARGIN;
      else			// bottom
	dst.y = mywindow.y + mywindow.h - avt.avatar_image->h - AVATAR_MARGIN;

      dst.w = avt.avatar_image->w;
      dst.h = avt.avatar_image->h;
      start_time = SDL_GetTicks ();

      if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	destination = ((window.x + window.w) / 2) - (avt.avatar_image->w / 2);
      else			// left
	destination = window.x + AVATAR_MARGIN;

      while (dst.x > destination)
	{
	  int16_t oldx = dst.x;

	  // move
	  dst.x = screen->w - ((SDL_GetTicks () - start_time) / MOVE_DELAY);

	  if (dst.x != oldx)
	    {
	      // draw
	      SDL_BlitSurface (avt.avatar_image, NULL, screen, &dst);

	      // update
	      if ((oldx + dst.w) >= screen->w)
		avt_update_area (dst.x, dst.y, screen->w - dst.x, dst.h);
	      else
		avt_update_area (dst.x, dst.y, dst.w + (oldx - dst.x), dst.h);

	      // if window is resized then break
	      if (window.x != mywindow.x or window.y != mywindow.y)
		break;

	      // delete (not visibly yet)
	      SDL_FillRect (screen, &dst, avt.background_color);
	    }

	  // check event
	  if (avt_checkevent ())
	    return _avt_STATUS;

	  // some time for other processes
	  SDL_Delay (1);
	}

      // final position
      avt_show_avatar ();
    }

  return _avt_STATUS;
}

extern int
avt_move_out (void)
{
  if (not screen or not avt.avatar_visible or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // needed to remove the balloon
  avt_show_avatar ();

  /*
   * remove clipping
   */
  SDL_SetClipRect (screen, NULL);

  if (avt.avatar_image)
    {
      SDL_Rect dst;
      uint32_t start_time;
      int16_t start_position;
      SDL_Rect mywindow;

      /*
       * mywindow is like window,
       * but to the edge of the screen on the right
       */
      mywindow = window;
      mywindow.w = screen->w - mywindow.x;

      if (AVT_FOOTER == avt.avatar_mode or AVT_HEADER == avt.avatar_mode)
	start_position =
	  ((window.x + window.w) / 2) - (avt.avatar_image->w / 2);
      else
	start_position = mywindow.x + AVATAR_MARGIN;

      dst.x = start_position;

      if (AVT_HEADER == avt.avatar_mode)
	dst.y = mywindow.y + TOPMARGIN;
      else			// bottom
	dst.y = mywindow.y + mywindow.h - avt.avatar_image->h - AVATAR_MARGIN;

      dst.w = avt.avatar_image->w;
      dst.h = avt.avatar_image->h;
      start_time = SDL_GetTicks ();

      // delete (not visibly yet)
      SDL_FillRect (screen, &dst, avt.background_color);

      while (dst.x < screen->w)
	{
	  int16_t oldx;

	  oldx = dst.x;

	  // move
	  dst.x =
	    start_position + ((SDL_GetTicks () - start_time) / MOVE_DELAY);

	  if (dst.x != oldx)
	    {
	      // draw
	      SDL_BlitSurface (avt.avatar_image, NULL, screen, &dst);

	      // update
	      if ((dst.x + dst.w) >= screen->w)
		avt_update_area (oldx, dst.y, screen->w - oldx, dst.h);
	      else
		avt_update_area (oldx, dst.y, dst.w + dst.x - oldx, dst.h);

	      // if window is resized then break
	      if (window.x != mywindow.x or window.y != mywindow.y)
		break;

	      // delete (not visibly yet)
	      SDL_FillRect (screen, &dst, avt.background_color);
	    }

	  // check event
	  if (avt_checkevent ())
	    return _avt_STATUS;

	  // some time for other processes
	  SDL_Delay (1);
	}
    }

  // fill the whole screen with background color
  avt_clear_screen ();

  return _avt_STATUS;
}

extern int
avt_wait_button (void)
{
  avt_char old_buttons_key, old_motion_key;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // alignment: right bottom
  avt_show_button (window.w - BASE_BUTTON_WIDTH - AVATAR_MARGIN,
		   window.h - BASE_BUTTON_HEIGHT - AVATAR_MARGIN,
		   btn_right, AVT_KEY_ENTER, AVT_BUTTON_COLOR);

  old_motion_key = avt_set_pointer_motion_key (0);	// ignore moves
  old_buttons_key = avt_set_pointer_buttons_key (AVT_KEY_ENTER);

  avt_clear_keys ();
  avt_key (NULL);

  avt_clear_buttons ();
  avt_set_pointer_motion_key (old_motion_key);
  avt_set_pointer_buttons_key (old_buttons_key);
  avt_clear_keys ();

  if (avt.textfield.x >= 0)
    SDL_SetClipRect (screen, &avt.viewport);

  return _avt_STATUS;
}

/*
 * maximum number of displayable navigation buttons
 * for the smallest resolution
 */
#define NAV_MAX MAX_BUTTONS

extern int
avt_navigate (const char *buttons)
{
  int button_count;
  avt_char audio_end_button, old_audio_key;
  struct avt_position button;
  int result;

  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (not screen)
    return AVT_ERROR;

  result = AVT_ERROR;		// no result
  audio_end_button = old_audio_key = 0;	// none
  button_count = SDL_strlen (buttons);

  if (not buttons or not * buttons or button_count > NAV_MAX)
    {
      SDL_SetError ("No or too many buttons for navigation bar");
      return AVT_FAILURE;
    }

  // display buttons:

  SDL_SetClipRect (screen, &window);

  button.x = window.w - AVATAR_MARGIN
    - (button_count * (BASE_BUTTON_WIDTH + BUTTON_DISTANCE)) +
    BUTTON_DISTANCE;

  button.y = window.h - BASE_BUTTON_HEIGHT - AVATAR_MARGIN;

  for (int i = 0; i < button_count; i++)
    {
      switch (buttons[i])
	{
	case 'l':
	  avt_show_button (button.x, button.y, btn_left, L'l',
			   AVT_BUTTON_COLOR);
	  break;

	case 'd':
	  avt_show_button (button.x, button.y, btn_down, L'd',
			   AVT_BUTTON_COLOR);
	  break;

	case 'u':
	  avt_show_button (button.x, button.y, btn_up, L'u',
			   AVT_BUTTON_COLOR);
	  break;

	case 'r':
	  avt_show_button (button.x, button.y, btn_right, L'r',
			   AVT_BUTTON_COLOR);
	  break;

	case 'x':
	  avt_show_button (button.x, button.y, btn_cancel, L'x',
			   AVT_BUTTON_COLOR);
	  break;

	case 's':
	  avt_show_button (button.x, button.y, btn_stop, L's',
			   AVT_BUTTON_COLOR);
	  if (not audio_end_button)	// 'f' has precedence
	    audio_end_button = 's';
	  break;

	case 'f':
	  avt_show_button (button.x, button.y, btn_fastforward, L'f',
			   AVT_BUTTON_COLOR);
	  audio_end_button = 'f';	// this has precedence
	  break;

	case 'b':
	  avt_show_button (button.x, button.y, btn_fastbackward, L'b',
			   AVT_BUTTON_COLOR);
	  break;

	case '+':
	  avt_show_button (button.x, button.y, btn_yes, L'+',
			   AVT_BUTTON_COLOR);
	  break;

	case '-':
	  avt_show_button (button.x, button.y, btn_no, L'-',
			   AVT_BUTTON_COLOR);
	  break;

	case 'p':
	  avt_show_button (button.x, button.y, btn_pause, L'p',
			   AVT_BUTTON_COLOR);
	  break;

	case '?':
	  avt_show_button (button.x, button.y, btn_help, L'?',
			   AVT_BUTTON_COLOR);
	  break;

	case 'e':
	  avt_show_button (button.x, button.y, btn_eject, L'e',
			   AVT_BUTTON_COLOR);
	  break;

	case '*':
	  avt_show_button (button.x, button.y, btn_circle, L'*',
			   AVT_BUTTON_COLOR);
	  break;

	default:
	  /*
	   * empty button allowed
	   * for compatibility to buttons in later versions
	   */
	  break;
	}

      button.x += BASE_BUTTON_WIDTH + BUTTON_DISTANCE;
    }

  if (audio_end_button)
    old_audio_key = avt_set_audio_end_key (audio_end_button);

  // check button presses

  while (result < 0 and _avt_STATUS == AVT_NORMAL)
    {
      avt_char ch;
      avt_key (&ch);

      switch (ch)
	{
	case AVT_KEY_UP:
	case AVT_KEY_HOME:
	  ch = L'u';
	  break;

	case AVT_KEY_DOWN:
	case AVT_KEY_END:
	  ch = L'd';
	  break;

	case AVT_KEY_LEFT:
	  ch = L'l';
	  break;

	case AVT_KEY_RIGHT:
	  ch = L'r';
	  break;

	case AVT_KEY_HELP:
	case AVT_KEY_F1:
	  ch = L'?';
	  break;

	case AVT_KEY_PAGEDOWN:
	  ch = L'f';
	  break;

	case AVT_KEY_PAGEUP:
	  ch = L'b';
	  break;
	}

      // check if it is one of the requested characters
      const char *b = buttons;
      while (*b)
	if (ch == (avt_char) * b++)
	  result = ch;
    }

  avt_clear_buttons ();
  avt_set_audio_end_key (old_audio_key);

  if (avt.textfield.x >= 0)
    SDL_SetClipRect (screen, &avt.viewport);

  if (_avt_STATUS != AVT_NORMAL)
    result = _avt_STATUS;

  return result;
}

extern bool
avt_decide (void)
{
  struct avt_position yes_button, no_button;
  int result;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  SDL_SetClipRect (screen, &window);

  // alignment: right bottom
  yes_button.x = window.w - BASE_BUTTON_WIDTH - AVATAR_MARGIN;
  yes_button.y = window.h - BASE_BUTTON_HEIGHT - AVATAR_MARGIN;

  no_button.x = yes_button.x - BUTTON_DISTANCE - BASE_BUTTON_WIDTH;
  no_button.y = yes_button.y;

  // draw buttons
  avt_show_button (yes_button.x, yes_button.y, btn_yes, L'+', 0x00AA00);
  avt_show_button (no_button.x, no_button.y, btn_no, L'-', 0xAA0000);

  avt_clear_keys ();
  result = -1;			// no result
  while (result < 0 and _avt_STATUS == AVT_NORMAL)
    {
      avt_char ch;
      avt_key (&ch);

      if (L'-' == ch or L'0' == ch or AVT_KEY_BACKSPACE == ch
	  or AVT_KEY_ESCAPE == ch)
	result = false;
      else if (L'+' == ch or L'1' == ch or AVT_KEY_ENTER == ch)
	result = true;
    }

  avt_clear_buttons ();
  avt_clear_keys ();

  if (avt.textfield.x >= 0)
    SDL_SetClipRect (screen, &avt.viewport);

  return (result > 0);
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

  // clear the screen
  avt_free_screen ();

  // set informational variables
  avt_no_textfield ();
  avt.avatar_visible = false;

  // center image on screen
  dst.x = (screen->w / 2) - (image->w / 2);
  dst.y = (screen->h / 2) - (image->h / 2);
  dst.w = image->w;
  dst.h = image->h;

  // eventually increase inner window - never decrease!
  if (dst.w > window.w)
    {
      window.w = (dst.w <= screen->w) ? dst.w : screen->w;
      window.x = (screen->w / 2) - (window.w / 2);
    }

  if (dst.h > window.h)
    {
      window.h = (dst.h <= screen->h) ? dst.h : screen->h;
      window.y = (screen->h / 2) - (window.h / 2);
    }

  /*
   * if image is larger than the screen,
   * just the upper left part is shown, as far as it fits
   */
  SDL_BlitSurface (image, NULL, screen, &dst);
  avt_update_all ();
  avt_checkevent ();
}


// RW is closed here
static int
avt_show_image_rw (SDL_RWops * RW)
{
  SDL_Surface *image;

  if (not RW)
    return AVT_FAILURE;

  image = NULL;

  // try internal XPM reader first
  // it's better than in SDL_image
  image = avt_load_image_xpm_RW (RW, 0);

  if (image == NULL)
    image = avt_load_image_xbm_RW (RW, 0, avt.bitmap_color);

  if (image == NULL)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);
    }

  SDL_RWclose (RW);

  if (image == NULL)
    {
      avt_clear_screen ();	// at least clear the screen
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


/*
 * load image
 * if SDL_image isn't available then
 * XPM and uncompressed BMP are still supported
 */
extern int
avt_show_image_file (const char *filename)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;
  else
    return avt_show_image_rw (SDL_RWFromFile (filename, "rb"));
}


extern int
avt_show_image_stream (avt_stream * stream)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;
  else
    return avt_show_image_rw (SDL_RWFromFP ((FILE *) stream, 0));
}


/*
 * show image from image data
 */
extern int
avt_show_image_data (void *img, size_t imgsize)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;
  else
    return avt_show_image_rw (SDL_RWFromMem (img, imgsize));
}


extern int
avt_show_image_xpm (char **xpm)
{
  SDL_Surface *image = NULL;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  image = avt_load_image_xpm (xpm);

  if (image == NULL)
    {
      avt_clear_screen ();	// at least clear the screen
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_show_image_xbm (const unsigned char *bits, int width, int height,
		    int color)
{
  SDL_Surface *image;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (width <= 0 or height <= 0 or color < 0)
    {
      avt_clear ();		// at least clear the balloon
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  image = avt_load_image_xbm (bits, width, height, color);

  if (not image)
    {
      avt_clear_screen ();	// at least clear the screen
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

static SDL_Surface *
avt_import_image (void *image_data, int width, int height,
		  int bytes_per_pixel)
{
  SDL_Surface *image;

  image = NULL;

  // the wrong endianess can be optimized away while compiling
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
  else				// little endian
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

  return image;
}

/*
 * show raw image
 * only 3 or 4 Bytes per pixel supported (RGB or RGBA)
 */
extern int
avt_show_raw_image (void *image_data, int width, int height,
		    int bytes_per_pixel)
{
  if (not screen or _avt_STATUS != AVT_NORMAL or not image_data)
    return _avt_STATUS;

  if (bytes_per_pixel < 3 or bytes_per_pixel > 4)
    {
      SDL_SetError ("wrong number of bytes_per_pixel for raw image");
      return AVT_FAILURE;
    }

  // check if it's a different image
  if (raw_image
      and (width != raw_image->w or height != raw_image->h
	   or image_data != raw_image->pixels))
    avt_release_raw_image ();

  if (not raw_image)
    raw_image = avt_import_image (image_data, width, height, bytes_per_pixel);

  if (not raw_image)
    {
      avt_clear_screen ();	// at least clear the screen
      SDL_SetError ("couldn't show image");
      return AVT_FAILURE;
    }

  avt_show_image (raw_image);

  return _avt_STATUS;
}


static int
avt_put_image_rw (SDL_RWops * RW, int x, int y, void *image_data,
		  int width, int height, int bytes_per_pixel)
{
  SDL_Surface *src, *dest;
  SDL_Rect destrect;

  if (not RW)
    return AVT_FAILURE;

  if (not screen or _avt_STATUS != AVT_NORMAL or not image_data)
    return _avt_STATUS;

  if (bytes_per_pixel < 3 or bytes_per_pixel > 4)
    {
      SDL_SetError ("wrong number of bytes_per_pixel for raw image");
      return AVT_FAILURE;
    }

  src = dest = NULL;

  src = avt_load_image_xpm_RW (RW, 0);

  if (not src)
    src = avt_load_image_xbm_RW (RW, 0, avt.bitmap_color);

  if (not src)
    {
      load_image_init ();
      src = load_image.rw (RW, 0);
    }

  SDL_RWclose (RW);

  if (not src)
    return AVT_FAILURE;

  dest = avt_import_image (image_data, width, height, bytes_per_pixel);

  if (not dest)
    {
      SDL_FreeSurface (src);
      SDL_SetError ("export_image");
      return AVT_FAILURE;
    }

  destrect.x = x;
  destrect.y = y;
  destrect.w = destrect.h = 0;	// ignored

  SDL_BlitSurface (src, NULL, dest, &destrect);

  SDL_FreeSurface (dest);
  SDL_FreeSurface (src);

  return _avt_STATUS;
}

extern int
avt_put_raw_image_file (const char *file, int x, int y,
			void *image_data, int width, int height,
			int bytes_per_pixel)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;
  else
    return avt_put_image_rw (SDL_RWFromFile (file, "rb"), x, y, image_data,
			     width, height, bytes_per_pixel);
}

extern int
avt_put_raw_image_stream (avt_stream * stream, int x, int y,
			  void *image_data, int width, int height,
			  int bytes_per_pixel)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;
  else
    return avt_put_image_rw (SDL_RWFromFP ((FILE *) stream, 0), x, y,
			     image_data, width, height, bytes_per_pixel);
}

extern int
avt_put_raw_image_data (void *img, size_t imgsize, int x, int y,
			void *image_data, int width, int height,
			int bytes_per_pixel)
{
  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;
  else
    return avt_put_image_rw (SDL_RWFromMem (img, imgsize), x, y, image_data,
			     width, height, bytes_per_pixel);
}

extern int
avt_put_raw_image_xpm (char **xpm, int x, int y,
		       void *image_data, int width, int height,
		       int bytes_per_pixel)
{
  SDL_Surface *src, *dest;
  SDL_Rect destrect;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  src = dest = NULL;

  src = avt_load_image_xpm (xpm);

  if (not src)
    return AVT_FAILURE;

  dest = avt_import_image (image_data, width, height, bytes_per_pixel);

  if (not dest)
    {
      SDL_FreeSurface (src);
      SDL_SetError ("export_image");
      return AVT_FAILURE;
    }

  destrect.x = x;
  destrect.y = y;
  destrect.w = destrect.h = 0;	// ignored

  SDL_BlitSurface (src, NULL, dest, &destrect);

  SDL_FreeSurface (dest);
  SDL_FreeSurface (src);

  return _avt_STATUS;
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
// deprecated, but needed internally
DEPRECATED_EXTERN avt_image_t *
avt_make_transparent (avt_image_t * image)
{
  int32_t color;

  if (SDL_MUSTLOCK (image))
    SDL_LockSurface (image);

  // get color of upper left corner
  color = avt_getpixel (image, 0, 0);

  if (SDL_MUSTLOCK (image))
    SDL_UnlockSurface (image);

  if (not SDL_SetColorKey (image, SDL_SRCCOLORKEY | SDL_RLEACCEL, color))
    image = NULL;

  return image;
}


#ifndef DISABLE_DEPRECATED

// deprecated
extern avt_image_t *
avt_import_xpm (char **xpm)
{
  if (avt_init_SDL ())
    return NULL;

  return avt_load_image_xpm (xpm);
}

// deprecated
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

// deprecated
extern avt_image_t *
avt_import_image_data (void *img, size_t imgsize)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (avt_init_SDL ())
    return NULL;

  image = NULL;
  RW = SDL_RWFromMem (img, imgsize);

  if (RW)
    {
      // try internal XPM reader first
      image = avt_load_image_xpm_RW (RW, 0);

      if (not image)
	image = avt_load_image_xbm_RW (RW, 0, avt.bitmap_color);

      if (not image)
	{
	  load_image_init ();
	  image = load_image.rw (RW, 0);

	  // if it's not yet transparent, make it transparent
	  if (image)
	    if (not (image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	      avt_make_transparent (image);
	}

      SDL_RWclose (RW);
    }

  return image;
}

// deprecated
extern avt_image_t *
avt_import_image_file (const char *filename)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (avt_init_SDL ())
    return NULL;

  image = NULL;
  RW = SDL_RWFromFile (filename, "rb");

  if (RW)
    {
      // try internal XPM reader first
      image = avt_load_image_xpm_RW (RW, 0);

      if (not image)
	image = avt_load_image_xbm_RW (RW, 0, avt.bitmap_color);

      if (not image)
	{
	  load_image_init ();
	  image = load_image.rw (RW, 0);

	  // if it's not yet transparent, make it transparent
	  if (image)
	    if (not (image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	      avt_make_transparent (image);
	}

      SDL_RWclose (RW);
    }

  return image;
}

// deprecated
extern avt_image_t *
avt_import_image_stream (avt_stream * stream)
{
  SDL_Surface *image;
  SDL_RWops *RW;

  if (avt_init_SDL ())
    return NULL;

  image = NULL;
  RW = SDL_RWFromFP ((FILE *) stream, 0);

  if (RW)
    {
      // try internal XPM reader first
      image = avt_load_image_xpm_RW (RW, 0);

      if (not image)
	image = avt_load_image_xbm_RW (RW, 0, avt.bitmap_color);

      if (not image)
	{
	  load_image_init ();
	  image = load_image.rw (RW, 0);

	  // if it's not yet transparent, make it transparent
	  if (image)
	    if (not (image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	      avt_make_transparent (image);
	}

      SDL_RWclose (RW);
    }

  return image;
}

#endif // DISABLE_DEPRECATED


// change avatar image and (re)calculate balloon size
static int
avt_set_avatar_image (SDL_Surface * image)
{
  if (_avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  if (avt.avatar_visible)
    avt_clear_screen ();

  // free old image
  if (avt.avatar_image)
    {
      SDL_FreeSurface (avt.avatar_image);
      avt.avatar_image = NULL;
    }

  if (avt.name)
    {
      SDL_free (avt.name);
      avt.name = NULL;
    }

  // import the avatar image
  if (image)
    {
      // convert image to display-format for faster drawing
      if (image->flags & SDL_SRCALPHA)
	avt.avatar_image = SDL_DisplayFormatAlpha (image);
      else
	avt.avatar_image = SDL_DisplayFormat (image);

      if (not avt.avatar_image)
	{
	  SDL_SetError ("couldn't load avatar");
	  _avt_STATUS = AVT_ERROR;
	  return _avt_STATUS;
	}
    }

  calculate_balloonmaxheight ();

  // set actual balloon size to the maximum size
  avt.balloonheight = avt.balloonmaxheight;
  avt.balloonwidth = AVT_LINELENGTH;

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

  if (not image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_xbm (const unsigned char *bits,
		      int width, int height, int color)
{
  SDL_Surface *image;

  if (width <= 0 or height <= 0 or color < 0)
    {
      SDL_SetError ("invalid parameters");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  image = avt_load_image_xbm (bits, width, height, color);

  if (not image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


// RW is closed here
static int
avt_avatar_image_rw (SDL_RWops * RW)
{
  SDL_Surface *image;

  if (not RW)
    return AVT_FAILURE;

  image = NULL;

  // try internal XPM reader first
  image = avt_load_image_xpm_RW (RW, 0);

  if (not image)
    image = avt_load_image_xbm_RW (RW, 0, avt.bitmap_color);

  if (not image)
    {
      load_image_init ();
      image = load_image.rw (RW, 0);

      // if it's not yet transparent, make it transparent
      if (image)
	if (not (image->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA)))
	  avt_make_transparent (image);
    }

  SDL_RWclose (RW);

  if (not image)
    return AVT_FAILURE;

  avt_set_avatar_image (image);
  SDL_FreeSurface (image);

  return _avt_STATUS;
}


extern int
avt_avatar_image_data (void *img, size_t imgsize)
{
  return avt_avatar_image_rw (SDL_RWFromMem (img, imgsize));
}


extern int
avt_avatar_image_file (const char *file)
{
  return avt_avatar_image_rw (SDL_RWFromFile (file, "rb"));
}


extern int
avt_avatar_image_stream (avt_stream * stream)
{
  return avt_avatar_image_rw (SDL_RWFromFP ((FILE *) stream, 0));
}


extern int
avt_set_avatar_name (const wchar_t * name)
{
  int size;

  // clear old name
  if (avt.name)
    {
      SDL_free (avt.name);
      avt.name = NULL;
    }

  // copy name
  if (name and * name)
    {
      size = (avt_strwidth (name) + 1) * sizeof (wchar_t);
      avt.name = (wchar_t *) SDL_malloc (size);
      SDL_memcpy (avt.name, name, size);
    }

  if (avt.avatar_visible)
    avt_show_avatar ();

  return _avt_STATUS;
}

extern int
avt_set_avatar_name_mb (const char *name)
{
  wchar_t *wcname;

  if (not name or not * name)
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
  avt.text_background_color = avt.ballooncolor;
}

// can and should be called before avt_initialize
extern void
avt_set_balloon_color (int color)
{
  if (color >= 0)
    {
      avt.ballooncolor = color;

      if (screen)
	{
	  avt_set_text_background_ballooncolor ();

	  // redraw the balloon, if it is visible
	  if (avt.textfield.x >= 0)
	    avt_draw_balloon ();
	}
    }
}

extern int
avt_get_balloon_color (void)
{
  return avt.ballooncolor;
}

// can and should be called before avt_initialize
extern void
avt_set_background_color (int color)
{
  if (color >= 0)
    {
      avt.backgroundcolornr = color;

      if (screen)
	{
	  avt.background_color =
	    SDL_MapRGB (screen->format, avt_red (color),
			avt_green (color), avt_blue (color));

	  if (avt.textfield.x >= 0)
	    {
	      avt.avatar_visible = false;	// force to redraw everything
	      avt_draw_balloon ();
	    }
	  else if (avt.avatar_visible)
	    avt_show_avatar ();
	  else
	    avt_clear_screen ();
	}
    }
}

extern int
avt_get_background_color (void)
{
  return avt.backgroundcolornr;
}

extern void
avt_set_bitmap_color (int color)
{
  avt.bitmap_color = color;
}

extern void
avt_reserve_single_keys (bool onoff)
{
  avt.reserve_single_keys = onoff;
}

extern void
avt_register_keyhandler (avt_keyhandler handler)
{
  avt.ext_keyhandler = handler;
}

extern void
avt_register_mousehandler (avt_mousehandler handler)
{
  avt.ext_mousehandler = handler;
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
avt_set_text_color (int colornr)
{
  avt.text_color = colornr;
}

extern void
avt_set_text_background_color (int colornr)
{
  avt.text_background_color = colornr;
}

extern void
avt_inverse (bool onoff)
{
  avt.inverse = onoff;
}

extern bool
avt_get_inverse (void)
{
  return avt.inverse;
}

extern void
avt_bold (bool onoff)
{
  avt.bold = onoff;
}

extern bool
avt_get_bold (void)
{
  return avt.bold;
}

extern void
avt_underlined (bool onoff)
{
  avt.underlined = onoff;
}

extern bool
avt_get_underlined (void)
{
  return avt.underlined;
}

extern void
avt_normal_text (void)
{
  avt.underlined = avt.bold = avt.inverse = avt.markup = false;

  avt.text_color = AVT_COLOR_BLACK;
  avt.text_background_color = avt.ballooncolor;
}

extern void
avt_markup (bool onoff)
{
  avt.markup = onoff;
  avt.underlined = avt.bold = false;
}

extern void
avt_set_text_delay (int delay)
{
  avt.text_delay = delay;

  // eventually switch off updates lock
  if (avt.text_delay != 0 and avt.hold_updates)
    avt_lock_updates (false);
}

extern void
avt_set_flip_page_delay (int delay)
{
  avt.flip_page_delay = delay;
}

extern void
avt_set_scroll_mode (int mode)
{
  avt.scroll_mode = mode;
}

extern int
avt_get_scroll_mode (void)
{
  return avt.scroll_mode;
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

#define CREDITDELAY 50

// scroll one line up
static void
avt_credits_up (SDL_Surface * last_line)
{
  SDL_Rect src, dst, line_pos;
  int32_t moved;
  uint32_t now, next_time;
  int32_t pixel;
  uint32_t tickinterval;

  moved = 0;
  pixel = 1;
  tickinterval = CREDITDELAY;
  next_time = SDL_GetTicks () + tickinterval;

  while (moved <= LINEHEIGHT)
    {
      // move screen up
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

      avt_update_rect (window);

      if (avt_checkevent ())
	return;

      moved += pixel;
      now = SDL_GetTicks ();

      if (next_time > now)
	SDL_Delay (next_time - now);
      else
	{
	  // move more pixels at once and give more time next time
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
  int old_backgroundcolornr;
  avt_keyhandler old_keyhandler;
  avt_mousehandler old_mousehandler;
  const wchar_t *p;
  int length;

  if (not screen or _avt_STATUS != AVT_NORMAL)
    return _avt_STATUS;

  // store old background color
  old_backgroundcolornr = avt.backgroundcolornr;

  // deactivate mous/key- handlers
  old_keyhandler = avt.ext_keyhandler;
  old_mousehandler = avt.ext_mousehandler;
  avt.ext_keyhandler = NULL;
  avt.ext_mousehandler = NULL;

  // needed to handle resizing correctly
  avt_no_textfield ();
  avt.avatar_visible = false;
  avt.hold_updates = false;

  // the background-color is used when the window is resized
  // this implicitly also clears the screen
  avt_set_background_color (AVT_COLOR_BLACK);
  avt_set_text_background_color (AVT_COLOR_BLACK);
  avt_set_text_color (AVT_COLOR_WHITE);

  window.x = (screen->w / 2) - (80 * fontwidth / 2);
  window.w = 80 * fontwidth;
  // horizontal values unchanged

  SDL_SetClipRect (screen, &window);

  // last line added to credits
  last_line =
    SDL_CreateRGBSurface (SDL_SWSURFACE,
			  window.w, LINEHEIGHT,
			  screen->format->BitsPerPixel,
			  screen->format->Rmask,
			  screen->format->Gmask,
			  screen->format->Bmask, screen->format->Amask);

  if (not last_line)
    {
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // cursor position for last_line
  avt.cursor.y = 0;

  // show text
  p = text;
  while (*p and _avt_STATUS == AVT_NORMAL)
    {
      // get line
      length = 0;
      while (*p and not avt_is_linebreak (*p))
	{
	  if (*p >= L' ' and length < 80)
	    {
	      line[length] = *p;
	      length++;
	    }

	  p++;
	}

      // skip linebreak
      p++;

      // draw line
      if (centered)
	avt.cursor.x = (window.w / 2) - (length * fontwidth / 2);
      else
	avt.cursor.x = 0;

      // clear line
      avt_fill (last_line, AVT_COLOR_BLACK);

      // print on last_line
      for (int i = 0; i < length; i++, avt.cursor.x += fontwidth)
	avt_drawchar ((avt_char) line[i], last_line);

      avt_credits_up (last_line);
    }

  // show one empty line to avoid streakes
  avt_fill (last_line, AVT_COLOR_BLACK);
  avt_credits_up (last_line);

  // scroll up until screen is empty
  for (int i = 0; i < window.h / LINEHEIGHT and _avt_STATUS == AVT_NORMAL;
       i++)
    avt_credits_up (NULL);

  SDL_FreeSurface (last_line);
  avt_avatar_window ();

  // back to normal (also sets variables!)
  avt_set_background_color (old_backgroundcolornr);
  avt_normal_text ();
  avt_clear_screen ();

  avt.ext_keyhandler = old_keyhandler;
  avt.ext_mousehandler = old_mousehandler;

  return _avt_STATUS;
}

extern int
avt_credits_mb (const char *txt, bool centered)
{
  wchar_t *wctext;

  if (screen and _avt_STATUS == AVT_NORMAL)
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
  avt_release_raw_image ();

  // close conversion descriptors
  if (output_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (output_cd);
  if (input_cd != ICONV_UNINITIALIZED)
    avt_iconv_close (input_cd);
  output_cd = input_cd = ICONV_UNINITIALIZED;

  avt.encoding[0] = '\0';

  if (screen)
    {
      SDL_FreeCursor (mpointer);
      mpointer = NULL;
      SDL_FreeSurface (circle);
      circle = NULL;
      SDL_FreeSurface (base_button);
      base_button = NULL;
      SDL_FreeSurface (avt.pointer);
      avt.pointer = NULL;
      SDL_FreeSurface (avt.avatar_image);
      avt.avatar_image = NULL;
      SDL_FreeSurface (avt.cursor_character);
      avt.cursor_character = NULL;
      avt_alert_func = NULL;
      SDL_Quit ();
      screen = NULL;		// it was freed by SDL_Quit
      avt.avatar_visible = false;
      avt.textfield.x = avt.textfield.y = avt.textfield.w = avt.textfield.h =
	-1;
      avt.viewport = avt.textfield;
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

// old SDL could only handle ASCII titles

extern void
avt_set_title (const char *title, const char *shortname)
{
  SDL_WM_SetCaption (title, shortname);
}

#else // not OLD_SDL

extern void
avt_set_title (const char *title, const char *shortname)
{
  // check if encoding was set
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  // check if it's already in correct encoding default="UTF-8"
  if (SDL_strcasecmp ("UTF-8", avt.encoding) == 0
      or SDL_strcasecmp ("UTF8", avt.encoding) == 0
      or SDL_strcasecmp ("CP65001", avt.encoding) == 0)
    SDL_WM_SetCaption (title, shortname);
  else				// convert them to UTF-8
    {
      char my_title[260];
      char my_shortname[84];

      if (title and * title)
	{
	  if (avt_recode_buffer ("UTF-8", avt.encoding,
				 my_title, sizeof (my_title),
				 title, SDL_strlen (title)) == (size_t) (-1))
	    {
	      SDL_memcpy (my_title, title, sizeof (my_title));
	      my_title[sizeof (my_title) - 1] = '\0';
	    }
	}

      if (shortname and * shortname)
	{
	  if (avt_recode_buffer ("UTF-8", avt.encoding,
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

#endif // not OLD_SDL


#define reverse_byte(b) \
  (((b) & 0x80) >> 7 | \
   ((b) & 0x40) >> 5 | \
   ((b) & 0x20) >> 3 | \
   ((b) & 0x10) >> 1 | \
   ((b) & 0x08) << 1 | \
   ((b) & 0x04) << 3 | \
   ((b) & 0x02) << 5 | \
   ((b) & 0x01) << 7)

// width must be a multiple of 8
#define xbm_bytes(img)  ((img##_width / 8) * img##_height)

static void
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

extern void
avt_reset ()
{
  _avt_STATUS = AVT_NORMAL;
  avt.reserve_single_keys = false;
  avt.newline_mode = true;
  avt.auto_margin = true;
  avt.origin_mode = true;	// for backwards compatibility
  avt.scroll_mode = 1;
  avt.textdir_rtl = AVT_LEFT_TO_RIGHT;
  avt.flip_page_delay = AVT_DEFAULT_FLIP_PAGE_DELAY;
  avt.text_delay = 0;
  avt.bitmap_color = AVT_COLOR_BLACK;
  avt.ballooncolor = AVT_BALLOON_COLOR;
  avt.cursor_color = 0xF28919;
  avt.pointer_motion_key = 0;
  avt.pointer_button_key = 0;

  avt_clear_keys ();
  avt_clear_screen ();		// also resets some variables
  avt_normal_text ();
  avt_reset_tab_stops ();
  avt_set_avatar_mode (AVT_SAY);
  avt_set_mouse_visible (true);
  avt_set_avatar_name (NULL);

  if (avt.avatar_image)
    avt_set_avatar_image (NULL);
}

extern int
avt_start (const char *title, const char *shortname, int mode)
{
  SDL_Surface *icon;

  // already initialized?
  if (screen)
    {
      SDL_SetError ("AKFAvatar already initialized");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  avt.mode = mode;

  avt_reset ();

  avt_get_font_dimensions (&fontwidth, &fontheight, &fontunderline);

  // fine-tuning: avoid conflict between underscore and underlining
  if (fontheight >= 18)
    ++fontunderline;

  if (avt_init_SDL ())
    {
      SDL_SetError ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (not title)
    title = "AKFAvatar";

  if (not shortname)
    shortname = title;

  avt_set_title (title, shortname);

  // register icon
  icon = avt_load_image_xpm (akfavatar_xpm);
  SDL_WM_SetIcon (icon, NULL);
  SDL_FreeSurface (icon);

  // Initialize the display
  screenflags = SDL_SWSURFACE | SDL_RESIZABLE;

#ifndef __WIN32__
  if (avt.mode == AVT_AUTOMODE)
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

  if (avt.mode >= 1)
    screenflags |= SDL_FULLSCREEN | SDL_NOFRAME;

  if (avt.mode == AVT_FULLSCREENNOSWITCH)
    {
      screen = SDL_SetVideoMode (0, 0, COLORDEPTH, screenflags);

      // fallback if 0,0 is not supported yet (before SDL-1.2.10)
      if (screen and (screen->w == 0 or screen->h == 0))
	screen =
	  SDL_SetVideoMode (MINIMALWIDTH, MINIMALHEIGHT, COLORDEPTH,
			    screenflags);
    }
  else
    screen =
      SDL_SetVideoMode (MINIMALWIDTH, MINIMALHEIGHT, COLORDEPTH, screenflags);

  if (not screen)
    {
      SDL_SetError ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // assure we really get what we need
  if (SDL_MUSTLOCK (screen) or screen->format->BytesPerPixel != 4)
    {
      SDL_SetError ("error initializing AKFAvatar");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (screen->w < MINIMALWIDTH or screen->h < MINIMALHEIGHT)
    {
      SDL_SetError ("screen too small");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // size of the window (not to be confused with the variable window
  windowmode_size.x = windowmode_size.y = 0;	// unused
  windowmode_size.w = screen->w;
  windowmode_size.h = screen->h;

  avt_avatar_window ();
  avt_set_mouse_pointer ();

  avt.background_color = SDL_MapRGB (screen->format,
				     avt_red (avt.backgroundcolornr),
				     avt_green (avt.backgroundcolornr),
				     avt_blue (avt.backgroundcolornr));

  avt_normal_text ();

  base_button = avt_load_image_xpm (btn_xpm);

  // set actual balloon size to the maximum size
  avt.balloonheight = avt.balloonmaxheight;
  avt.balloonwidth = AVT_LINELENGTH;

  // reserve space for character under text-mode cursor
  avt.cursor_character =
    SDL_CreateRGBSurface (SDL_SWSURFACE, fontwidth, fontheight,
			  screen->format->BitsPerPixel,
			  screen->format->Rmask, screen->format->Gmask,
			  screen->format->Bmask, screen->format->Amask);

  if (not avt.cursor_character)
    {
      SDL_SetError ("out of memory");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  circle = avt_load_image_xbm (AVT_XBM_INFO (circle), avt.ballooncolor);

  // just to be save, because of avt_reset()
  if (avt.pointer)
    SDL_FreeSurface (avt.pointer);

  avt.avatar_mode = AVT_SAY;
  avt.pointer =
    avt_load_image_xbm (AVT_XBM_INFO (balloonpointer), avt.ballooncolor);

  // needed to get the character of the typed key
  SDL_EnableUNICODE (1);

  // key repeat mode
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  // ignore what we don't use
  SDL_EventState (SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState (SDL_KEYUP, SDL_IGNORE);

  // visual flash for the alert
  // when you initialize the audio stuff, you get an audio alert
  avt_alert_func = avt_flash;

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
