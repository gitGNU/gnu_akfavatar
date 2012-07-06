/*
 * Lua 5.2 binding for AKFAvatar
 * Copyright (c) 2008,2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99, POSIX.1-2001
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

#pragma GCC diagnostic ignored "-Wunused-parameter"

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

#include "akfavatar.h"
#include "avtaddons.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
  extern int luaopen_akfavatar (lua_State * L);
}
#endif

#include <stdio.h>
#include <stdlib.h>		// for exit() and wchar_t
#include <string.h>		// for strcmp(), strerror()
#include <errno.h>
#include <iso646.h>

#include <unistd.h>		// for chdir(), getcwd(), execlp
#include <dirent.h>		// opendir, readdir, closedir

#include <sys/types.h>
#include <sys/stat.h>

// MinGW and Wine don't know ENOMSG
#ifndef ENOMSG
#  define ENOMSG EINVAL
#endif

// environment variable for avt.datapath
#define AVTDATAPATH  "AVTDATAPATH"

// internal name for the table in the registry
#define AVTMODULE  "AKFAvatar-module"

// internal name for audio data
#define AUDIODATA   "AKFAvatar-Audio"

// separator in paths
#define PATHSEP ';'

static bool initialized = false;

static const char *const modes[] =
  { "auto", "window", "fullscreen", "fullscreen no switch", NULL };

// modes for playing audio
static const char *const playmodes[] = { "load", "play", "loop", NULL };

// "check()" checks the returned status code
#define check(X)  do { if ((X) != AVT_NORMAL) quit (L); } while (0)
#define is_initialized(void)  if (not initialized) auto_initialize(L)

// for internal use only
static int
quit (lua_State * L)
{
  if (avt_get_status () <= AVT_ERROR)
    {
      return luaL_error (L, "%s", avt_get_error ());
    }
  else				// stop requested
    {
      // no actual error, so no error message
      // this is handled by the calling program
      lua_pushnil (L);
      return lua_error (L);
    }
}

// check if it's a boolean
static bool
check_bool (lua_State * L, int index)
{
  luaL_checktype (L, index, LUA_TBOOLEAN);
  return (bool) lua_toboolean (L, index);
}

#define to_bool(L, index)  ((bool) lua_toboolean ((L), (index)))

// imports an XPM table at given index
// result must be freed by caller
static char **
import_xpm (lua_State * L, int index)
{
  char **xpm;
  unsigned int linenr, linecount;
  int idx;

  idx = lua_absindex (L, index);
  xpm = NULL;
  linenr = 0;

  linecount = 512;		// can be extended later
  xpm = (char **) malloc (linecount * sizeof (*xpm));
  if (not xpm)
    return NULL;

  lua_pushnil (L);
  while (lua_next (L, idx))
    {
      xpm[linenr] = (char *) lua_tostring (L, -1);
      linenr++;

      if (linenr >= linecount)	// leave one line reserved
	{
	  linecount += 512;
	  xpm = (char **) realloc (xpm, linecount * sizeof (*xpm));
	  if (not xpm)
	    return NULL;
	}

      lua_pop (L, 1);		// pop value - leave key
    }

  // last line must be NULL
  if (xpm)
    xpm[linenr] = NULL;

  return xpm;
}


static void
auto_initialize (lua_State * L)
{
  // it might be initialized outside of this module
  if (not avt_initialized ())
    check (avt_start (NULL, NULL, AVT_WINDOW));

  initialized = true;
}

static int
get_mode (lua_State * L, int index)
{
  const char *mode_name;
  int mode = AVT_AUTOMODE;

  mode_name = lua_tostring (L, index);
  if (not mode_name)
    return luaL_error (L, "initialize: mode: (string expected, got %s)",
		       lua_typename (L, lua_type (L, -1)));
  else
    {
      int i;

      for (i = 0; modes[i]; i++)
	if (strcmp (mode_name, modes[i]) == 0)
	  {
	    mode = i - 1;
	    break;
	  }

      if (modes[i] == NULL)
	return luaL_error (L, "initialize: mode: " LUA_QS ": %s", mode_name,
			   strerror (ENOMSG));
    }

  return mode;
}


static int
lavt_start (lua_State * L)
{
  int mode;
  const char *title, *shortname;

  mode = AVT_AUTOMODE;

  if (not lua_isnoneornil (L, 1))
    mode = get_mode (L, 1);

  lua_getfield (L, LUA_REGISTRYINDEX, "AKFAvatar-title");
  title = lua_tostring (L, -1);
  lua_getfield (L, LUA_REGISTRYINDEX, "AKFAvatar-shortname");
  shortname = lua_tostring (L, -1);
  lua_pop (L, 2);

  if (not initialized and not avt_initialized ())
    {
      check (avt_start (title, shortname, mode));
    }
  else				// already initialized
    {
      // reset almost everything
      // not the background color - should be set before initialization
      avt_clear_screen ();
      avt_set_balloon_size (0, 0);
      avt_newline_mode (true);
      avt_set_auto_margin (true);
      avt_set_origin_mode (true);
      avt_set_scroll_mode (1);
      avt_reserve_single_keys (false);
      avt_set_balloon_color (0xFFFAF0);	// floral white
      avt_normal_text ();
      avt_set_mouse_visible (true);
      avt_set_title (title, shortname);
      avt_avatar_image_none ();

      if (mode != AVT_AUTOMODE)
	avt_switch_mode (mode);
    }

  initialized = true;
  return 0;
}

// quit the avatar subsystem (closes the window)
static int
lavt_quit (lua_State * L)
{
  avt_quit ();
  initialized = false;
  return 0;
}

// returns version string
static int
lavt_version (lua_State * L)
{
  lua_pushstring (L, avt_version ());
  return 1;
}

// returns copyright string
static int
lavt_copyright (lua_State * L)
{
  lua_pushstring (L, avt_copyright ());
  return 1;
}

// returns license string
static int
lavt_license (lua_State * L)
{
  lua_pushstring (L, avt_license ());
  return 1;
}

// returns whether it is started
static int
lavt_started (lua_State * L)
{
  lua_pushboolean (L, (int) avt_initialized ());
  return 1;
}

/*
 * get color for a given integer value
 * returns name and RGB value as strings
 * or returns nothing on error
 */
static int
lavt_get_color (lua_State * L)
{
  const char *name;
  char RGB[8];
  int colornr;

  name = avt_palette (luaL_checkint (L, 1), &colornr);

  if (name)
    {
      sprintf (RGB, "#%06X", colornr);
      lua_pushstring (L, name);
      lua_pushstring (L, RGB);
      return 2;
    }
  else
    return 0;
}

// internal function used in lavt_colors()
static int
lavt_color_iteration (lua_State * L)
{
  const char *name;
  char RGB[8];
  int colornr;
  int nr = lua_tointeger (L, 2) + 1;

  name = avt_palette (nr, &colornr);

  if (name)
    {
      sprintf (RGB, "#%06X", colornr);
      lua_pushinteger (L, nr);
      lua_pushstring (L, name);
      lua_pushstring (L, RGB);
      return 3;
    }
  else
    return 0;
}

/*
 * iterator function for colors
 * example:
 *   for i,name,rgb in avt.colors() do print(i, rgb, name) end
 */
static int
lavt_colors (lua_State * L)
{
  lua_pushcfunction (L, lavt_color_iteration);
  lua_pushnil (L);		// no state
  lua_pushinteger (L, -1);	// first color - 1
  return 3;
}

