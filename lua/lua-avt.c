/*
 * Lua 5.1 binding for AKFAvatar
 * Copyright (c) 2008, 2009, 2010 Andreas K. Foerster <info@akfoerster.de>
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

#include "akfavatar.h"
#include "avtaddons.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <stdlib.h>		/* for exit() and wchar_t */
#include <string.h>		/* for strcmp(), strerror() */
#include <errno.h>

#include <unistd.h>		/* for chdir(), getcwd() */

/* MinGW and Wine don't know ENOMSG */
#ifndef ENOMSG
#  define ENOMSG EINVAL
#endif

/* internal name for audio data */
#define AUDIODATA   "AKFAvatar-Audio"

static avt_bool_t initialized = AVT_FALSE;

static const char *const modes[] =
  { "auto", "window", "fullscreen", "fullscreen no switch", NULL };

/* "check()" checks the returned status code */
#define check(X)  do { if (X != AVT_NORMAL) quit (L); } while (0)
#define is_initialized(void)  if (!initialized) auto_initialize(L)

/* for internal use only */
static int
quit (lua_State * L)
{
  if (avt_get_status () <= AVT_ERROR)
    {
      return luaL_error (L, "%s", avt_get_error ());
    }
  else				/* stop requested */
    {
      /* no actual error, so no error message */
      /* this is handled by the calling program */
      lua_pushnil (L);
      return lua_error (L);
    }
}

static void
auto_initialize (lua_State * L)
{
  /* it might be initialized outside of this module */
  if (!avt_initialized ())
    check (avt_initialize (NULL, NULL, avt_default (), AVT_WINDOW));

  initialized = AVT_TRUE;
}

static avt_image_t *
get_avatar (lua_State * L, int index)
{
  if (lua_isnil (L, index))
    return avt_default ();

  if (!lua_isstring (L, index))
    {
      luaL_error (L, "avatar: %s: %s", lua_typename (L, lua_type (L, index)),
		  strerror (ENOMSG));
      return NULL;
    }
  else				/* it is a string */
    {
      size_t len;
      const char *str = lua_tolstring (L, index, &len);

      if (strcmp ("default", str) == 0)
	return avt_default ();
      else if (strcmp ("none", str) == 0)
	return (avt_image_t *) NULL;
      else
	{
	  avt_image_t *image;

	  /* check if it is image data */
	  image = avt_import_image_data ((void *) str, len);

	  /* if not, could it be a filename? */
	  if (!image)
	    image = avt_import_image_file (str);

	  if (image)
	    return image;
	  else			/* give up */
	    {
	      luaL_error (L, "cannot load avatar-image");
	      return NULL;
	    }
	}
    }
}

static int
get_mode (lua_State * L, int index)
{
  const char *mode_name;
  int mode = AVT_AUTOMODE;

  mode_name = lua_tostring (L, index);
  if (!mode_name)
    luaL_error (L, "initialize: mode: (string expected, got %s)",
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
	luaL_error (L, "initialize: mode: '%s': %s", mode_name,
		    strerror (ENOMSG));
    }

  return mode;
}


/*
 * expects a table with values for: title, shortname, avatar, encoding, mode
 *
 * title and shortname should be strings
 *
 * avatar may be "default, "none", image data in a string,
 *        or a path to a file
 *
 * audio may be set to true to activate audio
 *
 * encoding is the encoding for strings, for example to "ISO-8859-1"
 *          default is "UTF-8"
 *
 * mode is one of "auto", "window", "fullscreen", "fullscreen no switch"
 *
 * No parameter is needed, use them as you wish.
 */
