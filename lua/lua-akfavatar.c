/*
 * Lua 5.1 binding for AKFAvatar
 * Copyright (c) 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
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

/*
 * parameters:
 * 1 title
 * 2 icontitle
 * 3 avatar-image (using avt.default() or avt.import_image_file)
 * 4 mode (0=window, 1=fullscreen, 2=fullscreen without switching)
 *
 * no parameter is needed, use as many as you wish, just keep the order
 * you can also use the value "nil" for any of them
 */
static int
lavt_initialize (lua_State * L)
{
  const char *title, *icontitle;
  avt_image_t *image;
  int mode;

  title = lua_tostring (L, 1);
  icontitle = lua_tostring (L, 2);
  image = (avt_image_t *) lua_touserdata (L, 3);
  mode = lua_tointeger (L, 4);

  if (!image)
    image = avt_default ();

  if (!icontitle)
    icontitle = title;

  if (!avt_initialized ())
    lua_pushinteger (L, avt_initialize (title, icontitle, image, mode));
  else				/* already initialized */
    {
      avt_set_title (title, icontitle);
      avt_change_avatar_image (image);
      avt_switch_mode (mode);
      lua_pushinteger (L, avt_get_status ());
    }

  return 1;
}

/*
 * initializes the audio system
 * call this directly after avt.initialize()
 * this also affects avt.bell() and printing "\a"
 */
static int
lavt_initialize_audio (lua_State * L)
{
  lua_pushinteger (L, avt_initialize_audio ());
  return 1;
}

/* quit the avatar subsystem */
static int
lavt_quit (lua_State * L)
{
  avt_quit ();
  return 0;
}