// change the used encoding (iconv)
static int
lavt_encoding (lua_State * L)
{
  check (avt_mb_encoding (luaL_checkstring (L, 1)));
  return 0;
}

// get the current encoding (iconv)
static int
lavt_get_encoding (lua_State * L)
{
  lua_pushstring (L, avt_get_mb_encoding ());
  return 1;
}

// recode from one given encoding to another
// avt.recode (string, fromcode [,tocode])
static int
lavt_recode (lua_State * L)
{
  const char *fromcode, *tocode;
  char *string, *result;
  size_t len;
  size_t result_size;

  string = (char *) luaL_checklstring (L, 1, &len);
  fromcode = lua_tostring (L, 2);	// may be nil
  tocode = lua_tostring (L, 3);	// optional

  if (not fromcode and not tocode)
    {
      lua_pushnil (L);
      return 1;
    }

  result_size = avt_recode (tocode, fromcode, &result, string, len);

  if (result_size == (size_t) (-1))
    lua_pushnil (L);
  else
    {
      lua_pushlstring (L, result, result_size);
      avt_free (result);
    }

  return 1;
}


// set avatar image from data (string or table)
static int
lavt_avatar_image (lua_State * L)
{
  is_initialized ();

  if (lua_isnoneornil (L, 1))
    avt_avatar_image_none ();
  else if (lua_istable (L, 1))	// assume XPM table
    {
      char **xpm = import_xpm (L, 1);

      if (not xpm)
	{
	  lua_pushnil (L);
	  lua_pushliteral (L, "cannot load avatar-image");
	  return 2;
	}

      avt_avatar_image_xpm (xpm);
      free (xpm);
    }
  else				// not a table
    {
      const char *avatar;
      size_t len;

      avatar = luaL_checklstring (L, 1, &len);
      if (strcmp ("default", avatar) == 0)
	avt_avatar_image_default ();
      else if (strcmp ("none", avatar) == 0)
	avt_avatar_image_none ();
      else if (avt_avatar_image_data ((void *) avatar, len) != AVT_NORMAL)
	{
	  lua_pushnil (L);
	  lua_pushliteral (L, "cannot load avatar-image");
	  return 2;
	}
    }

  lua_pushboolean (L, true);
  return 1;
}

static int
lavt_avatar_image_file (lua_State * L)
{
  const char *avatar;

  is_initialized ();

  avatar = luaL_checkstring (L, 1);

  if (strcmp ("default", avatar) == 0)
    avt_avatar_image_default ();
  else if (strcmp ("none", avatar) == 0)
    avt_avatar_image_none ();
  else if (avt_avatar_image_file (avatar) != AVT_NORMAL)
    {
      lua_pushnil (L);
      lua_pushliteral (L, "cannot load avatar-image");
      return 2;
    }

  lua_pushboolean (L, true);
  return 1;
}

/*
 * set the name of the avatar
 * must be after setting the image
 */
static int
lavt_set_avatar_name (lua_State * L)
{
  check (avt_set_avatar_name_mb (lua_tostring (L, 1)));
  return 0;
}

/*
 * load image and show it
 * on error it returns nil
 * if it succeeds call avt.wait or avt.waitkey
 */
static int
lavt_show_image_file (lua_State * L)
{
  const char *fn;

  is_initialized ();
  fn = luaL_checkstring (L, 1);
  lua_pushboolean (L, (int) (avt_show_image_file (fn) == AVT_NORMAL));
  return 1;
}

/*
 * get image from data
 * on error it returns nil
 * if it succeeds call avt.wait or avt.waitkey
 */
static int
lavt_show_image (lua_State * L)
{
  is_initialized ();

  if (lua_istable (L, 1))	// assume XPM table
    {
      char **xpm = import_xpm (L, 1);

      if (xpm)
	{
	  lua_pushboolean (L, (int) (avt_show_image_xpm (xpm) == AVT_NORMAL));
	  free (xpm);
	}
      else
	lua_pushboolean (L, (int) false);
    }
  else				// not a table
    {
      char *data;
      size_t len;

      data = (char *) luaL_checklstring (L, 1, &len);
      lua_pushboolean (L,
		       (int) (avt_show_image_data (data, len) == AVT_NORMAL));
    }

  return 1;
}

// change title and/or icontitle
static int
lavt_set_title (lua_State * L)
{
  const char *title, *shortname;

  title = luaL_checkstring (L, 1);
  shortname = luaL_optstring (L, 2, title);

  lua_pushstring (L, title);
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-title");
  lua_pushstring (L, shortname);
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-shortname");

  if (initialized)
    avt_set_title (title, shortname);

  return 0;
}

// right to left writing (true/false)
static int
lavt_right_to_left (lua_State * L)
{
  avt_text_direction ((int) check_bool (L, 1));
  return 0;
}

// set_text_delay
// 0 for no delay, no value for default delay
static int
lavt_set_text_delay (lua_State * L)
{
  avt_set_text_delay (luaL_optint (L, 1, AVT_DEFAULT_TEXT_DELAY));
  return 0;
}

// set flip-page delay
// 0 for no delay, no value for default delay
static int
lavt_set_flip_page_delay (lua_State * L)
{
  avt_set_flip_page_delay (luaL_optint (L, 1, AVT_DEFAULT_TEXT_DELAY));
  return 0;
}

// set balloon size (height, width)
// no values sets the maximum
static int
lavt_set_balloon_size (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_size (lua_tointeger (L, 1), lua_tointeger (L, 2));
  return 0;
}

// set balloon width
// no value sets the maximum
static int
lavt_set_balloon_width (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_width (lua_tointeger (L, 1));
  return 0;
}

// set balloon height
// no value sets the maximum
static int
lavt_set_balloon_height (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_height (lua_tointeger (L, 1));
  return 0;
}

// set balloon color (name)
static int
lavt_set_balloon_color (lua_State * L)
{
  is_initialized ();

  if (lua_type (L, 1) == LUA_TNUMBER)
    avt_set_balloon_color (lua_tointeger (L, 1));
  else
    avt_set_balloon_color (avt_colorname (luaL_checkstring (L, 1)));

  return 0;
}

// set balloon mode
static int
lavt_set_balloon_mode (lua_State * L)
{
  const char *const balloon_modes[] = { "say", "think", "separate", NULL };
  avt_set_balloon_mode (luaL_checkoption (L, 1, "say", balloon_modes));

  return 0;
}

// show cursor (true or false)
static int
lavt_activate_cursor (lua_State * L)
{
  avt_activate_cursor (check_bool (L, 1));
  return 0;
}

// underline mode (true or false)
static int
lavt_underlined (lua_State * L)
{
  avt_underlined (check_bool (L, 1));
  return 0;
}

// return underline mode (true or false)
static int
lavt_get_underlined (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_underlined ());
  return 1;
}

// bold mode (true or false)
static int
lavt_bold (lua_State * L)
{
  avt_bold (check_bool (L, 1));
  return 0;
}

// return bold mode (true or false)
static int
lavt_get_bold (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_bold ());
  return 1;
}

// inverse mode (true or false)
static int
lavt_inverse (lua_State * L)
{
  avt_inverse (check_bool (L, 1));
  return 0;
}

// return inverse mode (true or false)
static int
lavt_get_inverse (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_inverse ());
  return 1;
}

static int
lavt_markup (lua_State * L)
{
  avt_markup (check_bool (L, 1));
  return 0;
}

// reset to normal text mode
static int
lavt_normal_text (lua_State * L)
{
  avt_normal_text ();
  return 0;
}