static int
lavt_initialize (lua_State * L)
{
  const char *title, *shortname;
  const char *encoding;
  avt_image_t *avatar;
  avt_bool_t audio;
  int mode;

  title = shortname = NULL;
  avatar = (avt_image_t *) (&avatar);	/* dummy value, not NULL */
  encoding = "UTF-8";
  mode = AVT_AUTOMODE;
  audio = AVT_FALSE;

  if (lua_isnone (L, 1))	/* no argument */
    {
      avatar = avt_default ();
    }
  else				/* has an argument */
    {
      luaL_checktype (L, 1, LUA_TTABLE);

      lua_pushnil (L);		/* initial dummy key */
      while (lua_next (L, 1))
	{
	  const char *key;

	  if (lua_type (L, -2) != LUA_TSTRING)
	    luaL_error (L, "initialize: %s: %s",
			lua_typename (L, lua_type (L, -2)),
			strerror (ENOMSG));

	  key = lua_tostring (L, -2);	/* known to be a string */

	  if (strcmp ("title", key) == 0)
	    title = lua_tostring (L, -1);
	  else if (strcmp ("shortname", key) == 0)
	    shortname = lua_tostring (L, -1);
	  else if (strcmp ("avatar", key) == 0)
	    avatar = get_avatar (L, -1);
	  else if (strcmp ("audio", key) == 0)
	    audio = (avt_bool_t) lua_toboolean (L, -1);
	  else if (strcmp ("encoding", key) == 0)
	    encoding = lua_tostring (L, -1);
	  else if (strcmp ("mode", key) == 0)
	    mode = get_mode (L, -1);
	  else
	    luaL_error (L, "initialize: %s: %s", key, strerror (EINVAL));

	  lua_pop (L, 1);	/* remove value, keep key */
	}			/* while (lua_next... */

      lua_pop (L, 1);		/* pop key */
    }				/* if (lua_isnone...) else */

  if (!shortname)
    shortname = title;

  /* if avatar was not set */
  if (avatar == (avt_image_t *) (&avatar))
    avatar = avt_default ();

  if (!initialized && !avt_initialized ())
    {
      check (avt_mb_encoding (encoding));
      check (avt_initialize (title, shortname, avatar, mode));
      if (audio)
	check (avt_initialize_audio ());
    }
  else				/* already initialized */
    {
      /* reset almost everything */
      /* not the background color - should be set before initialization */
      avt_clear_screen ();
      avt_set_balloon_size (0, 0);
      avt_newline_mode (AVT_TRUE);
      avt_set_auto_margin (AVT_TRUE);
      avt_set_origin_mode (AVT_TRUE);
      avt_set_scroll_mode (1);
      avt_reserve_single_keys (AVT_FALSE);
      avt_set_balloon_color_name ("floral white");
      avt_normal_text ();

      check (avt_mb_encoding (encoding));
      avt_set_title (title, shortname);
      check (avt_change_avatar_image (avatar));

      if (mode != AVT_AUTOMODE)
	avt_switch_mode (mode);

      if (audio)
	avt_initialize_audio ();
      else
	avt_quit_audio ();
    }

  initialized = AVT_TRUE;
  return 0;
}

/* quit the avatar subsystem (closes the window) */
static int
lavt_quit (lua_State * L AVT_UNUSED)
{
  avt_quit ();
  initialized = AVT_FALSE;
  return 0;
}

/* returns version string */
static int
lavt_version (lua_State * L)
{
  lua_pushstring (L, avt_version ());
  return 1;
}

/* returns copyright string */
static int
lavt_copyright (lua_State * L)
{
  lua_pushstring (L, avt_copyright ());
  return 1;
}

/* returns license string */
static int
lavt_license (lua_State * L)
{
  lua_pushstring (L, avt_license ());
  return 1;
}

/* returns whether it is initialized */
static int
lavt_initialized (lua_State * L)
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
  int red, green, blue;

  name = avt_get_color (luaL_checkint (L, 1), &red, &green, &blue);

  if (name)
    {
      sprintf (RGB, "#%02X%02X%02X", red, green, blue);
      lua_pushstring (L, name);
      lua_pushstring (L, RGB);
      return 2;
    }
  else
    return 0;
}

/* change the used encoding (iconv) */
static int
lavt_encoding (lua_State * L)
{
  check (avt_mb_encoding (luaL_checkstring (L, 1)));
  return 0;
}

