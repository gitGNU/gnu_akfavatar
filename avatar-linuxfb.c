/*
 * Linux framebuffer backend for AKFAvatar
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is only for systems with the kernel Linux.
 * Screen and keyboard are supported, but mouse support is missing.
 * The framebuffer must have 32, 24, 16 or 15 bit per pixel.
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

/*
 * This file is system specific, so it may use system specific extensions
 * including GNU extesions
 */

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

// don't make functions deprecated for this file
#define _AVT_USE_DEPRECATED

#include "akfavatar.h"
#include "avtinternals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <iso646.h>
#include <errno.h>
#include <unistd.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>

static struct fb_var_screeninfo var_info;
static struct fb_fix_screeninfo fix_info;
static short bytes_per_pixel;
static int screen_fd, tty;
static uint_least8_t *fb;	// frame buffer
static struct termios terminal_settings;
static char error_message[256];
static bool reserve_single_keys;

//-----------------------------------------------------------------------------

static inline void
normalize_coordinates (avt_graphic * screen, int *x, int *y, int *width,
		       int *height)
{
  if (*x < 0)
    {
      *width -= (-*x);
      *x = 0;
    }

  if (*x + *width > screen->width)
    *width = screen->width - *x;

  if (*y < 0)
    {
      height -= (-*y);
      *y = 0;
    }

  if (*y + *height > screen->height)
    *height = screen->height - *y;
}

static void
update_area_32bit (avt_graphic * screen, int x, int y, int width, int height)
{
  normalize_coordinates (screen, &x, &y, &width, &height);

  if (width <= 0 or height <= 0 or x > screen->width or y > screen->height)
    return;

  avt_color *pixels = screen->pixels + (y * screen->width);
  uint_least8_t *fbp = fb + y * fix_info.line_length + x * bytes_per_pixel;
  int x2 = x + width;

  for (int ly = 0; ly < height; ly++)
    {
      uint_least32_t *p = (uint_least32_t *) fbp;

      // in this mode it might be superfluous to repack pixels,
      // but I want to play save
      for (int lx = x; lx < x2; lx++)
	{
	  register avt_color color = pixels[lx];

	  *p++ = (avt_red (color) << var_info.red.offset)
	    bitor (avt_green (color) << var_info.green.offset)
	    bitor (avt_blue (color) << var_info.blue.offset);
	}

      fbp += fix_info.line_length;
      pixels += screen->width;
    }
}

static void
update_area_24bit (avt_graphic * screen, int x, int y, int width, int height)
{
  normalize_coordinates (screen, &x, &y, &width, &height);

  if (width <= 0 or height <= 0 or x > screen->width or y > screen->height)
    return;

  avt_color *pixels = screen->pixels + (y * screen->width);
  uint_least8_t *fbp = fb + y * fix_info.line_length + x * bytes_per_pixel;
  int x2 = x + width;

  for (int ly = 0; ly < height; ly++)
    {
      uint_least8_t *p = fbp;

      for (int lx = x; lx < x2; lx++)
	{
	  register avt_color color = pixels[lx];

	  if (AVT_BIG_ENDIAN == AVT_BYTE_ORDER)
	    {
	      *p++ = avt_red (color);
	      *p++ = avt_green (color);
	      *p++ = avt_blue (color);
	    }
	  else			// little endian
	    {
	      *p++ = avt_blue (color);
	      *p++ = avt_green (color);
	      *p++ = avt_red (color);
	    }
	}

      fbp += fix_info.line_length;
      pixels += screen->width;
    }
}

static void
update_area_16bit (avt_graphic * screen, int x, int y, int width, int height)
{
  normalize_coordinates (screen, &x, &y, &width, &height);

  if (width <= 0 or height <= 0 or x > screen->width or y > screen->height)
    return;

  avt_color *pixels = screen->pixels + (y * screen->width);
  uint_least8_t *fbp = fb + y * fix_info.line_length + x * bytes_per_pixel;
  int x2 = x + width;

  for (int ly = 0; ly < height; ly++)
    {
      uint_least16_t *p = (uint_least16_t *) fbp;

      for (int lx = x; lx < x2; lx++)
	{
	  register avt_color color = pixels[lx];

	  *p++ = ((avt_red (color) >> (8 - var_info.red.length)))
	    << var_info.red.offset
	    bitor (avt_green (color) >> (8 - var_info.green.length))
	    << var_info.green.offset
	    bitor (avt_blue (color) >> (8 - var_info.blue.length))
	    << var_info.blue.offset;
	}

      fbp += fix_info.line_length;
      pixels += screen->width;
    }
}

// switch to fullscreen or window mode
extern void
avt_switch_mode (int new_mode)
{
  (void) new_mode;

  // only 1 mode
}

extern void
avt_toggle_fullscreen (void)
{
  // only 1 mode
}

extern int
avt_get_mode (void)
{
  return AVT_FULLSCREENNOSWITCH;
}