// clear the whole screen
static int
lavt_clear_screen (lua_State * L)
{
  is_initialized ();
  avt_clear_screen ();
  return 0;
}

/*
 * clear the textfield 
 * if there is no balloon yet, it is drawn
 */
static int
lavt_clear (lua_State * L)
{
  is_initialized ();
  avt_clear ();
  return 0;
}

/*
 * clears from cursor position down the viewport
 * if there is no balloon yet, it is drawn
 */
static int
lavt_clear_down (lua_State * L)
{
  is_initialized ();
  avt_clear_down ();
  return 0;
}

/*
 * clear end of line
 * depending on text direction
 */
static int
lavt_clear_eol (lua_State * L)
{
  is_initialized ();
  avt_clear_eol ();
  return 0;
}

/*
 * clear beginning of line
 * depending on text direction
 */
static int
lavt_clear_bol (lua_State * L)
{
  is_initialized ();
  avt_clear_bol ();
  return 0;
}

// clear line
static int
lavt_clear_line (lua_State * L)
{
  is_initialized ();
  avt_clear_line ();
  return 0;
}

/*
 * clears from cursor position up the viewport
 * if there is no balloon yet, it is drawn
 */
static int
lavt_clear_up (lua_State * L)
{
  is_initialized ();
  avt_clear_up ();
  return 0;
}

// show only the avatar
static int
lavt_show_avatar (lua_State * L)
{
  is_initialized ();
  avt_show_avatar ();
  return 0;
}

// move in
static int
lavt_move_in (lua_State * L)
{
  is_initialized ();
  check (avt_move_in ());
  return 0;
}

// move out
static int
lavt_move_out (lua_State * L)
{
  is_initialized ();
  check (avt_move_out ());
  return 0;
}

// bell or flash if sound is not initialized
static int
lavt_bell (lua_State * L)
{
  is_initialized ();
  avt_bell ();
  return 0;
}

// flash
static int
lavt_flash (lua_State * L)
{
  is_initialized ();
  avt_flash ();
  return 0;
}

// background color of window (name)
static int
lavt_set_background_color (lua_State * L)
{
  if (lua_type (L, 1) == LUA_TNUMBER)
    avt_set_background_color (lua_tointeger (L, 1));
  else
    avt_set_background_color (avt_colorname (luaL_checkstring (L, 1)));

  return 0;
}

// text color (name | #RRGGBB)
static int
lavt_set_text_color (lua_State * L)
{
  if (lua_type (L, 1) == LUA_TNUMBER)
    avt_set_text_color (lua_tointeger (L, 1));
  else
    avt_set_text_color (avt_colorname (luaL_checkstring (L, 1)));

  return 0;
}

// background color of text (name | #RRGGBB)
static int
lavt_set_text_background_color (lua_State * L)
{
  if (lua_type (L, 1) == LUA_TNUMBER)
    avt_set_text_background_color (lua_tointeger (L, 1));
  else
    avt_set_text_background_color (avt_colorname (luaL_checkstring (L, 1)));
  return 0;
}

// set background color of text to ballooncolor
static int
lavt_set_text_background_ballooncolor (lua_State * L)
{
  avt_set_text_background_ballooncolor ();
  return 0;
}

// get x position
static int
lavt_where_x (lua_State * L)
{
  lua_pushinteger (L, avt_where_x ());
  return 1;
}

// get y position
static int
lavt_where_y (lua_State * L)
{
  lua_pushinteger (L, avt_where_y ());
  return 1;
}

// get maximum x position
static int
lavt_get_max_x (lua_State * L)
{
  lua_pushinteger (L, avt_get_max_x ());
  return 1;
}

// get maximum y position
static int
lavt_get_max_y (lua_State * L)
{
  lua_pushinteger (L, avt_get_max_y ());
  return 1;
}

// is the cursor in the home position?
// (also works for right-to-left writing)
static int
lavt_home_position (lua_State * L)
{
  lua_pushboolean (L, (int) avt_home_position ());
  return 1;
}

// move to x position
static int
lavt_move_x (lua_State * L)
{
  is_initialized ();
  avt_move_x (luaL_checkint (L, 1));
  return 0;
}

// move to y position
static int
lavt_move_y (lua_State * L)
{
  is_initialized ();
  avt_move_y (luaL_checkint (L, 1));
  return 0;
}

// move to x and y position
static int
lavt_move_xy (lua_State * L)
{
  is_initialized ();
  avt_move_xy (luaL_checkint (L, 1), luaL_checkint (L, 2));
  return 0;
}

// save cursor position
static int
lavt_save_position (lua_State * L)
{
  is_initialized ();
  avt_save_position ();
  return 0;
}

// restore cursor position
static int
lavt_restore_position (lua_State * L)
{
  is_initialized ();
  avt_restore_position ();
  return 0;
}

// next tab position
static int
lavt_next_tab (lua_State * L)
{
  is_initialized ();
  avt_next_tab ();
  return 0;
}

// last tab position
static int
lavt_last_tab (lua_State * L)
{
  is_initialized ();
  avt_last_tab ();
  return 0;
}

// reset tab stops to every eigth column
static int
lavt_reset_tab_stops (lua_State * L)
{
  is_initialized ();
  avt_reset_tab_stops ();
  return 0;
}

// clear all tab stops
static int
lavt_clear_tab_stops (lua_State * L)
{
  is_initialized ();
  avt_clear_tab_stops ();
  return 0;
}

// set or clear tab in position x
static int
lavt_set_tab (lua_State * L)
{
  is_initialized ();
  avt_set_tab (luaL_checkint (L, 1), check_bool (L, 2));
  return 0;
}

/*
 * delete num lines, starting from line
 * the rest ist scrolled up
 */
static int
lavt_delete_lines (lua_State * L)
{
  is_initialized ();
  avt_delete_lines (luaL_checkint (L, 1), luaL_checkint (L, 2));
  return 0;
}

/*
 * insert num lines, starting at line
 * the rest ist scrolled down
 */
static int
lavt_insert_lines (lua_State * L)
{
  is_initialized ();
  avt_insert_lines (luaL_checkint (L, 1), luaL_checkint (L, 2));
  return 0;
}

// wait until a button is pressed
static int
lavt_wait_button (lua_State * L)
{
  is_initialized ();
  check (avt_wait_button ());
  return 0;
}

// reserve single keys? (true/false)
static int
lavt_reserve_single_keys (lua_State * L)
{
  avt_reserve_single_keys (check_bool (L, 1));
  return 0;
}

static int
lavt_switch_mode (lua_State * L)
{
  is_initialized ();
  avt_switch_mode (luaL_checkoption (L, 1, "auto", modes) - 1);
  return 0;
}

static int
lavt_get_mode (lua_State * L)
{
  lua_pushinteger (L, avt_get_mode ());
  return 1;
}

static int
lavt_toggle_fullscreen (lua_State * L)
{
  is_initialized ();
  avt_toggle_fullscreen ();
  return 0;
}

/*
 * wait a while and then clear the textfield
 * same as \f in avt.say
 */
static int
lavt_flip_page (lua_State * L)
{
  is_initialized ();
  check (avt_flip_page ());

  return 0;
}

static int
lavt_update (lua_State * L)
{
  is_initialized ();
  check (avt_update ());

  return 0;
}