/* show button, move out and quit */
static int
lavt_button_quit (lua_State * L)
{
  avt_button_quit ();
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

/* returns the status number */
/* 0=normal, 1=halt requested, -1=fatal error */
static int
lavt_get_status (lua_State * L)
{
  lua_pushinteger (L, avt_get_status ());
  return 1;
}

/* set status (see above) */
static int
lavt_set_status (lua_State * L)
{
  avt_set_status (luaL_checkint (L, 1));
  return 0;
}

/* get error message, when error occured */
static int
lavt_get_error (lua_State * L)
{
  lua_pushstring (L, avt_get_error ());
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

/* set the used encoding (iconv) */
/* default is UTF-8 */
static int
lavt_encoding (lua_State * L)
{
  lua_pushinteger (L, avt_mb_encoding (luaL_checkstring (L, 1)));
  return 1;
}

/* get the default avatar */
static int
lavt_default (lua_State * L)
{
  lua_pushlightuserdata (L, avt_default ());
  return 1;
}

/* get avatar-image from file */
/* to be used with avt.initialize or avt.change_avatar_image */
static int
lavt_import_image_file (lua_State * L)
{
  lua_pushlightuserdata (L, avt_import_image_file (luaL_checkstring (L, 1)));
  return 1;
}

/* get avatar-image from string */
/* to be used with avt.initialize or avt.change_avatar_image */
static int
lavt_import_image_string (lua_State * L)
{
  char *data;
  size_t len;

  data = (char *) luaL_checklstring (L, 1, &len);
  lua_pushlightuserdata (L, avt_import_image_data (data, len));
  return 1;
}

/* free image data */
static int
lavt_free_image (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TLIGHTUSERDATA);
  avt_free_image ((avt_image_t *) lua_touserdata (L, 1));
  return 0;
}

/*
 * change avatar image while running
 * if the avatar is visible, the screen gets cleared
 * the original image is freed in this function!
 * the image may be nil or nothing if no avatar should be shown
 * on error AVT_ERROR is set and returned
 */
static int
lavt_change_avatar_image (lua_State * L)
{
  lua_pushinteger (L, avt_change_avatar_image (lua_touserdata (L, 1)));
  return 1;
}

/*
 * set the name of the avatar
 * must be after change_avatar_image
 */
static int
lavt_set_avatar_name (lua_State * L)
{
  lua_pushinteger (L, avt_set_avatar_name_mb (lua_tostring (L, 1)));
  return 1;
}

/*
 * load image and show it
 * on error it returns AVT_ERROR without changing the status
 * if it succeeds call avt_wait or avt_waitkey
 */
static int
lavt_show_image_file (lua_State * L)
{
  lua_pushinteger (L, avt_show_image_file (luaL_checkstring (L, 1)));
  return 1;
}

/* get avatar-image from string */
/* to be used with avt.initialize or avt.change_avatar_image */
static int
lavt_show_image_string (lua_State * L)
{
  char *data;
  size_t len;

  data = (char *) luaL_checklstring (L, 1, &len);
  lua_pushinteger (L, avt_show_image_data (data, len));
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
  avt_text_direction (lua_toboolean (L, 1));
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
  avt_set_balloon_size (lua_tointeger (L, 1), lua_tointeger (L, 2));
  return 0;
}

/* set balloon width */
/* no value sets the maximum */
static int
lavt_set_balloon_width (lua_State * L)
{
  avt_set_balloon_width (lua_tointeger (L, 1));
  return 0;
}

/* set balloon height */
/* no value sets the maximum */
static int
lavt_set_balloon_height (lua_State * L)
{
  avt_set_balloon_height (lua_tointeger (L, 1));
  return 0;
}

/* set balloon color (name) */
static int
lavt_set_balloon_color (lua_State * L)
{
  avt_set_balloon_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* show cursor (true or false) */
static int
lavt_activate_cursor (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_activate_cursor (lua_toboolean (L, 1));
  return 0;
}

/* underline mode (true or false) */
static int
lavt_underlined (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_underlined (lua_toboolean (L, 1));
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
  avt_bold (lua_toboolean (L, 1));
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
  avt_inverse (lua_toboolean (L, 1));
  return 0;
}

/* return inverse mode (true or false) */
static int
lavt_get_inverse (lua_State * L)
{
  lua_pushboolean (L, (int) avt_get_inverse ());
  return 1;
}

/* reset to normal text mode */
static int
lavt_normal_text (lua_State * L)
{
  avt_normal_text ();
  return 0;
}

/* clear the whole screen */
static int
lavt_clear_screen (lua_State * L)
{
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
  avt_clear_bol ();
  return 0;
}

/* clear line */
static int
lavt_clear_line (lua_State * L)
{
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
  avt_clear_up ();
  return 0;
}

/* show only the avatar */
static int
lavt_show_avatar (lua_State * L)
{
  avt_show_avatar ();
  return 0;
}

/* move in */
/* returns the status */
static int
lavt_move_in (lua_State * L)
{
  lua_pushinteger (L, avt_move_in ());
  return 1;
}

/* move out */
/* returns the status */
static int
lavt_move_out (lua_State * L)
{
  lua_pushinteger (L, avt_move_out ());
  return 1;
}

/* bell or flash if sound is not initialized */
static int
lavt_bell (lua_State * L)
{
  avt_bell ();
  return 0;
}

/* flash */
static int
lavt_flash (lua_State * L)
{
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

/* text color (name) */
static int
lavt_set_text_color (lua_State * L)
{
  avt_set_text_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* background color of text (name) */
static int
lavt_set_text_background_color (lua_State * L)
{
  avt_set_text_background_color_name (luaL_checkstring (L, 1));
  return 0;
}

/* set background color of text to ballooncolor */
static int
lavt_set_text_background_ballooncolor (lua_State * L)
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
  avt_move_x (luaL_checkint (L, 1));
  return 0;
}

/* move to y position */
static int
lavt_move_y (lua_State * L)
{
  avt_move_y (luaL_checkint (L, 1));
  return 0;
}

/* move to x and y position */
static int
lavt_move_xy (lua_State * L)
{
  avt_move_xy (luaL_checkint (L, 1), luaL_checkint (L, 2));
  return 0;
}

/* save cursor position */
static int
lavt_save_position (lua_State * L)
{
  avt_save_position ();
  return 0;
}

/* restore cursor position */
static int
lavt_restore_position (lua_State * L)
{
  avt_restore_position ();
  return 0;
}

/* next tab position */
static int
lavt_next_tab (lua_State * L)
{
  avt_next_tab ();
  return 0;
}

/* last tab position */
static int
lavt_last_tab (lua_State * L)
{
  avt_last_tab ();
  return 0;
}

/* reset tab stops to every eigth column */
static int
lavt_reset_tab_stops (lua_State * L)
{
  avt_reset_tab_stops ();
  return 0;
}

/* clear all tab stops */
static int
lavt_clear_tab_stops (lua_State * L)
{
  avt_clear_tab_stops ();
  return 0;
}

/* set or clear tab in position x */
static int
lavt_set_tab (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TNUMBER);
  luaL_checktype (L, 2, LUA_TBOOLEAN);
  avt_set_tab (lua_tointeger (L, 1), lua_toboolean (L, 2));
  return 0;
}

/*
 * delete num lines, starting from line
 * the rest ist scrolled up
 */
static int
lavt_delete_lines (lua_State * L)
{
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
  avt_insert_lines (luaL_checkint (L, 1), luaL_checkint (L, 2));
  return 0;
}

/* wait until a button is pressed */
static int
lavt_wait_button (lua_State * L)
{
  lua_pushinteger (L, avt_wait_button ());
  return 1;
}

/* reserve single keys? (true/false) */
static int
lavt_reserve_single_keys (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_reserve_single_keys (lua_toboolean (L, 1));
  return 0;
}

static int
lavt_switch_mode (lua_State * L)
{
  avt_switch_mode (luaL_checkint (L, 1));
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
  lua_pushinteger (L, avt_flip_page ());
  return 1;
}

static int
lavt_update (lua_State * L)
{
  lua_pushinteger (L, avt_update ());
  return 1;
}

/* wait a given amount of milliseconds */
static int
lavt_wait (lua_State * L)
{
  lua_pushinteger (L, avt_wait (luaL_checkint (L, 1)));
  return 1;
}

/* wait a given amount of seconds (fraction) */
static int
lavt_wait_sec (lua_State * L)
{
  lua_pushinteger (L, avt_wait ((int) (1000.0 * luaL_checknumber (L, 1))));
  return 1;
}

/* show final credits from file */
/* 1=filename, 2=centered (true/false/nothing) */
static int
lavt_credits (lua_State * L)
{
  lua_pushinteger (L, avt_credits_mb (luaL_checkstring (L, 1),
				      lua_toboolean (L, 2)));
  return 1;
}

static int
lavt_newline (lua_State * L)
{
  lua_pushinteger (L, avt_new_line ());
  return 1;
}

static int
lavt_ask (lua_State * L)
{
  char buf[4 * AVT_LINELENGTH];

  avt_ask_mb (buf, sizeof (buf));
  lua_pushstring (L, buf);
  return 1;
}

/* returns a pressed key (unicode-value) */
static int
lavt_get_key (lua_State * L)
{
  wchar_t ch;

  avt_get_key (&ch);
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
 * the function returns -1 on error or 1 on quit request
 * otherwise it returns the approriete the character
 */
static int
lavt_navigate (lua_State * L)
{
  int r;

  r = avt_navigate (luaL_checkstring (L, 1));

  if (r < 32)
    lua_pushinteger (L, r);
  else
    lua_pushfstring (L, "%c", r);

  return 1;
}

/* make a positive/negative decision */
/* you should also check avt.getstatus() after this */
static int
lavt_decide (lua_State * L)
{
  lua_pushboolean (L, (int) avt_decide ());
  return 1;
}

static int
lavt_choice (lua_State * L)
{
  int result;
  const char *c;

  /* get string in position 3 */
  c = lua_tostring (L, 3);

  avt_choice (&result,
	      luaL_checkint (L, 1),
	      luaL_checkint (L, 2),
	      (c) ? c[0] : 0, lua_toboolean (L, 4), lua_toboolean (L, 5));

  lua_pushinteger (L, result);
  return 1;
}

/* expects one or more strings */
static int
lavt_say (lua_State * L)
{
  int status;
  int n, i;
  const char *s;
  size_t len;

  /* set status to some invalid number */
  status = -42;

  n = lua_gettop (L);

  for (i = 1; i <= n; i++)
    {
      s = lua_tolstring (L, i, &len);
      if (s)
	status = avt_say_mb_len (s, len);
    }

  /* if status wasn't set yet, get it */
  if (status == -42)
    status = avt_get_status ();

  lua_pushinteger (L, status);
  return 1;
}

static int
lavt_pager (lua_State * L)
{
  const char *s;
  size_t len;
  int startline;

  s = luaL_checklstring (L, 1, &len);
  startline = lua_tointeger (L, 2);

  if (s)
    avt_pager_mb (s, len, startline);

  return 0;
}

/*
 * loads an audio file
 * supported: AU and Wave
 */
static int
lavt_load_audio_file (lua_State * L)
{
  lua_pushlightuserdata (L, avt_load_audio_file (luaL_checkstring (L, 1)));
  return 1;
}

static int
lavt_load_audio_string (lua_State * L)
{
  char *data;
  size_t len;

  data = (char *) luaL_checklstring (L, 1, &len);
  lua_pushlightuserdata (L, avt_load_audio_data (data, len));
  return 1;
}

/* frees audio data */
static int
lavt_free_audio (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TLIGHTUSERDATA);
  avt_free_audio (lua_touserdata (L, 1));
  return 0;
}

/* plays audio data */
/* a second parameter set to true plays it in a loop */
static int
lavt_play_audio (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TLIGHTUSERDATA);
  lua_pushinteger (L, avt_play_audio (lua_touserdata (L, 1),
				      lua_toboolean (L, 2)));
  return 1;
}

/*
 * wait until the sound ends
 * this stops a loop, but still plays to the end of the sound
 */
static int
lavt_wait_audio_end (lua_State * L)
{
  lua_pushinteger (L, avt_wait_audio_end ());
  return 1;
}

static int
lavt_stop_audio (lua_State * L)
{
  avt_stop_audio ();
  return 0;
}

/*
 * set a viewport (sub-area of the textarea)
 * 1=x, 2=y, 3=width, 4=height
 * upper left corner is 1, 1
 */

static int
lavt_viewport (lua_State * L)
{
  avt_viewport (luaL_checkinteger (L, 1),
		luaL_checkinteger (L, 2),
		luaL_checkinteger (L, 3), luaL_checkinteger (L, 4));
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
  avt_newline_mode (lua_toboolean (L, 1));
  return 0;
}

static int
lavt_auto_margin (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_auto_margin (lua_toboolean (L, 1));
  return 0;
}

static int
lavt_set_origin_mode (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_set_origin_mode (lua_toboolean (L, 1));
  return 0;
}

static int
lavt_get_origin_mode (lua_State * L)
{
  lua_pushboolean (L, avt_get_origin_mode ());
  return 1;
}

static int
lavt_set_mouse_visible (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_set_mouse_visible (lua_toboolean (L, 1));
  return 0;
}

static int
lavt_lock_updates (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avt_lock_updates (lua_toboolean (L, 1));
  return 0;
}

static int
lavt_insert_spaces (lua_State * L)
{
  avt_insert_spaces (luaL_checkint (L, 1));
  return 0;
}

static int
lavt_delete_characters (lua_State * L)
{
  avt_delete_characters (luaL_checkint (L, 1));
  return 0;
}

static int
lavt_erase_characters (lua_State * L)
{
  avt_erase_characters (luaL_checkint (L, 1));
  return 0;
}

/* --------------------------------------------------------- */
/* avtaddons.h */

static int
lavt_file_selection (lua_State * L)
{
  char filename[256];

  if (avta_file_selection (filename, sizeof (filename), NULL) > -1)
    lua_pushstring (L, filename);
  else
    lua_pushnil (L);

  return 1;
}

/* --------------------------------------------------------- */
/* register library functions */

static const struct luaL_reg akfavtlib[] = {
  {"initialize", lavt_initialize},
  {"initialize_audio", lavt_initialize_audio},
  {"quit", lavt_quit},
  {"button_quit", lavt_button_quit},
  {"default", lavt_default},
  {"import_image_file", lavt_import_image_file},
  {"import_image_string", lavt_import_image_string},
  {"free_image", lavt_free_image},
  {"change_avatar_image", lavt_change_avatar_image},
  {"set_avatar_name", lavt_set_avatar_name},
  {"say", lavt_say},
  {"write", lavt_say},		/* alias */
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
  {"get_status", lavt_get_status},
  {"set_status", lavt_set_status},
  {"get_error", lavt_get_error},
  {"get_color", lavt_get_color},
  {"encoding", lavt_encoding},
  {"set_title", lavt_set_title},
  {"set_text_delay", lavt_set_text_delay},
  {"set_flip_page_delay", lavt_set_flip_page_delay},
  {"right_to_left", lavt_right_to_left},
  {"flip_page", lavt_flip_page},
  {"update", lavt_update},
  {"wait", lavt_wait},
  {"wait_sec", lavt_wait_sec},
  {"set_balloon_size", lavt_set_balloon_size},
  {"set_balloon_width", lavt_set_balloon_width},
  {"set_balloon_height", lavt_set_balloon_height},
  {"set_balloon_color", lavt_set_balloon_color},
  {"set_background_color", lavt_set_background_color},
  {"set_text_color", lavt_set_text_color},
  {"set_text_background_color", lavt_set_text_background_color},
  {"set_text_background_ballooncolor",
   lavt_set_text_background_ballooncolor},
  {"activate_cursor", lavt_activate_cursor},
  {"underlined", lavt_underlined},
  {"get_underlined", lavt_get_underlined},
  {"bold", lavt_bold},
  {"get_bold", lavt_get_bold},
  {"inverse", lavt_inverse},
  {"get_inverse", lavt_get_inverse},
  {"normal_text", lavt_normal_text},
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
  {"load_audio_file", lavt_load_audio_file},
  {"load_audio_string", lavt_load_audio_string},
  {"free_audio", lavt_free_audio},
  {"play_audio", lavt_play_audio},
  {"wait_audio_end", lavt_wait_audio_end},
  {"stop_audio", lavt_stop_audio},
  {"viewport", lavt_viewport},
  {"set_scroll_mode", lavt_set_scroll_mode},
  {"get_scroll_mode", lavt_get_scroll_mode},
  {"newline_mode", lavt_newline_mode},
  {"auto_margin", lavt_auto_margin},
  {"set_origin_mode", lavt_set_origin_mode},
  {"get_origin_mode", lavt_get_origin_mode},
  {"set_mouse_visible", lavt_set_mouse_visible},
  {"lock_updates", lavt_lock_updates},
  {"insert_spaces", lavt_insert_spaces},
  {"delete_characters", lavt_delete_characters},
  {"erase_characters", lavt_erase_characters},
  {"file_selection", lavt_file_selection},
  {NULL, NULL}
};

int
luaopen_akfavatar (lua_State * L)
{
  luaL_register (L, "avt", akfavtlib);
  return 1;
}