// this version is stripped down
// see avtutf8.c for a more complete version
static avt_char
utf8_to_unicode (const char *utf8)
{
  avt_char c = BROKEN_WCHAR;
  const unsigned char *u8 = (const unsigned char *) utf8;

  if (*u8 <= 0x7Fu)
    c = *u8;
  else if (*u8 <= 0xDFu)
    c = ((u8[0] bitand compl 0xC0u) << 6) bitor (u8[1] bitand compl 0x80u);
  else if (*u8 <= 0xEFu)
    c = ((u8[0] bitand compl 0xE0u) << (2 * 6))
      bitor ((u8[1] bitand compl 0x80u) << 6)
      bitor (u8[2] bitand compl 0x80u);
  else if (*u8 <= 0xF4u)
    c = ((u8[0] bitand compl 0xF0u) << (3 * 6))
      bitor ((u8[1] bitand compl 0x80u) << (2 * 6))
      bitor ((u8[2] bitand compl 0x80u) << 6)
      bitor (u8[3] bitand compl 0x80u);

  return c;
}

static avt_char
utf8 (char c)
{
  char inbuf[4];
  size_t inbytes;
  avt_char result;

  result = AVT_KEY_NONE;
  memset (inbuf, '\0', sizeof (inbuf));
  inbuf[0] = c;
  inbytes = read (tty, inbuf + 1, sizeof (inbuf) - 1);

  if (inbytes > 0)
    result = utf8_to_unicode (inbuf);

  return result;
}

#define code(x) (strcmp((x), sequence) == 0)

static avt_char
escape (void)
{
  char sequence[30];
  avt_char result;

  result = AVT_KEY_NONE;
  memset (sequence, 0, sizeof (sequence));

  // this is ugly, but it works for me
  if (read (tty, &sequence, sizeof (sequence)) < 0)
    {
      // real Escape key
      if (reserve_single_keys)
	result = AVT_KEY_ESCAPE;
      else
	_avt_STATUS = AVT_QUIT;
    }
  else if (code ("[A"))
    result = AVT_KEY_UP;
  else if (code ("[B"))
    result = AVT_KEY_DOWN;
  else if (code ("[C"))
    result = AVT_KEY_RIGHT;
  else if (code ("[D"))
    result = AVT_KEY_LEFT;
  else if (code ("[1~"))
    result = AVT_KEY_HOME;
  else if (code ("[2~"))
    result = AVT_KEY_INSERT;
  else if (code ("[3~"))
    result = AVT_KEY_DELETE;
  else if (code ("[4~"))
    result = AVT_KEY_END;
  else if (code ("[5~"))
    result = AVT_KEY_PAGEUP;
  else if (code ("[6~"))
    result = AVT_KEY_PAGEDOWN;
  else if (code ("[[A"))
    result = AVT_KEY_F1;
  else if (code ("[[B"))
    result = AVT_KEY_F2;
  else if (code ("[[C"))
    result = AVT_KEY_F3;
  else if (code ("[[D"))
    result = AVT_KEY_F4;
  else if (code ("[[E"))
    result = AVT_KEY_F5;
  else if (code ("[17~"))
    result = AVT_KEY_F6;
  else if (code ("[18~"))
    result = AVT_KEY_F7;
  else if (code ("[19~"))
    result = AVT_KEY_F8;
  else if (code ("[20~"))
    result = AVT_KEY_F9;
  else if (code ("[21~"))
    result = AVT_KEY_F10;
  else if (code ("[23~"))
    result = AVT_KEY_F11;
  else if (code ("[24~"))
    result = AVT_KEY_F12;
  else if (code ("q"))		// Alt + q
    _avt_STATUS = AVT_QUIT;

  return result;
}

extern int
avt_update (void)
{
  uint_least8_t c;
  avt_char key;

  key = AVT_KEY_NONE;

  while (read (tty, &c, 1) > 0 and _avt_STATUS == AVT_NORMAL)
    {
      if (c == '\033')
	key = escape ();
      else if (c == 127)
	key = AVT_KEY_BACKSPACE;
      else if (c > 127)
	key = utf8 (c);
      else
	key = c;

      if (key != AVT_KEY_NONE)
	avt_add_key (key);
    }

  return _avt_STATUS;
}

extern int
avt_wait (size_t milliseconds)
{
  if (milliseconds <= 500)
    {
      if (_avt_STATUS == AVT_NORMAL)
	avt_delay (milliseconds);

      avt_update ();
    }
  else				// longer time
    {
      size_t start, elapsed;
      fd_set input_set;
      struct timeval timeout;

      start = avt_ticks ();
      elapsed = 0;

      while (elapsed < milliseconds and _avt_STATUS == AVT_NORMAL)
	{
	  FD_ZERO (&input_set);
	  FD_SET (tty, &input_set);
	  timeout.tv_sec = (milliseconds - elapsed) / 1000;
	  timeout.tv_usec = ((milliseconds - elapsed) % 1000) * 1000;

	  select (FD_SETSIZE, &input_set, NULL, NULL, &timeout);
	  avt_update ();
	  elapsed = avt_elapsed (start);
	}
    }

  return _avt_STATUS;
}

extern void
avt_push_key (avt_char key)
{
  avt_add_key (key);
}