// wait a given amount of seconds (fraction)
#define DEF_WAIT (AVT_DEFAULT_FLIP_PAGE_DELAY / 1000.0)
static int
lavt_wait_sec (lua_State * L)
{
  is_initialized ();
  check (avt_wait ((size_t) (luaL_optnumber (L, 1, DEF_WAIT) * 1000.0)));

  return 0;
}

static int
lavt_ticks (lua_State * L)
{
  // a Lua number may be larger than a Lua integer
  lua_pushnumber (L, avt_ticks ());
  return 1;
}

// show final credits from a string
// 1=text, 2=centered (true/false/nothing)
static int
lavt_credits (lua_State * L)
{
  is_initialized ();
  check (avt_credits_mb (luaL_checkstring (L, 1), to_bool (L, 2)));

  return 0;
}

static int
lavt_newline (lua_State * L)
{
  is_initialized ();
  check (avt_new_line ());

  return 0;
}

static int
lavt_ask (lua_State * L)
{
  char buf[4 * AVT_LINELENGTH + 1];
  const char *question;
  size_t len;

  is_initialized ();
  question = lua_tolstring (L, 1, &len);
  if (question)
    check (avt_say_mb_len (question, len));
  check (avt_ask_mb (buf, sizeof (buf)));

  lua_pushstring (L, buf);
  return 1;
}

// returns a pressed key (unicode-value)
static int
lavt_get_key (lua_State * L)
{
  avt_char ch;

  is_initialized ();
  check (avt_key (&ch));

  lua_pushinteger (L, (lua_Integer) ch);
  return 1;
}

/*
 * navigation bar
 *
 * buttons is a string with the following characters
 * 'l': left
 * 'r': right (play)
 * 'd': down
 * 'u': up
 * 'x': cancel
 * 'f': (fast)forward
 * 'b': (fast)backward
 * 'p': pause
 * 's': stop
 * 'e': eject
 * '*': circle (record)
 * '+': plus (add)
 * '-': minus (remove)
 * '?': help
 * ' ': spacer (no button)
 *
 * Pressing a key with one of those characters selects it.
 * For the directions you can also use the arrow keys,
 * The [Pause] key returns 'p'.
 * The [Help] key or [F1] return '?'.
 *
 * it returns the approriete character or a number
 */
static int
lavt_navigate (lua_State * L)
{
  int r;

  is_initialized ();
  r = avt_navigate (luaL_checkstring (L, 1));

  check (avt_get_status ());

  if (r < 32)
    lua_pushinteger (L, r);
  else
    lua_pushfstring (L, "%c", r);

  return 1;
}

// make a positive/negative decision
static int
lavt_decide (lua_State * L)
{
  is_initialized ();
  lua_pushboolean (L, (int) avt_decide ());
  check (avt_get_status ());

  return 1;
}

static int
lavt_choice (lua_State * L)
{
  int result;
  const char *c;

  is_initialized ();

  // get string in position 3
  c = lua_tostring (L, 3);

  check (avt_choice (&result,
		     luaL_checkint (L, 1), luaL_checkint (L, 2),
		     (c) ? c[0] : 0, to_bool (L, 4), to_bool (L, 5)));

  lua_pushinteger (L, result);
  return 1;
}

// works like "io.write", but writes in the balloon
// expects one or more strings or numbers
static int
lavt_say (lua_State * L)
{
  int n, i;
  const char *s;
  size_t len;

  is_initialized ();
  n = lua_gettop (L);

  for (i = 1; i <= n; i++)
    {
      s = luaL_checklstring (L, i, &len);
      if (s)
	check (avt_say_mb_len (s, len));
    }

  return 0;
}

// works like "print", but prints in the balloon
// expects one or more strings or numbers
static int
lavt_print (lua_State * L)
{
  int n, i;
  const char *s;
  size_t len;

  is_initialized ();
  n = lua_gettop (L);
  lua_getglobal (L, "tostring");

  for (i = 1; i <= n; i++)
    {
      lua_pushvalue (L, -1);	// function "tostring"
      lua_pushvalue (L, i);	// value
      lua_call (L, 1, 1);
      s = lua_tolstring (L, -1, &len);
      lua_pop (L, 1);		// pop result of "tostring"
      if (s)
	{
	  if (i > 1)
	    avt_next_tab ();
	  check (avt_say_mb_len (s, len));
	}
    }

  lua_pop (L, 1);		// pop function
  avt_new_line ();

  return 0;
}

static int
lavt_tell (lua_State * L)
{
  const char *s;
  size_t len;

  is_initialized ();
  lua_concat (L, lua_gettop (L));	// make it one single string

  s = luaL_checklstring (L, 1, &len);
  if (s)
    check (avt_tell_mb_len (s, len));

  return 0;
}

static int
lavt_say_unicode (lua_State * L)
{
  int n, i;
  const char *s;
  size_t len;

  is_initialized ();

  n = lua_gettop (L);

  for (i = 1; i <= n; i++)
    {
      /*
       * lua_isnumber is not suited, because it implicitly tries to
       * convert strings to numbers
       */
      if (lua_type (L, i) == LUA_TNUMBER)
	check (avt_put_char ((avt_char) lua_tointeger (L, i)));
      else
	{
	  s = luaL_checklstring (L, i, &len);
	  if (s)
	    check (avt_say_mb_len (s, len));
	}
    }

  return 0;
}

static int
lavt_printable (lua_State * L)
{
  lua_pushboolean (L,
		   (int) avt_is_printable ((avt_char)
					   luaL_checkinteger (L, 1)));
  return 1;
}

static int
lavt_pager (lua_State * L)
{
  const char *s;
  size_t len;
  int startline;

  is_initialized ();
  s = luaL_checklstring (L, 1, &len);
  startline = lua_tointeger (L, 2);

  if (s)
    check (avt_pager_mb (s, len, startline));

  return 0;
}

static int
lavt_start_audio (lua_State * L)
{
  if (avt_start_audio ())
    {
      lua_pushnil (L);
      lua_pushstring (L, avt_get_error ());
      return 2;
    }
  else
    {
      lua_pushboolean (L, true);
      return 1;
    }
}

// the current sound may be garbage collected again
#define audio_not_playing(L) \
  do { \
    lua_pushnil(L); \
    lua_setfield(L, LUA_REGISTRYINDEX, "AKFAvatar-current-sound"); \
    } while(0)

// shut down the audio-system
static int
lavt_quit_audio (lua_State * L)
{
  avt_quit_audio ();
  audio_not_playing (L);
  return 0;
}

static void
make_audio_element (lua_State * L, avt_audio * data)
{
  avt_audio **audio;

  if (data)
    {
      audio = (avt_audio **) lua_newuserdata (L, sizeof (avt_audio *));
      *audio = data;
      luaL_getmetatable (L, AUDIODATA);
      lua_setmetatable (L, -2);
    }
  else
    lua_getfield (L, LUA_REGISTRYINDEX, "AKFAvatar-silence");
}

/*
 * get a silent audio structure
 *
 * this function is dedicated to the song
 * "Sound of Silence" by Simon & Garfunkel
 */
static int
lavt_silent (lua_State * L)
{
  lua_getfield (L, LUA_REGISTRYINDEX, "AKFAvatar-silence");
  return 1;
}

/*
 * loads an audio file
 * supported: AU and Wave
 */
