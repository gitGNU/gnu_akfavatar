/*
 * Linux framebuffer backend for AKFAvatar
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is only for systems with the kernel Linux.
 * Screen and keyboard are supported, but mouse support is missing.
 * The framebuffer must have 32 bit per pixel.
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
#define _BSD_SOURCE

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
#include <iconv.h>
#include <unistd.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>


static struct avt_settings *avt;
static struct fb_var_screeninfo screeninfo;
static char error_msg[256];
static int screen_fd, tty;
static avt_color *fb;		// frame buffer
static struct termios terminal_settings;
static iconv_t conv;

//-----------------------------------------------------------------------------

// TODO: support other bitdepths
extern void
avt_update_area (int x, int y, int width, int height)
{
  avt_color *pixels;
  int bytes;			// bytes per scanline
  int screen_width;

  pixels = avt->screen->pixels;
  screen_width = avt->screen->width;
  bytes = width * sizeof (avt_color);

  for (int ly = 0; ly < height; ly++)
    memcpy (fb + (y + ly) * screeninfo.xres + x,
	    pixels + (y + ly) * screen_width + x, bytes);
}

// switch to fullscreen or window mode
extern void
avt_switch_mode (int new_mode)
{
  // only 1 mode
}

extern void
avt_toggle_fullscreen (void)
{
  // only 1 mode
}

static avt_char
utf8 (char c)
{
  uint8_t inbuf[4];
  uint8_t outbuf[4];
  size_t inbytes, outbytes;
  avt_char result;

  result = 0;
  memset (inbuf, 0, sizeof (inbuf));
  inbuf[0] = (uint8_t) c;
  inbytes = read (tty, inbuf + 1, sizeof (inbuf) - 2) + 1;

  if (inbytes >= 2)
    {
      char *inptr = (char *) inbuf;
      char *outptr = (char *) outbuf;
      outbytes = sizeof (outbuf);

      if (iconv (conv, &inptr, &inbytes, &outptr, &outbytes) != (size_t) (-1))
	result =
	  outbuf[0] | outbuf[1] << 8 | outbuf[2] << 16 | outbuf[3] << 24;
    }

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
      if (avt->reserve_single_keys)
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
  uint8_t c;
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

// TODO
extern int
avt_wait (size_t milliseconds)
{
  if (milliseconds < 1000)
    {
      avt_delay (milliseconds);
      avt_update ();
    }
  else
    {
      size_t start = avt_ticks ();

      do
	{
	  avt_delay (10);
	  avt_update ();
	}
      while (avt_elapsed (start) < milliseconds);
    }

  return _avt_STATUS;
}

extern void
avt_push_key (avt_char key)
{
  avt_add_key (key);
}

// TODO: use select?
extern void
avt_wait_key (void)
{
  do
    {
      avt_delay (10);
      avt_update ();
    }
  while (_avt_STATUS == AVT_NORMAL and not avt_key_pressed ());
}

extern avt_char
avt_set_pointer_motion_key (avt_char key)
{
  return 0;
}

// key for pointer buttons 1-3
extern avt_char
avt_set_pointer_buttons_key (avt_char key)
{
  return 0;
}

extern void
avt_get_pointer_position (int *x, int *y)
{
  *x = *y = 0;
}

extern void
avt_set_mouse_visible (bool visible)
{
  // no mouse
}

extern int
avt_get_mode (void)
{
  return AVT_FULLSCREEN;
}

extern char *
avt_get_error (void)
{
  return &error_msg[0];
}

extern void
avt_set_error (const char *message)
{
  strncpy (error_msg, message, sizeof (error_msg));
  error_msg[sizeof (error_msg) - 1] = '\0';
}

// TODO: support other bitdepths
static void
avt_clear_screen_fb (avt_color color)
{
  for (int i = screeninfo.xres * screeninfo.yres - 1; i >= 0; i--)
    fb[i] = color;
}

extern void
avt_set_title (const char *title, const char *shortname)
{
}

static void
beep (void)
{
  // this is agent \007 with the license to beep ;-)
  write (tty, "\007", 1);
}

static void
avt_quit_fb (void)
{
  iconv_close (conv);

  munmap (fb, screeninfo.xres * screeninfo.yres * sizeof (avt_color));
  close (screen_fd);

  ioctl (tty, KDSETMODE, KD_TEXT);
  tcsetattr (tty, TCSANOW, &terminal_settings);
  write (tty, "\033[H\033[2J", 7);	// home and clear screen
  close (tty);

  avt->alert = &avt_flash;
}

extern int
avt_start (const char *title, const char *shortname, int window_mode)
{
  // already initialized?
  if (avt)
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

      if (not screen_fd)
	screen_fd = open ("/dev/fb/0", O_RDWR);
    }

  if (not screen_fd)
    {
      avt_set_error ("Error opening framebuffer");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  tty = open ("/dev/tty", O_RDWR | O_NONBLOCK);


  if (not tty)
    {
      close (screen_fd);
      avt_set_error ("Error opening /dev/tty");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // get screeninfo
  ioctl (screen_fd, FBIOGET_VSCREENINFO, &screeninfo);

  // check bits per pixel
  if (screeninfo.bits_per_pixel != CHAR_BIT * sizeof (avt_color))
    {
      avt_set_error ("need 32 bit per pixel");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }


  if (screeninfo.xres < MINIMALWIDTH or screeninfo.yres < MINIMALHEIGHT)
    {
      avt_set_error ("screen too small");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  fb = (avt_color *)
    mmap (NULL,
	  screeninfo.xres * screeninfo.yres *
	  sizeof (avt_color), PROT_WRITE, MAP_SHARED, screen_fd, 0);

  ioctl (tty, KDSETMODE, KD_GRAPHICS);

  // keyboard
  struct termios settings;

  if (tcgetattr (tty, &settings) < 0)
    {
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  terminal_settings = settings;

  cfmakeraw (&settings);

  tcsetattr (tty, TCSANOW, &settings);

  avt = avt_start_common (avt_new_graphic (screeninfo.xres, screeninfo.yres));

  if (not avt)
    {
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // do not change for big endian!
  conv = iconv_open ("UTF-32LE", "UTF-8");

  avt_clear_screen_fb (avt->background_color);
  avt->quit_backend = &avt_quit_fb;
  avt->clear_screen = &avt_clear_screen_fb;
  avt->alert = &beep;		// just remove this line, if you don't like it

  return _avt_STATUS;
}