static void
wait_key_fb (void)
{
  fd_set input_set;

  while (_avt_STATUS == AVT_NORMAL and not avt_key_pressed ())
    {
      FD_ZERO (&input_set);
      FD_SET (tty, &input_set);

      select (FD_SETSIZE, &input_set, NULL, NULL, NULL);
      avt_update ();
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
  (void) key;

  return 0;
}

// key for pointer buttons 1-3
extern avt_char
avt_set_pointer_buttons_key (avt_char key)
{
  (void) key;

  return 0;
}

extern void
avt_get_pointer_position (int *x, int *y)
{
  if (x)
    *x = 0;

  if (y)
    *y = 0;
}

extern void
avt_set_mouse_visible (bool visible)
{
  (void) visible;

  // no mouse
}

extern char *
avt_get_error (void)
{
  return &error_message[0];
}

extern void
avt_set_error (const char *message)
{
  // old messages are always completely overwritten
  // for strncpy I leave the last byte untouched

  if (message and * message)
    strncpy (error_message, message, sizeof (error_message) - 1);
  else				// no message
    memset (error_message, 0, sizeof (error_message));
}

extern void
avt_set_title (const char *title, const char *shortname)
{
  (void) title;
  (void) shortname;

  // nothing to do
}

static void
beep (void)
{
  // this is agent \007 with the license to beep ;-)
  TEMP_FAILURE_RETRY (write (tty, "\007", 1));
}

static void
quit_fb (void)
{
  if (fb and fb != MAP_FAILED)
    {
      munmap (fb, fix_info.smem_len);
      fb = NULL;
    }

  if (screen_fd > 0)
    {
      close (screen_fd);
      screen_fd = -1;
    }

  if (tty > 0)
    {
      ioctl (tty, KDSETMODE, KD_TEXT);
      tcsetattr (tty, TCSANOW, &terminal_settings);
      TEMP_FAILURE_RETRY (write (tty, "\033c", 2));	// Reset
      close (tty);
      tty = -1;
    }

  avt_bell_function (avt_flash);
}

extern int
avt_start (const char *title, const char *shortname, int window_mode)
{
  struct avt_backend *backend;

  (void) title;
  (void) shortname;
  (void) window_mode;

  avt_set_error (NULL);

  // already initialized?
  if (screen_fd > 0)
    {
      avt_set_error ("AKFAvatar already initialized");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (getenv ("DISPLAY"))
    {
      avt_set_error
	("This version is not for X, but for the Linux framebuffer!");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  char *device = getenv ("FRAMEBUFFER");

  if (device)
    screen_fd = open (device, O_RDWR);
  else
    {
      screen_fd = open ("/dev/fb0", O_RDWR);

      if (screen_fd < 0)
	screen_fd = open ("/dev/fb/0", O_RDWR);
    }

  if (screen_fd < 0)
    {
      avt_set_error ("Error opening framebuffer");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  tty = open ("/dev/tty", O_RDWR | O_NONBLOCK);

  if (tty < 0)
    {
      close (screen_fd);
      avt_set_error ("Error opening /dev/tty");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (tcgetattr (tty, &terminal_settings) < 0)
    {
      close (screen_fd);
      close (tty);
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // get info
  ioctl (screen_fd, FBIOGET_FSCREENINFO, &fix_info);
  ioctl (screen_fd, FBIOGET_VSCREENINFO, &var_info);

  // check screen format
  if (fix_info.type != FB_TYPE_PACKED_PIXELS or var_info.bits_per_pixel < 15)
    {
      quit_fb ();
      avt_set_error ("unsupported screen format");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  if (var_info.xres < MINIMALWIDTH or var_info.yres < MINIMALHEIGHT)
    {
      quit_fb ();
      avt_set_error ("screen too small");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  bytes_per_pixel = (var_info.bits_per_pixel + CHAR_BIT - 1) / CHAR_BIT;

  fb = mmap (NULL, fix_info.smem_len, PROT_WRITE, MAP_SHARED, screen_fd, 0);

  if (MAP_FAILED == fb)
    {
      quit_fb ();
      avt_set_error ("mmap failed");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // Select UTF-8 mode for input
  TEMP_FAILURE_RETRY (write (tty, "\033%G", 3));

  // set terminal in graphic mode with raw keyboard
  struct termios settings = terminal_settings;
  cfmakeraw (&settings);
  tcsetattr (tty, TCSANOW, &settings);
  ioctl (tty, KDSETMODE, KD_GRAPHICS);

  backend = avt_start_common (avt_new_graphic (var_info.xres, var_info.yres));

  if (not backend or _avt_STATUS != AVT_NORMAL)
    {
      quit_fb ();
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  switch (var_info.bits_per_pixel)
    {
    case 32:
      backend->update_area = update_area_32bit;
      break;

    case 24:
      backend->update_area = update_area_24bit;
      break;

    case 16:
    case 15:
      backend->update_area = update_area_16bit;
      break;

    default:
      quit_fb ();
      avt_set_error ("unsupported screen format");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  backend->quit = quit_fb;
  backend->wait_key = wait_key_fb;

  memset (fb, 0, fix_info.smem_len);

  avt_bell_function (beep);	// just remove this line, if you don't like it

  return _avt_STATUS;
}