static int
lavt_load_audio_file (lua_State * L)
{
  const char *filename;
  size_t len;
  avt_audio *audio_data;
  int playmode;

  filename = "";
  audio_data = NULL;
  len = 0;

  if (not lua_isnoneornil (L, 1))
    filename = luaL_checklstring (L, 1, &len);

  playmode = luaL_checkoption (L, 2, "load", playmodes);

  // if filename is not none or nil or ""
  if (len > 0)
    {
      if (not avt_audio_playing (NULL))
	audio_not_playing (L);

      // full garbage collection
      lua_gc (L, LUA_GCCOLLECT, 0);

      audio_data = avt_load_audio_file (filename, playmode);

      if (not audio_data)
	{
	  lua_pushnil (L);
	  lua_pushstring (L, avt_get_error ());
	  return 2;
	}
    }

  make_audio_element (L, audio_data);
  return 1;
}

// loads audio from a stream
static int
lavt_load_audio_stream (lua_State * L)
{
  luaL_Stream *stream;
  avt_audio *audio_data;
  lua_Unsigned maxsize;
  int playmode;

  stream = (luaL_Stream *) luaL_checkudata (L, 1, LUA_FILEHANDLE);
  maxsize = lua_tounsigned (L, 2);	// nothing or 0 allowed
  playmode = luaL_checkoption (L, 3, "load", playmodes);

  if (stream->closef == NULL)
    return luaL_error (L, "attempt to use a closed file");

  if (not avt_audio_playing (NULL))
    audio_not_playing (L);

  // full garbage collection
  lua_gc (L, LUA_GCCOLLECT, 0);

  audio_data =
    avt_load_audio_part ((avt_stream *) stream->f, maxsize, playmode);

  if (not audio_data)
    {
      lua_pushnil (L);
      lua_pushstring (L, avt_get_error ());
      return 2;
    }

  make_audio_element (L, audio_data);
  return 1;
}

static int
lavt_load_audio (lua_State * L)
{
  char *data;
  size_t len;
  avt_audio *audio_data;
  int playmode;

  data = "";
  audio_data = NULL;
  len = 0;

  if (not lua_isnoneornil (L, 1))
    data = (char *) luaL_checklstring (L, 1, &len);

  playmode = luaL_checkoption (L, 2, "load", playmodes);

  // if string is not none or nil or ""
  if (len > 0)
    {
      if (not avt_audio_playing (NULL))
	audio_not_playing (L);

      // full garbage collection
      lua_gc (L, LUA_GCCOLLECT, 0);

      audio_data = avt_load_audio_data (data, len, playmode);

      if (not audio_data)
	{
	  lua_pushnil (L);
	  lua_pushstring (L, avt_get_error ());
	  return 2;
	}
    }

  make_audio_element (L, audio_data);
  return 1;
}

/*
 * wait until the sound ends
 * this stops a loop, but still plays to the end of the sound
 */
static int
lavt_wait_audio_end (lua_State * L)
{
  check (avt_wait_audio_end ());
  audio_not_playing (L);
  return 0;
}

static int
lavt_stop_audio (lua_State * L)
{
  avt_stop_audio ();
  audio_not_playing (L);
  return 0;
}

static int
lavt_pause_audio (lua_State * L)
{
  avt_pause_audio (check_bool (L, 1));
  return 0;
}

// plays audio data
static int
laudio_play (lua_State * L)
{
  avt_audio **audio;

  audio = (avt_audio **) luaL_checkudata (L, 1, AUDIODATA);

  if (audio and * audio)
    avt_play_audio (*audio, AVT_PLAY);	// no check!

  // store reference to audio, so it isn't garbage collected while playing
  lua_pushvalue (L, 1);
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-current-sound");

  return 0;
}

// plays audio data in a loop
static int
laudio_loop (lua_State * L)
{
  avt_audio **audio;

  audio = (avt_audio **) luaL_checkudata (L, 1, AUDIODATA);

  if (audio and * audio)
    avt_play_audio (*audio, AVT_LOOP);	// no check!

  // store reference to audio, so it isn't garbage collected while playing
  lua_pushvalue (L, 1);
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-current-sound");

  return 0;
}

// frees audio data
static int
laudio_free (lua_State * L)
{
  avt_audio **audio;

  audio = (avt_audio **) luaL_checkudata (L, 1, AUDIODATA);
  if (audio and * audio)
    {
      avt_free_audio (*audio);
      *audio = NULL;
    }

  return 0;
}

// checks if audio is still playing
static int
laudio_playing (lua_State * L)
{
  avt_audio **audio;

  if (lua_isnoneornil (L, 1))
    lua_pushboolean (L, avt_audio_playing (NULL));
  else
    {
      audio = (avt_audio **) luaL_checkudata (L, 1, AUDIODATA);
      lua_pushboolean (L, (audio and * audio and avt_audio_playing (*audio)));
    }

  return 1;
}

static int
laudio_tostring (lua_State * L)
{
  avt_audio **audio;

  audio = (avt_audio **) luaL_checkudata (L, 1, AUDIODATA);

  if (audio and * audio)
    lua_pushfstring (L, AUDIODATA " (%p)", audio);
  else
    lua_pushfstring (L, AUDIODATA ", silent (%p)", audio);

  return 1;
}

/*
 * set a viewport (sub-area of the textarea)
 * 1=x, 2=y, 3=width, 4=height
 * upper left corner is 1, 1
 */
static int
lavt_viewport (lua_State * L)
{
  avt_viewport (luaL_checkint (L, 1),
		luaL_checkint (L, 2),
		luaL_checkint (L, 3), luaL_checkint (L, 4));
  return 0;
}

static int
lavt_set_scroll_mode (lua_State * L)
{
  avt_set_scroll_mode (luaL_checkint (L, 1));
  return 0;
}

static int
lavt_get_scroll_mode (lua_State * L)
{
  lua_pushinteger (L, avt_get_scroll_mode ());
  return 1;
}

static int
lavt_newline_mode (lua_State * L)
{
  avt_newline_mode (check_bool (L, 1));
  return 0;
}

static int
lavt_set_auto_margin (lua_State * L)
{
  avt_set_auto_margin (check_bool (L, 1));
  return 0;
}

static int
lavt_get_auto_margin (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_auto_margin ());
  return 1;
}

static int
lavt_set_origin_mode (lua_State * L)
{
  avt_set_origin_mode (check_bool (L, 1));
  return 0;
}

static int
lavt_get_origin_mode (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_origin_mode ());
  return 1;
}

static int
lavt_set_mouse_visible (lua_State * L)
{
  avt_set_mouse_visible (check_bool (L, 1));
  return 0;
}

static int
lavt_lock_updates (lua_State * L)
{
  avt_lock_updates (check_bool (L, 1));
  return 0;
}

static int
lavt_insert_spaces (lua_State * L)
{
  is_initialized ();
  avt_insert_spaces (luaL_checkint (L, 1));
  return 0;
}

static int
lavt_delete_characters (lua_State * L)
{
  is_initialized ();
  avt_delete_characters (luaL_checkint (L, 1));
  return 0;
}

static int
lavt_erase_characters (lua_State * L)
{
  is_initialized ();
  avt_erase_characters (luaL_checkint (L, 1));
  return 0;
}

static int
lavt_backspace (lua_State * L)
{
  is_initialized ();
  avt_backspace ();
  return 0;
}

static int
lavt_subprogram (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TFUNCTION);

  if (lua_pcall (L, lua_gettop (L) - 1, LUA_MULTRET, 0) != 0)
    {
      if (lua_isnil (L, -1))	// just a quit-request?
	{
	  avt_set_status (AVT_NORMAL);
	  return 0;
	}
      else
	return lua_error (L);
    }

  return lua_gettop (L);
}