/* get the current encoding (iconv) */
static int
lavt_get_encoding (lua_State * L)
{
  lua_pushstring (L, avt_get_mb_encoding ());
  return 1;
}

/*
 * change avatar image while running
 * if the avatar is visible, the screen gets cleared
 * the image may be given like in avt.initialize
 */
static int
lavt_change_avatar_image (lua_State * L)
{
  is_initialized ();
  check (avt_change_avatar_image (get_avatar (L, 1)));
  return 0;
}

/*
 * set the name of the avatar
 * must be after avt.change_avatar_image
 */
static int
lavt_set_avatar_name (lua_State * L)
{
  is_initialized ();
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
 * get image from string
 * on error it returns nil
 * if it succeeds call avt.wait or avt.waitkey
 */
static int
lavt_show_image_string (lua_State * L)
{
  char *data;
  size_t len;

  is_initialized ();
  data = (char *) luaL_checklstring (L, 1, &len);
  lua_pushboolean (L, (int) (avt_show_image_data (data, len) == AVT_NORMAL));
  return 1;
}

/* change title and/or icontitle */
/* a missing option or "nil" leaves it unchanged */
static int
lavt_set_title (lua_State * L)
{
  avt_set_title (lua_tostring (L, 1), lua_tostring (L, 2));
  return 0;
}

/* right to left writing (true/false) */
static int
lavt_right_to_left (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_text_direction ((int) lua_toboolean (L, 1));
  return 0;
}

/* set_text_delay */
/* 0 for no delay, no value for default delay */
static int
lavt_set_text_delay (lua_State * L)
{
  avt_set_text_delay (luaL_optint (L, 1, AVT_DEFAULT_TEXT_DELAY));
  return 0;
}

/* set flip-page delay */
/* 0 for no delay, no value for default delay */
static int
lavt_set_flip_page_delay (lua_State * L)
{
  avt_set_flip_page_delay (luaL_optint (L, 1, AVT_DEFAULT_TEXT_DELAY));
  return 0;
}

/* set balloon size (height, width) */
/* no values sets the maximum */
static int
lavt_set_balloon_size (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_size (lua_tointeger (L, 1), lua_tointeger (L, 2));
  return 0;
}

/* set balloon width */
/* no value sets the maximum */
static int
lavt_set_balloon_width (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_width (lua_tointeger (L, 1));
  return 0;
}

/* set balloon height */
/* no value sets the maximum */
static int
lavt_set_balloon_height (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_height (lua_tointeger (L, 1));
  return 0;
}

/* set balloon color (name) */
static int
lavt_set_balloon_color (lua_State * L)
{
  is_initialized ();
  avt_set_balloon_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* show cursor (true or false) */
static int
lavt_activate_cursor (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_activate_cursor ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

/* underline mode (true or false) */
static int
lavt_underlined (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_underlined ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

/* return underline mode (true or false) */
static int
lavt_get_underlined (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_underlined ());
  return 1;
}

/* bold mode (true or false) */
static int
lavt_bold (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_bold ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

/* return bold mode (true or false) */
static int
lavt_get_bold (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_bold ());
  return 1;
}

/* inverse mode (true or false) */
static int
lavt_inverse (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_inverse ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

/* return inverse mode (true or false) */
static int
lavt_get_inverse (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_inverse ());
  return 1;
}

static int
lavt_markup (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_markup ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

/* reset to normal text mode */
static int
lavt_normal_text (lua_State * L AVT_UNUSED)
{
  avt_normal_text ();
  return 0;
}

/* clear the whole screen */
static int
lavt_clear_screen (lua_State * L AVT_UNUSED)
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
lavt_clear (lua_State * L AVT_UNUSED)
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
lavt_clear_down (lua_State * L AVT_UNUSED)
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
lavt_clear_eol (lua_State * L AVT_UNUSED)
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
lavt_clear_bol (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_clear_bol ();
  return 0;
}

/* clear line */
static int
lavt_clear_line (lua_State * L AVT_UNUSED)
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
lavt_clear_up (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_clear_up ();
  return 0;
}

/* show only the avatar */
static int
lavt_show_avatar (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_show_avatar ();
  return 0;
}

/* move in */
static int
lavt_move_in (lua_State * L)
{
  is_initialized ();
  check (avt_move_in ());
  return 0;
}

/* move out */
static int
lavt_move_out (lua_State * L)
{
  is_initialized ();
  check (avt_move_out ());
  return 0;
}

/* bell or flash if sound is not initialized */
static int
lavt_bell (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_bell ();
  return 0;
}

/* flash */
static int
lavt_flash (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_flash ();
  return 0;
}

/* background color of window (name) */
static int
lavt_set_background_color (lua_State * L)
{
  avt_set_background_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* text color (name | #RRGGBB) */
static int
lavt_set_text_color (lua_State * L)
{
  avt_set_text_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* background color of text (name | #RRGGBB) */
static int
lavt_set_text_background_color (lua_State * L)
{
  avt_set_text_background_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* set background color of text to ballooncolor */
static int
lavt_set_text_background_ballooncolor (lua_State * L AVT_UNUSED)
{
  avt_set_text_background_ballooncolor ();
  return 0;
}

/* get x position */
static int
lavt_where_x (lua_State * L)
{
  lua_pushinteger (L, avt_where_x ());
  return 1;
}

/* get y position */
static int
lavt_where_y (lua_State * L)
{
  lua_pushinteger (L, avt_where_y ());
  return 1;
}

/* get maximum x position */
static int
lavt_get_max_x (lua_State * L)
{
  lua_pushinteger (L, avt_get_max_x ());
  return 1;
}

/* get maximum y position */
static int
lavt_get_max_y (lua_State * L)
{
  lua_pushinteger (L, avt_get_max_y ());
  return 1;
}

/* is the cursor in the home position? */
/* (also works for right-to-left writing) */
static int
lavt_home_position (lua_State * L)
{
  lua_pushboolean (L, (int) avt_home_position ());
  return 1;
}

/* move to x position */
static int
lavt_move_x (lua_State * L)
{
  is_initialized ();
  avt_move_x (luaL_checkint (L, 1));
  return 0;
}

/* move to y position */
static int
lavt_move_y (lua_State * L)
{
  is_initialized ();
  avt_move_y (luaL_checkint (L, 1));
  return 0;
}

/* move to x and y position */
static int
lavt_move_xy (lua_State * L)
{
  is_initialized ();
  avt_move_xy (luaL_checkint (L, 1), luaL_checkint (L, 2));
  return 0;
}

/* save cursor position */
static int
lavt_save_position (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_save_position ();
  return 0;
}

/* restore cursor position */
static int
lavt_restore_position (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_restore_position ();
  return 0;
}

/* next tab position */
static int
lavt_next_tab (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_next_tab ();
  return 0;
}

/* last tab position */
static int
lavt_last_tab (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_last_tab ();
  return 0;
}

/* reset tab stops to every eigth column */
static int
lavt_reset_tab_stops (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_reset_tab_stops ();
  return 0;
}

/* clear all tab stops */
static int
lavt_clear_tab_stops (lua_State * L AVT_UNUSED)
{
  is_initialized ();
  avt_clear_tab_stops ();
  return 0;
}

/* set or clear tab in position x */
static int
lavt_set_tab (lua_State * L)
{
  is_initialized ();
  luaL_checktype (L, 1, LUA_TNUMBER);
  luaL_checktype (L, 2, LUA_TBOOLEAN);
  avt_set_tab (lua_tointeger (L, 1), (avt_bool_t) lua_toboolean (L, 2));
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

/* wait until a button is pressed */
static int
lavt_wait_button (lua_State * L)
{
  is_initialized ();
  check (avt_wait_button ());
  return 0;
}

/* reserve single keys? (true/false) */
static int
lavt_reserve_single_keys (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_reserve_single_keys ((avt_bool_t) lua_toboolean (L, 1));
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
lavt_toggle_fullscreen (lua_State * L AVT_UNUSED)
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

/* wait a given amount of seconds (fraction) */
#define DEF_WAIT (AVT_DEFAULT_FLIP_PAGE_DELAY / 1000.0)
static int
lavt_wait_sec (lua_State * L)
{
  is_initialized ();
  check (avt_wait ((int) (luaL_optnumber (L, 1, DEF_WAIT) * 1000.0)));

  return 0;
}

/* show final credits from a string */
/* 1=text, 2=centered (true/false/nothing) */
static int
lavt_credits (lua_State * L)
{
  is_initialized ();
  check (avt_credits_mb (luaL_checkstring (L, 1),
			 (avt_bool_t) lua_toboolean (L, 2)));

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

/* returns a pressed key (unicode-value) */
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

/* make a positive/negative decision */
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

  /* get string in position 3 */
  c = lua_tostring (L, 3);

  check (avt_choice (&result,
		     luaL_checkint (L, 1),
		     luaL_checkint (L, 2),
		     (c) ? c[0] : 0,
		     (avt_bool_t) lua_toboolean (L, 4),
		     (avt_bool_t) lua_toboolean (L, 5)));

  lua_pushinteger (L, result);
  return 1;
}

/* works like "io.write", but writes in the balloon */
/* expects one or more strings or numbers */
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

/* works like "print", but prints in the balloon */
/* expects one or more strings or numbers */
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
      lua_pushvalue (L, -1);	/* function "tostring" */
      lua_pushvalue (L, i);	/* value */
      lua_call (L, 1, 1);
      s = lua_tolstring (L, -1, &len);
      lua_pop (L, 1);		/* pop result of "tostring" */
      if (s)
	{
	  if (i > 1)
	    avt_next_tab ();
	  check (avt_say_mb_len (s, len));
	}
    }

  lua_pop (L, 1);		/* pop function */
  avt_new_line ();

  return 0;
}

static int
lavt_tell (lua_State * L)
{
  const char *s;
  size_t len;

  is_initialized ();
  lua_concat (L, lua_gettop (L));	/* make it one single string */

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
		   (int) avt_printable ((wchar_t) luaL_checkinteger (L, 1)));
  return 1;
}

static int
lavt_unicode_to_utf8 (lua_State * L)
{
  int nr, i;
  luaL_Buffer buffer;
  avt_char c;

  is_initialized ();

  nr = lua_gettop (L);

  luaL_buffinit (L, &buffer);

  for (i = 1; i <= nr; i++)
    {
      c = (avt_char) luaL_checkinteger (L, i);

      /*
       * first check for invalid codes
       * UTF-16 surrugates are also invalid in UTF-8
       * they must be treated outside of this function
       */
      if (c < 0 || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF))
	{
	  luaL_addstring (&buffer, "\xEF\xBF\xBD");
	}
      else if (c <= 0x7F)
	{
	  luaL_addchar (&buffer, (char) c);
	}
      else if (c <= 0x7FF)
	{
	  luaL_addchar (&buffer, (char) (0xC0 | (c >> 6)));
	  luaL_addchar (&buffer, (char) (0x80 | (c & 0x3F)));
	}
      else if (c <= 0xFFFF)
	{
	  luaL_addchar (&buffer, (char) (0xE0 | (c >> 12)));
	  luaL_addchar (&buffer, (char) (0x80 | ((c >> 6) & 0x3F)));
	  luaL_addchar (&buffer, (char) (0x80 | (c & 0x3F)));
	}
      else
	{
	  luaL_addchar (&buffer, (char) (0xF0 | (c >> 18)));
	  luaL_addchar (&buffer, (char) (0x80 | ((c >> 12) & 0x3F)));
	  luaL_addchar (&buffer, (char) (0x80 | ((c >> 6) & 0x3F)));
	  luaL_addchar (&buffer, (char) (0x80 | (c & 0x3F)));
	}
    }

  luaL_pushresult (&buffer);

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
lavt_initialize_audio (lua_State * L)
{
  if (avt_initialize_audio ())
    {
      lua_pushnil (L);
      lua_pushstring (L, avt_get_error ());
      return 2;
    }
  else
    {
      lua_pushboolean (L, AVT_TRUE);
      return 1;
    }
}

/* shut down the audio-system */
static int
lavt_quit_audio (lua_State * L AVT_UNUSED)
{
  avt_quit_audio ();
  return 0;
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
  avt_audio_t **audio;

  filename = "";
  len = 0;
  if (!lua_isnoneornil (L, 1))
    filename = luaL_checklstring (L, 1, &len);

  /* promblem: the garbage collector doesn't see, how heavy this is */
  audio = (avt_audio_t **) lua_newuserdata (L, sizeof (avt_audio_t *));
  *audio = NULL;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);

  if (len > 0)
    *audio = avt_load_audio_file (filename);

  return 1;
}

static int
lavt_load_audio_string (lua_State * L)
{
  char *data;
  size_t len;
  avt_audio_t **audio;

  data = "";
  len = 0;
  if (!lua_isnoneornil (L, 1))
    data = (char *) luaL_checklstring (L, 1, &len);

  /* promblem: the garbage collector doesn't see, how heavy this is */
  audio = (avt_audio_t **) lua_newuserdata (L, sizeof (avt_audio_t *));
  *audio = NULL;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);

  if (len > 0)
    *audio = avt_load_audio_data (data, len);

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
  return 0;
}

static int
lavt_stop_audio (lua_State * L AVT_UNUSED)
{
  avt_stop_audio ();
  return 0;
}

/* plays audio data */
static int
laudio_play (lua_State * L)
{
  avt_audio_t **audio;

  audio = (avt_audio_t **) luaL_checkudata (L, 1, AUDIODATA);

  if (audio && *audio)
    avt_play_audio (*audio, AVT_FALSE);	/* no check! */

  return 0;
}

/* plays audio data in a loop */
static int
laudio_loop (lua_State * L)
{
  avt_audio_t **audio;

  audio = (avt_audio_t **) luaL_checkudata (L, 1, AUDIODATA);

  if (audio && *audio)
    avt_play_audio (*audio, AVT_TRUE);	/* no check! */

  return 0;
}

/* frees audio data */
static int
laudio_free (lua_State * L)
{
  avt_audio_t **audio;

  audio = (avt_audio_t **) luaL_checkudata (L, 1, AUDIODATA);
  if (audio && *audio)
    {
      avt_free_audio (*audio);
      *audio = NULL;
    }

  return 0;
}

/* checks if audio is still playing */
static int
laudio_playing (lua_State * L)
{
  avt_audio_t **audio;

  if (lua_isnoneornil (L, 1))
    lua_pushboolean (L, avt_audio_playing (NULL));
  else
    {
      audio = (avt_audio_t **) luaL_checkudata (L, 1, AUDIODATA);
      lua_pushboolean (L, (audio && *audio && avt_audio_playing (*audio)));
    }

  return 1;
}

static int
laudio_tostring (lua_State * L)
{
  avt_audio_t **audio;

  audio = (avt_audio_t **) luaL_checkudata (L, 1, AUDIODATA);

  if (audio && *audio)
    lua_pushfstring (L, AUDIODATA " (%p)", audio);
  else
    lua_pushfstring (L, AUDIODATA ", empty (%p)", audio);

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
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_newline_mode ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

static int
lavt_set_auto_margin (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_set_auto_margin ((avt_bool_t) lua_toboolean (L, 1));
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
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_set_origin_mode ((avt_bool_t) lua_toboolean (L, 1));
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
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_set_mouse_visible ((avt_bool_t) lua_toboolean (L, 1));
  return 0;
}

static int
lavt_lock_updates (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_lock_updates ((avt_bool_t) lua_toboolean (L, 1));
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
      if (lua_isnil (L, -1))	/* just a quit-request? */
	{
	  avt_set_status (AVT_NORMAL);
	  return 0;
	}
      else
	return lua_error (L);
    }

  return lua_gettop (L);
}

/* --------------------------------------------------------- */
/* avtaddons.h */

/* avt.file_selection (filter) */

/* we need to temporarily store a pointer to the Lua_state :-( */
static lua_State *tmp_lua_state;

/* call the function at index 1 with the filename */
static avt_bool_t
file_filter (const char *filename)
{
  avt_bool_t result;

  lua_pushvalue (tmp_lua_state, 1);	/* push func again, to keep it */
  lua_pushstring (tmp_lua_state, filename);	/* parameter */
  lua_call (tmp_lua_state, 1, 1);
  result = (avt_bool_t) lua_toboolean (tmp_lua_state, -1);
  lua_pop (tmp_lua_state, 1);	/* pop result, leave func on stack */

  return result;
}

static int
lavt_file_selection (lua_State * L)
{
  char filename[256];
  avta_filter_t filter;

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

/* --------------------------------------------------------- */
/* system calls */

static int
lavt_chdir (lua_State * L)
{
  /* chdir() conforms to POSIX.1-2001 */
  chdir (luaL_checkstring (L, 1));
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
  while (AVT_TRUE)
    {
      char *buffer;
      int error_nr;

      buffer = (char *) malloc (size);
      if (buffer == NULL)
	avta_error ("Lua-AKFAvatar", strerror (errno));

      /* getcwd() conforms to POSIX.1-2001 */
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
	  /* unsolvable error */
	  lua_pushnil (L);
	  lua_pushstring (L, strerror (error_nr));
	  return 2;
	}

      /* try again with a larger buffer */
      size *= 2;
    }
}

/* --------------------------------------------------------- */
/* high level functions */

/* three arrows up */
#define BACK L"\x2191 \x2191 \x2191"

/* three arrows down */
#define CONTINUE L"\x2193 \x2193 \x2193"

#define MARK(S) \
         do { \
           avt_set_text_background_color (0xdd, 0xdd, 0xdd); \
           avt_clear_line (); \
           avt_move_x (mid_x-(sizeof(S)/sizeof(wchar_t)-1)/2); \
           avt_say(S); \
           avt_normal_text(); \
         } while(0)

static int
lavt_long_menu (lua_State * L)
{
  long int item_nr;
  const char *item_desc;
  int i;
  int start_line, menu_start;
  int max_idx, items, page_nr, items_per_page;
  int choice;
  int mid_x;
  size_t len;
  avt_bool_t old_auto_margin;

  is_initialized ();
  luaL_checktype (L, 1, LUA_TTABLE);

  avt_set_text_delay (0);
  avt_normal_text ();
  avt_lock_updates (AVT_TRUE);

  start_line = avt_where_y ();
  if (start_line < 1)		/* no balloon yet? */
    start_line = 1;

  mid_x = avt_get_max_x () / 2;	/* used by MARK() */
  max_idx = avt_get_max_y () - start_line + 1;

  item_desc = NULL;
  items = 0;
  item_nr = 0;
  page_nr = 0;
  items_per_page = max_idx - 2;

  old_auto_margin = avt_get_auto_margin ();
  avt_set_auto_margin (AVT_FALSE);

  while (!item_nr)
    {
      avt_move_xy (1, start_line);
      avt_clear_down ();

      if (page_nr > 0)
	MARK (BACK);
      else
	MARK (L"");

      items = 1;
      avt_new_line ();

      for (i = 1; i <= items_per_page; i++)
	{
	  lua_rawgeti (L, 1, i + (page_nr * items_per_page));
	  item_desc = lua_tolstring (L, -1, &len);

	  if (item_desc)
	    {
	      avt_say_mb_len (item_desc, len);
	      avt_new_line ();
	      items++;
	    }

	  lua_pop (L, 1);	/* pop item description from stack */
	  /* from now on item_desc should not be dereferenced */

	  if (!item_desc)
	    break;
	}

      /* are there more items? */
      if (item_desc)
	{
	  lua_rawgeti (L, 1, (page_nr + 1) * items_per_page + 1);
	  item_desc = lua_tolstring (L, -1, &len);
	  if (item_desc)
	    {
	      MARK (CONTINUE);
	      items = max_idx;
	    }
	  lua_pop (L, 1);	/* pop item description from stack */
	}

      menu_start = start_line;
      if (page_nr == 0)
	{
	  menu_start++;
	  items--;
	}

      avt_lock_updates (AVT_FALSE);
      check (avt_choice (&choice, menu_start, items, 0,
			 (page_nr > 0), (item_desc != NULL)));
      avt_lock_updates (AVT_TRUE);

      if (page_nr == 0)
	choice++;

      if (choice == 1 && page_nr > 0)
	page_nr--;		/* page back */
      else if (choice == max_idx)
	page_nr += (item_desc == NULL) ? 0 : 1;	/* page forward */
      else
	item_nr = choice - 1 + (page_nr * items_per_page);
    }

  avt_set_auto_margin (old_auto_margin);
  avt_clear ();
  avt_lock_updates (AVT_FALSE);

  lua_pushinteger (L, item_nr);
  return 1;
}

/* --------------------------------------------------------- */
/* register library functions */

static const struct luaL_reg akfavtlib[] = {
  {"initialize", lavt_initialize},
  {"quit", lavt_quit},
  {"change_avatar_image", lavt_change_avatar_image},
  {"set_avatar_name", lavt_set_avatar_name},
  {"say", lavt_say},
  {"write", lavt_say},		/* alias */
  {"print", lavt_print},
  {"tell", lavt_tell},
  {"say_unicode", lavt_say_unicode},
  {"unicode_to_utf8", lavt_unicode_to_utf8},
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
  {"initialized", lavt_initialized},
  {"get_color", lavt_get_color},
  {"encoding", lavt_encoding},
  {"get_encoding", lavt_get_encoding},
  {"set_title", lavt_set_title},
  {"set_text_delay", lavt_set_text_delay},
  {"set_flip_page_delay", lavt_set_flip_page_delay},
  {"right_to_left", lavt_right_to_left},
  {"flip_page", lavt_flip_page},
  {"update", lavt_update},
  {"wait", lavt_wait_sec},
  {"set_balloon_size", lavt_set_balloon_size},
  {"set_balloon_width", lavt_set_balloon_width},
  {"set_balloon_height", lavt_set_balloon_height},
  {"set_balloon_color", lavt_set_balloon_color},
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
  {"show_image_file", lavt_show_image_file},
  {"show_image_string", lavt_show_image_string},
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
  {"initialize_audio", lavt_initialize_audio},
  {"quit_audio", lavt_quit_audio},
  {"load_audio_file", lavt_load_audio_file},
  {"load_audio_string", lavt_load_audio_string},
  {"audio_playing", laudio_playing},
  {"wait_audio_end", lavt_wait_audio_end},
  {"stop_audio", lavt_stop_audio},
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
  {"long_menu", lavt_long_menu},
  {"subprogram", lavt_subprogram},
  {NULL, NULL}
};

static const struct luaL_reg audiolib[] = {
  {"play", laudio_play},
  {"loop", laudio_loop},
  {"playing", laudio_playing},
  {"free", laudio_free},
  {"__gc", laudio_free},
  {"__call", laudio_play},
  {"__tostring", laudio_tostring},
  {NULL, NULL}
};

int
luaopen_akfavatar (lua_State * L)
{
  luaL_register (L, "avt", akfavtlib);

#ifdef MODULE
  /*
   * use some dummy userdata to register a
   * cleanup function for the garbage collector
   */
  lua_newuserdata (L, 1);
  lua_newtable (L);		/* create metatable */
  lua_pushcfunction (L, lavt_quit);	/* function for collector */
  lua_setfield (L, -2, "__gc");
  lua_setmetatable (L, -2);	/* set it up as metatable */
  lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-module_quit");
#endif

  /* type for audio data */
  luaL_newmetatable (L, AUDIODATA);
  lua_pushvalue (L, -1);
  lua_setfield (L, -2, "__index");	/* use metatabe itself for indexing */
  luaL_register (L, NULL, audiolib);
  lua_pop (L, 1);		/* pop metatable */

  return 1;
}