// ---------------------------------------------------------
// avtaddons.h

// avt.file_selection (filter)

// we need to temporarily store a pointer to the Lua_state :-(
static lua_State *tmp_lua_state;

// call the function at index 1 with the filename
static bool
file_filter (const char *filename)
{
  bool result;

  lua_pushvalue (tmp_lua_state, 1);	// push func again, to keep it
  lua_pushstring (tmp_lua_state, filename);	// parameter
  lua_call (tmp_lua_state, 1, 1);
  result = to_bool (tmp_lua_state, -1);
  lua_pop (tmp_lua_state, 1);	// pop result, leave func on stack

  return result;
}

static int
lavt_file_selection (lua_State * L)
{
  char filename[256];
  avta_filter filter;

  is_initialized ();

  if (lua_isnoneornil (L, 1))
    filter = NULL;
  else
    {
      luaL_checktype (L, 1, LUA_TFUNCTION);
      tmp_lua_state = L;
      filter = &file_filter;
    }

  if (avta_file_selection (filename, sizeof (filename), filter) > -1)
    lua_pushstring (L, filename);
  else
    lua_pushnil (L);

  tmp_lua_state = NULL;
  return 1;
}

static int
lavt_color_selection (lua_State * L)
{
  lua_pushstring (L, avta_color_selection ());
  return 1;
}

// ---------------------------------------------------------
// system calls

static int
lavt_chdir (lua_State * L)
{
  const char *path;

  // nil or empty string is acceptable
  path = lua_tostring (L, 1);

  if (path and * path)
    chdir (path);		// chdir() conforms to POSIX.1-2001

  return 0;
}

static int
lavt_getcwd (lua_State * L)
{
  size_t size;

  /*
   * this might seem overcomplicated, but there are systems with
   * no limit on the length of a path name (eg. GNU/HURD)
   */

  size = 100;
  while (true)
    {
      char *buffer;
      int error_nr;

      buffer = (char *) malloc (size);
      if (buffer == NULL)
	return luaL_error (L, "%s", strerror (errno));

      // getcwd() conforms to POSIX.1-2001
      if (getcwd (buffer, size) == buffer)
	{
	  lua_pushstring (L, buffer);
	  free (buffer);
	  return 1;
	}

      error_nr = errno;

      free (buffer);

      if (error_nr != ERANGE)
	{
	  // unsolvable error
	  lua_pushnil (L);
	  lua_pushstring (L, strerror (error_nr));
	  return 2;
	}

      // try again with a larger buffer
      size *= 2;
    }
}

static int
lavt_launch (lua_State * L)
{
  char *argv[256];
  int i, n;

  // program must be given
  argv[0] = (char *) luaL_checkstring (L, 1);

  n = lua_gettop (L);		// number of options

  // number of arguments is already limited in Lua, but I want to be save
  if (n > 255)
    return luaL_error (L, "launch: %s", strerror (E2BIG));

  // collect arguments
  for (i = 1; i < n; i++)
    argv[i] = (char *) lua_tostring (L, i + 1);
  argv[n] = NULL;

  avt_quit ();			// close window / graphic mode
  initialized = false;

  // don't close lua state - it is still needed

  // conforming to POSIX.1-2001
  execvp (argv[0], argv);

  // execvp only returns in case of an error
  return luaL_error (L, "launch: %s: %s", argv[0], strerror (errno));
}

// ---------------------------------------------------------
// high level functions

// three arrows up
#define BACK L"\x2191 \x2191 \x2191"

// three arrows down
#define CONTINUE L"\x2193 \x2193 \x2193"

#define MARK(S) \
         do { \
           avt_set_text_background_color (0xDDDDDD); \
           avt_clear_line (); \
           avt_move_x (mid_x-(sizeof(S)/sizeof(wchar_t)-1)/2); \
           avt_say(S); \
           avt_normal_text(); \
         } while(0)


static int
lavt_menu (lua_State * L)
{
  long int item_nr;
  const char *item_desc;
  int i;
  int start_line, menu_start;
  int max_idx, items, page_nr, items_per_page;
  int choice;
  int mid_x;
  size_t len;
  bool old_auto_margin, old_newline_mode;
  bool small;

  is_initialized ();
  luaL_checktype (L, 1, LUA_TTABLE);

  avt_set_text_delay (0);
  avt_normal_text ();
  avt_lock_updates (true);

  start_line = avt_where_y ();
  if (start_line < 1)		// no balloon yet?
    start_line = 1;

  mid_x = avt_get_max_x () / 2;	// used by MARK()
  max_idx = avt_get_max_y () - start_line + 1;

  // check, if it's a short menu
  lua_rawgeti (L, 1, max_idx + 1);
  small = (bool) lua_isnil (L, -1);
  lua_pop (L, 1);

  item_desc = NULL;
  items = 0;
  item_nr = 0;
  page_nr = 0;
  items_per_page = small ? max_idx : max_idx - 2;

  old_auto_margin = avt_get_auto_margin ();
  avt_set_auto_margin (false);
  old_newline_mode = avt_get_newline_mode ();
  avt_newline_mode (true);

  while (not item_nr)
    {
      avt_move_xy (1, start_line);
      avt_clear_down ();

      items = 0;

      if (not small)
	{
	  if (page_nr > 0)
	    MARK (BACK);
	  else
	    MARK (L"");

	  items = 1;
	}

      for (i = 1; i <= items_per_page; i++)
	{
	  lua_rawgeti (L, 1, i + (page_nr * items_per_page));
	  if (lua_istable (L, -1))
	    {
	      lua_rawgeti (L, -1, 1);
	      item_desc = lua_tolstring (L, -1, &len);
	      lua_pop (L, 1);	// pop value
	    }
	  else			// only string given
	    item_desc = lua_tolstring (L, -1, &len);

	  if (item_desc)
	    {
	      if (not small or i > 1)
		avt_new_line ();
	      avt_say_mb_len (item_desc, len);
	      items++;
	    }

	  lua_pop (L, 1);	// pop item from stack
	  // from now on item_desc should not be dereferenced

	  if (not item_desc)
	    break;
	}

      // are there more items?
      if (item_desc)
	{
	  lua_rawgeti (L, 1, (page_nr + 1) * items_per_page + 1);
	  if (not lua_isnil (L, -1))
	    {
	      avt_new_line ();
	      MARK (CONTINUE);
	      items = max_idx;
	    }
	  lua_pop (L, 1);	// pop item description from stack
	}

      menu_start = start_line;
      if (not small and page_nr == 0)
	{
	  menu_start++;
	  items--;
	}

      avt_lock_updates (false);
      check (avt_choice (&choice, menu_start, items, 0,
			 (page_nr > 0), (not small and item_desc != NULL)));
      avt_lock_updates (true);

      if (page_nr == 0)
	choice++;

      if (not small and choice == 1 and page_nr > 0)
	page_nr--;		// page back
      else if (not small and choice == max_idx)
	page_nr += (item_desc == NULL) ? 0 : 1;	// page forward
      else
	item_nr = choice - 1 + (page_nr * items_per_page);
    }

  avt_set_auto_margin (old_auto_margin);
  avt_newline_mode (old_newline_mode);
  avt_clear ();
  avt_lock_updates (false);

  // check item_nr
  lua_rawgeti (L, 1, item_nr);
  if (lua_istable (L, -1))
    {
      int item, nresults, table;

      table = lua_gettop (L);
      item = 2;			// skip title
      nresults = 0;

      while (1)
	{
	  lua_rawgeti (L, table, item++);

	  if (not lua_isnil (L, -1))
	    nresults++;
	  else
	    {
	      lua_pop (L, 1);	// pop nil
	      break;
	    }
	}

      return nresults;
    }
  else				// not a table
    {
      lua_pushinteger (L, item_nr);
      return 1;
    }
}

/*
 * optionally accepts directory name (defaults on current directory)
 * returns table with directory entries (except "." and "..")
 * and number of entries
 * returns nil on error
 */
static int
lavt_directory_entries (lua_State * L)
{
  DIR *dir;
  struct dirent *d;
  int nr;

  // conforming to POSIX.1-2001

  if ((dir = opendir (luaL_optstring (L, 1, "."))) == NULL)
    {
      int err = errno;
      lua_pushnil (L);
      lua_pushstring (L, strerror (err));
      return 2;
    }

  lua_newtable (L);
  nr = 0;

  while ((d = readdir (dir)) != NULL)
    {
      if (strcmp (".", d->d_name) != 0 and strcmp ("..", d->d_name) != 0)
	{
	  lua_pushstring (L, d->d_name);
	  lua_rawseti (L, -2, ++nr);
	}
    }

  closedir (dir);

  lua_pushinteger (L, nr);	// number of entries

  return 2;
}

// S_ISSOCK not on all systems, although in POSIX.1-2001
#ifndef S_ISSOCK
#define S_ISSOCK(x) 0
#endif

static int
lavt_entry_type (lua_State * L)
{
  struct stat st;

  // conforming to POSIX.1-2001

  if (stat (luaL_checkstring (L, 1), &st) == -1)
    {
      int err = errno;
      lua_pushnil (L);
      lua_pushstring (L, strerror (err));
      return 2;
    }

  if (S_ISREG (st.st_mode))
    lua_pushliteral (L, "file");
  else if (S_ISDIR (st.st_mode))
    lua_pushliteral (L, "directory");
  else if (S_ISCHR (st.st_mode))
    lua_pushliteral (L, "character device");
  else if (S_ISBLK (st.st_mode))
    lua_pushliteral (L, "block device");
  else if (S_ISFIFO (st.st_mode))
    lua_pushliteral (L, "fifo");
  else if (S_ISSOCK (st.st_mode))
    lua_pushliteral (L, "socket");
  else
    lua_pushliteral (L, "unknown");

  lua_pushinteger (L, st.st_size);

  return 2;
}

static int
lavt_optional (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TSTRING);
  lua_getglobal (L, "require");
  lua_pushvalue (L, 1);
  if (lua_pcall (L, 1, 1, 0) != 0)
    lua_pushnil (L);		// return nil on error

  return 1;
}

// get a string variable from this module
static const char *
get_string_var (lua_State * L, const char *name)
{
  const char *s;

  lua_getfield (L, LUA_REGISTRYINDEX, AVTMODULE);
  lua_getfield (L, -1, name);
  s = lua_tostring (L, -1);
  lua_pop (L, 2);

  // s points to the string in L
  return s;
}

// searches given file in in given path
static int
lavt_search (lua_State * L)
{
  const char *filename, *path;

  filename = luaL_checkstring (L, 1);
  path = lua_tostring (L, 2);

  if (path == NULL and (path = get_string_var (L, "datapath")) == NULL)
    path = ".";

  while (*path)
    {
      char fullname[4096];
      const char *name = filename;
      size_t pos = 0;

      // start with next directory from path
      while (*path and * path != PATHSEP and pos < sizeof (fullname) - 1)
	fullname[pos++] = *path++;

      // skip path seperator(s)
      while (*path == PATHSEP)
	path++;

      // eventually add directory separator
      if (fullname[pos - 1] != LUA_DIRSEP[0] and pos < sizeof (fullname) - 1)
	fullname[pos++] = LUA_DIRSEP[0];

      // add name
      while (*name and pos < sizeof (fullname) - 1)
	fullname[pos++] = *name++;

      // terminate fullname
      fullname[pos] = '\0';

      // check for file existence (POSIX.1-2001)
      if (access (fullname, F_OK) == 0)
	{
	  lua_pushstring (L, fullname);
	  return 1;
	}
    }

  // not found
  lua_pushnil (L);
  lua_pushfstring (L, LUA_QS " not found", filename);
  return 2;
}

static int
lavt_translate (lua_State * L)
{
  const char *text, *language, *translation;

  text = luaL_checkstring (L, 1);

  lua_getfield (L, LUA_REGISTRYINDEX, AVTMODULE);
  lua_getfield (L, -1, "language");
  language = lua_tostring (L, -1);

  if (not language)
    goto fail;

  lua_getfield (L, -2, "translations");
  if (not lua_istable (L, -1))
    goto fail;

  lua_getfield (L, -1, text);
  if (not lua_istable (L, -1))
    goto fail;

  lua_getfield (L, -1, language);
  translation = lua_tostring (L, -1);

  if (translation)		// success
    {
      lua_pushstring (L, translation);
      return 1;
    }

fail:
  // on failure return original text
  lua_pushstring (L, text);
  return 1;
}

// ---------------------------------------------------------
// register library functions

static const luaL_Reg akfavtlib[] = {
  {"start", lavt_start},
  {"quit", lavt_quit},
  {"avatar_image", lavt_avatar_image},
  {"avatar_image_file", lavt_avatar_image_file},
  {"set_avatar_name", lavt_set_avatar_name},
  {"say", lavt_say},
  {"write", lavt_say},		// alias
  {"print", lavt_print},
  {"tell", lavt_tell},
  {"say_unicode", lavt_say_unicode},
  {"printable", lavt_printable},
  {"ask", lavt_ask},
  {"navigate", lavt_navigate},
  {"decide", lavt_decide},
  {"pager", lavt_pager},
  {"choice", lavt_choice},
  {"get_key", lavt_get_key},
  {"newline", lavt_newline},
  {"version", lavt_version},
  {"copyright", lavt_copyright},
  {"license", lavt_license},
  {"started", lavt_started},
  {"get_color", lavt_get_color},
  {"colors", lavt_colors},
  {"encoding", lavt_encoding},
  {"get_encoding", lavt_get_encoding},
  {"recode", lavt_recode},
  {"title", lavt_set_title},
  {"set_title", lavt_set_title},
  {"set_text_delay", lavt_set_text_delay},
  {"set_flip_page_delay", lavt_set_flip_page_delay},
  {"right_to_left", lavt_right_to_left},
  {"flip_page", lavt_flip_page},
  {"update", lavt_update},
  {"wait", lavt_wait_sec},
  {"ticks", lavt_ticks},
  {"set_balloon_size", lavt_set_balloon_size},
  {"set_balloon_width", lavt_set_balloon_width},
  {"set_balloon_height", lavt_set_balloon_height},
  {"set_balloon_color", lavt_set_balloon_color},
  {"set_balloon_mode", lavt_set_balloon_mode},
  {"set_background_color", lavt_set_background_color},
  {"set_text_color", lavt_set_text_color},
  {"set_text_background_color", lavt_set_text_background_color},
  {"set_text_background_ballooncolor", lavt_set_text_background_ballooncolor},
  {"activate_cursor", lavt_activate_cursor},
  {"underlined", lavt_underlined},
  {"get_underlined", lavt_get_underlined},
  {"bold", lavt_bold},
  {"get_bold", lavt_get_bold},
  {"inverse", lavt_inverse},
  {"get_inverse", lavt_get_inverse},
  {"normal_text", lavt_normal_text},
  {"markup", lavt_markup},
  {"clear_screen", lavt_clear_screen},
  {"clear", lavt_clear},
  {"clear_up", lavt_clear_up},
  {"clear_down", lavt_clear_down},
  {"clear_eol", lavt_clear_eol},
  {"clear_bol", lavt_clear_bol},
  {"clear_line", lavt_clear_line},
  {"show_avatar", lavt_show_avatar},
  {"show_image", lavt_show_image},
  {"show_image_file", lavt_show_image_file},
  {"credits", lavt_credits},
  {"move_in", lavt_move_in},
  {"move_out", lavt_move_out},
  {"wait_button", lavt_wait_button},
  {"bell", lavt_bell},
  {"flash", lavt_flash},
  {"reserve_single_keys", lavt_reserve_single_keys},
  {"switch_mode", lavt_switch_mode},
  {"get_mode", lavt_get_mode},
  {"toggle_fullscreen", lavt_toggle_fullscreen},
  {"where_x", lavt_where_x},
  {"where_y", lavt_where_y},
  {"home_position", lavt_home_position},
  {"get_max_x", lavt_get_max_x},
  {"get_max_y", lavt_get_max_y},
  {"move_x", lavt_move_x},
  {"move_y", lavt_move_y},
  {"move_xy", lavt_move_xy},
  {"save_position", lavt_save_position},
  {"restore_position", lavt_restore_position},
  {"next_tab", lavt_next_tab},
  {"last_tab", lavt_last_tab},
  {"reset_tab_stops", lavt_reset_tab_stops},
  {"clear_tab_stops", lavt_clear_tab_stops},
  {"set_tab", lavt_set_tab},
  {"delete_lines", lavt_delete_lines},
  {"insert_lines", lavt_insert_lines},
  {"start_audio", lavt_start_audio},
  {"quit_audio", lavt_quit_audio},
  {"load_audio_file", lavt_load_audio_file},
  {"load_base_audio_file", lavt_load_audio_file},
  {"load_audio_stream", lavt_load_audio_stream},
  {"load_base_audio_stream", lavt_load_audio_stream},
  {"load_audio", lavt_load_audio},
  {"load_base_audio", lavt_load_audio},
  {"silent", lavt_silent},
  {"audio_playing", laudio_playing},
  {"wait_audio_end", lavt_wait_audio_end},
  {"stop_audio", lavt_stop_audio},
  {"pause_audio", lavt_pause_audio},
  {"viewport", lavt_viewport},
  {"set_scroll_mode", lavt_set_scroll_mode},
  {"get_scroll_mode", lavt_get_scroll_mode},
  {"newline_mode", lavt_newline_mode},
  {"set_auto_margin", lavt_set_auto_margin},
  {"get_auto_margin", lavt_get_auto_margin},
  {"set_origin_mode", lavt_set_origin_mode},
  {"get_origin_mode", lavt_get_origin_mode},
  {"set_mouse_visible", lavt_set_mouse_visible},
  {"lock_updates", lavt_lock_updates},
  {"insert_spaces", lavt_insert_spaces},
  {"delete_characters", lavt_delete_characters},
  {"erase_characters", lavt_erase_characters},
  {"backspace", lavt_backspace},
  {"file_selection", lavt_file_selection},
  {"color_selection", lavt_color_selection},
  {"get_directory", lavt_getcwd},
  {"set_directory", lavt_chdir},
  {"chdir", lavt_chdir},
  {"directory_entries", lavt_directory_entries},
  {"entry_type", lavt_entry_type},
  {"launch", lavt_launch},
  {"menu", lavt_menu},
  {"long_menu", lavt_menu},
  {"subprogram", lavt_subprogram},
  {"optional", lavt_optional},
  {"search", lavt_search},
  {"translate", lavt_translate},
  {NULL, NULL}
};

static const luaL_Reg audiolib[] = {
  {"play", laudio_play},
  {"loop", laudio_loop},
  {"playing", laudio_playing},
  {"free", laudio_free},
  {"__gc", laudio_free},
  {"__call", laudio_play},
  {"__tostring", laudio_tostring},
  {NULL, NULL}
};


#ifdef _WIN32
#include <windows.h>

static void
set_datapath (lua_State * L)
{
  char *avtdatapath, *p;
  char progdir[MAX_PATH + 1];
  DWORD len;

  len = GetModuleFileNameA (NULL, progdir, sizeof (progdir));

  if (len == 0 or len == sizeof (progdir)
      or (p = strrchr (progdir, '\\')) == NULL)
    {
      luaL_error (L, "error with GetModuleFileNameA");
      return;
    }

  *p = '\0';			// cut filename off

  avtdatapath = getenv (AVTDATAPATH);

  if (avtdatapath == NULL)
    lua_pushfstring (L, "%s\\data;%s\\akfavatar;%s\\akfavatar",
		     progdir, getenv ("LOCALAPPDATA"), getenv ("APPDATA"));
  else
    {
      lua_pushstring (L, avtdatapath);
      // replace "!" with the program's directory
      luaL_gsub (L, lua_tostring (L, -1), "!", progdir);
      lua_remove (L, -2);	// remove original string
    }

  lua_setfield (L, -2, "datapath");
}

#else // not _WIN32

static void
set_datapath (lua_State * L)
{
  char *avtdatapath = getenv (AVTDATAPATH);

  if (avtdatapath)
    lua_pushstring (L, avtdatapath);
  else
    lua_pushliteral (L, "/usr/local/share/akfavatar;/usr/share/akfavatar");

  lua_setfield (L, -2, "datapath");
}

#endif // not _WIN32


/*
 * use this as entry point when embedding this in
 * an AKFAvatar application
 * uses no auto-cleanup when lua is closed
 */
int
luaopen_akfavatar_embedded (lua_State * L)
{
  avt_audio **audio;

  luaL_newlib (L, akfavtlib);

  // make a reference in the registry
  lua_pushvalue (L, -1);
  lua_setfield (L, LUA_REGISTRYINDEX, AVTMODULE);

  // avt.datapath
  set_datapath (L);

  // variables
  // avt.dirsep
  lua_pushliteral (L, LUA_DIRSEP);
  lua_setfield (L, -2, "dirsep");

  // avt.language
  const char *language = avta_get_language ();
  if (language)
    {
      lua_pushstring (L, language);
      lua_setfield (L, -2, "language");
    }

  // type for audio data
  luaL_newmetatable (L, AUDIODATA);
  lua_pushvalue (L, -1);
  lua_setfield (L, -2, "__index");	// use metatabe itself for indexing
  luaL_setfuncs (L, audiolib, 0);
  lua_pop (L, 1);		// pop metatable

  // create a reusable silent sound
  audio = (avt_audio **) lua_newuserdata (L, sizeof (avt_audio *));
  *audio = NULL;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-silence");

  return 1;
}

/*
 * entry point when calling it as module
 * automatically closes AKFAvatar when Lua is closed
 */
int
luaopen_akfavatar (lua_State * L)
{
  luaopen_akfavatar_embedded (L);

  /*
   * use some dummy userdata to register a
   * cleanup function for the garbage collector
   */
  lua_newuserdata (L, 1);
  lua_newtable (L);		// create metatable
  lua_pushcfunction (L, lavt_quit);	// function for collector
  lua_setfield (L, -2, "__gc");
  lua_setmetatable (L, -2);	// set it up as metatable
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-module_quit");

  return 1;
}
