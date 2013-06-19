/*
 * Starter for Lua-AKFAvatar programs in Lua
 * Copyright (c) 2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
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

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

#include "akfavatar.h"
#include "avtaddons.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>		// getcwd, chdir
#include <locale.h>
#include <errno.h>
#include <iso646.h>

/* SDL redefines main on some systems */
#if defined(_WIN32) || defined(__APPLE__) || defined(macintosh)
#  include "SDL.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
}
#endif

#include "data/akfavatar-logo.xpm"


#define PRGNAME "Lua-AKFAvatar"	// keep it short
#define NAME_EXEC "AKFAvatar.lua"	// name in archive file

#define EXT_LUA   ".lua"
#define EXT_DEMO  ".avt"
#define EXT_EXEC  ".avtexe"
#define EXT_ABOUT ".about"

#define AVT_COLOR_BACKGROUND  0xE0D5C5	// "default"
#define AVT_COLOR_ERROR       0xFFAAAA
#define AVT_COLOR_SAY         0xFFFAF0	// "floral white"
#define AVT_COLOR_TEXT        0xD2B48C	// "tan"
#define AVT_COLOR_START       AVT_COLOR_TEXT
#define AVT_COLOR_SELECTOR    AVT_COLOR_TEXT

// from lua-avt.c
extern int open_lua_akfavatar (lua_State * L);

static lua_State *L;
static int mode = AVT_AUTOMODE;
static char *directory;


static void
version (void)
{
  printf (PRGNAME "\n\nAKFAvatar %s, %s\n%s, %s\n\nLicense %s\n\n",
	  avt_version (), avt_copyright (), LUA_RELEASE, LUA_COPYRIGHT,
	  avt_license ());
  exit (EXIT_SUCCESS);
}

static void
help (void)
{
  puts (PRGNAME);
  puts ("Usage: lua-akfavatar [script [args]]");
  puts (" or:   lua-akfavatar --dir=/usr/local/share/akfavatar/lua\n");
  puts (" -h, --help                show this help");
  puts (" -v, --version             show version");
  puts (" -f, --fullscreen          fullscreen mode (unless script given)");
  puts
    (" -F, --Fullscreen          full fullscreen mode (unless script given)");
  puts (" -l [var=]name             require library 'name'");
  puts
    (" --dir=<directory>         start in directory (for the filechooser)");
  exit (EXIT_SUCCESS);
}

// used with atexit
static void
quit (void)
{
  avt_quit ();
  lua_close (L);
}

// error message might include localized system strings
static void
error_msg (const char *msg)
{
  size_t len = strlen (msg) + 1;
  wchar_t message[len];

  size_t wlen = mbstowcs (message, msg, len);
  avt_tell_len (message, wlen);
}

static void
error_box (const char *msg)
{
  avt_set_status (AVT_NORMAL);
  avt_avatar_image_none ();
  avt_set_balloon_color (AVT_COLOR_ERROR);
  avt_normal_text ();
  avt_set_auto_margin (true);
  avt_set_scroll_mode (-1);
  avt_set_text_delay (0);
  avt_lock_updates (false);
  avt_bell ();
  error_msg (msg);
  avt_wait_button ();
}


static void
fatal (const char *m1, const char *m2)
{
  char msg[4096];

  if (m2)
    snprintf (msg, sizeof (msg), PRGNAME ": %s: %s", m1, m2);
  else
    snprintf (msg, sizeof (msg), PRGNAME ": %s", m1);

  if (avt_initialized ())
    error_box (msg);
  else
    {
#ifdef _WIN32
      // no standard
      MessageBox (NULL, msg, PRGNAME,
		  MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
#else
      fputs (msg, stderr);
      fputc ('\n', stderr);
#endif
    }

  exit (EXIT_FAILURE);
}


// require module and pushes nresults on the stack
static void
require (const char *module, int nresults)
{
  lua_getglobal (L, "require");
  lua_pushstring (L, module);
  if (lua_pcall (L, 1, nresults, 0) != 0)
    {
      fatal ("require", lua_tostring (L, -1));
      lua_pop (L, 1);		// pop message
    }
}

static void
handle_require_options (int argc, char *argv[])
{
  int i;
  char *name, *p;
  char variable[256];

  name = NULL;
  variable[0] = '\0';

  for (i = 1; i < argc and argv[i][0] == '-'; i++)
    {
      if (strncmp (argv[i], "-l", 2) == 0)
	{
	  if (argv[i][2] != '\0')
	    name = argv[i] + 2;
	  else
	    name = argv[++i];

	  if (not name)
	    fatal ("'-l' needs argument", NULL);

	  strncpy (variable, name, sizeof (variable));
	  variable[sizeof (variable) - 1] = '\0';

	  p = strchr (name, '=');
	  if (p)
	    name = p + 1;

	  p = strchr (variable, '=');
	  if (p)
	    *p = '\0';

	  p = strrchr (variable, '.');
	  if (p)
	    {
	      ++p;
	      memmove (variable, p, strlen (p) + 1);
	    }

	  require (name, 1);
	  lua_setglobal (L, variable);
	}
    }
}

static int
check_options (int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc and argv[i][0] == '-'; i++)
    {
      if (strcmp (argv[i], "--version") == 0 or strcmp (argv[i], "-v") == 0)
	version ();
      else if (strcmp (argv[i], "--help") == 0 or strcmp (argv[i], "-h") == 0)
	help ();
      else if (strcmp (argv[i], "--fullscreen") == 0
	       or strcmp (argv[i], "-f") == 0)
	mode = AVT_FULLSCREEN;
      else if (strcmp (argv[i], "--Fullscreen") == 0
	       or strcmp (argv[i], "-F") == 0)
	mode = AVT_FULLSCREENNOSWITCH;
      else if (strncmp (argv[i], "--dir=", 6) == 0)
	directory = (argv[i] + 6);
      else if (strncmp (argv[i], "-l", 2) == 0)
	{
	  // ignore -l for now
	  if (argv[i][2] == '\0')
	    i++;
	}
      else
	fatal ("unknown option", argv[i]);
    }

  // no script found?
  if (i >= argc)
    i = 0;

  // return script-index
  return i;
}

static void
initialize (void)
{
  if (avt_start ("Lua-AKFAvatar", "AKFAvatar", mode))
    fatal ("cannot initialize graphics", avt_get_error ());
}

static void
reset (void)
{
  avt_reset ();
  avt_set_background_color (AVT_COLOR_BACKGROUND);
  avt_set_balloon_color (AVT_COLOR_SAY);
  avt_text_direction (AVT_LEFT_TO_RIGHT);
  avt_quit_audio ();
  avt_set_title ("Lua-AKFAvatar", "AKFAvatar");
}

static bool
check_textfile (const char *filename, const char *ext)
{
  return strncasecmp (filename, "README", 6) == 0
    or strncasecmp (filename, "COPYING", 7) == 0
    or strcasecmp (filename, "AUTHORS") == 0
    or strcasecmp (filename, "NEWS") == 0
    or strcasecmp (filename, "ChangeLog") == 0
    or (ext and
	(strcasecmp (ext, EXT_ABOUT) == 0
	 or strcasecmp (ext, ".txt") == 0 or strcasecmp (ext, ".text") == 0));
}

// check if this program can handle the file
static bool
check_filename (const char *filename, void *data)
{
  (void) data;

  const char *ext = strrchr (filename, '.');

  return check_textfile (filename, ext)
    or (ext and (strcasecmp (EXT_LUA, ext) == 0
		 or strcasecmp (EXT_DEMO, ext) == 0
		 or strcasecmp (EXT_EXEC, ext) == 0));
}


#if defined(__linux__)

 /*
  * If this program is called from an unusual directory
  * set up special search paths for modules in
  * the subdirectory "lua".
  *
  * Lua itself does that already for Windows.
  * Here I do it for GNU/Linux.
  * I don't know how to do it on other systems.
  */

static void
change_searchpaths (void)
{
  char basedir[4097];

  avt_base_directory (basedir, sizeof (basedir));

  // if basedir is nonstandard, add to Lua searchpaths
  if (*basedir and strcmp ("/usr", basedir) != 0
      and strcmp ("/usr/local", basedir) != 0)
    {
      lua_getglobal (L, "package");

      // set package.path
      lua_pushfstring (L, "%s/lua/?.lua;", basedir);
      lua_getfield (L, -2, "path");
      lua_concat (L, 2);
      lua_setfield (L, -2, "path");

      // set package.cpath
      lua_pushfstring (L, "%s/?.so;%s/lua/?.so;", basedir, basedir);
      lua_getfield (L, -2, "cpath");
      lua_concat (L, 2);
      lua_setfield (L, -2, "cpath");

      lua_pop (L, 1);		// pop "package"
    }
}

#else // not __linux__
#define change_searchpaths(void)
#endif // not __linux__

static void
initialize_lua (void)
{
  L = luaL_newstate ();
  if (not L)
    fatal ("cannot open Lua", "not enough memory");

  luaL_checkversion (L);
  lua_gc (L, LUA_GCSTOP, 0);
  luaL_openlibs (L);
  lua_gc (L, LUA_GCRESTART, 0);

  // load lua-akfavatar
  luaL_requiref (L, "lua-akfavatar", open_lua_akfavatar, false);
  lua_pop (L, 1);

  change_searchpaths ();
}

static void
arg0 (const char *filename)
{
  // arg[0] = filename
  lua_newtable (L);
  lua_pushinteger (L, 0);
  lua_pushstring (L, filename);
  lua_settable (L, -3);
  lua_setglobal (L, "arg");
}

static void
avtdemo (const char *filename)
{
  require ("akfavatar.avtdemo", 1);
  lua_pushstring (L, filename);
  if (lua_pcall (L, 1, 0, 0) != 0)
    {
      // on a normal quit-request there is nil on the stack
      if (lua_isstring (L, -1))
	fatal ("akfavatar.avtdemo", lua_tostring (L, -1));
      lua_pop (L, 1);		// pop message (or the nil)
    }
}

// returns 0 on success, or -1 on error with message on stack
static int
run_executable (const char *filename)
{
  int status;
  size_t size;
  char *script, *start;

  script = avt_arch_get_data (filename, NAME_EXEC, &size);

  if (not script)
    {
      lua_pushfstring (L, "%s: error in executable", filename);
      return -1;
    }

  start = script;

  // skip UTF-8 BOM
  if (start[0] == '\xEF' and start[1] == '\xBB' and start[2] == '\xBF')
    {
      start += 3;
      size -= 3;
    }

  // skip #! line
  if (start[0] == '#' and start[1] == '!')
    while (*start != '\n')
      {
	start++;
	size--;
      }

  status = luaL_loadbufferx (L, start, size, filename, "t");
  free (script);

  arg0 (filename);

  if (status != 0 or lua_pcall (L, 0, 0, 0) != 0)
    {
      // on a normal quit-request there is nil on the stack
      if (lua_isstring (L, -1))
	return -1;		// message already on stack
      else
	lua_pop (L, 1);		// remove nil
    }

  return 0;
}

static void
show_text (const char *filename)
{
  avt_avatar_image_none ();
  avt_set_balloon_size (0, 0);
  avt_set_balloon_color (AVT_COLOR_TEXT);

  // text file must be UTF-8 encoded (or plain ASCII)
  const struct avt_charenc *old;
  old = avt_char_encoding (avt_utf8 ());
  avt_pager_file (filename, 1);
  avt_char_encoding (old);
}

static bool
ask_file (void)
{
  char filename[256];
  char lua_dir[4096 + 1];
  const char *ext;

  avt_clear_screen ();
  avt_set_balloon_color (AVT_COLOR_SELECTOR);
  avt_avatar_image_xpm (akfavatar_logo_xpm);
  avt_set_avatar_mode (AVT_HEADER);
  avt_set_balloon_size (0, 0);

  if (avt_file_selection
      (filename, sizeof (filename), &check_filename, NULL))
    return false;

  if (*filename)
    {
      avt_clear_screen ();
      avt_avatar_image_none ();
      avt_set_avatar_mode (AVT_SAY);

      if (not getcwd (lua_dir, sizeof (lua_dir)))
	lua_dir[0] = '\0';

      ext = strrchr (filename, '.');

      if (ext and strcasecmp (EXT_DEMO, ext) == 0)
	avtdemo (filename);
      else if (ext and strcasecmp (EXT_EXEC, ext) == 0)
	{
	  if (run_executable (filename) != 0)
	    {
	      error_box (lua_tostring (L, -1));
	      lua_pop (L, 1);
	    }
	}
      else if (check_textfile (filename, ext))
	show_text (filename);
      else			// assume Lua code
	{
	  arg0 (filename);
	  if (luaL_loadfilex (L, filename, "t") != 0
	      or lua_pcall (L, 0, 0, 0) != 0)
	    {
	      // on a normal quit-request there is nil on the stack
	      if (lua_isstring (L, -1))
		error_box (lua_tostring (L, -1));
	      lua_pop (L, 1);	// pop message (or the nil)
	    }
	}

      // go back to the lua directory if it was changed by the script
      if (*lua_dir)
	if (chdir (lua_dir) < 0)
	  {
	    // ignore error
	  }

      // script may have called avt.quit()
      if (avt_initialized ())
	reset ();
      else
	initialize ();

      return true;		// run this again
    }

  return false;
}

static void
start_screen (void)
{
  const char *language;
  bool german;

  language = avt_get_language ();
  german = (language and strcmp ("de", language) == 0);

  avt_lock_updates (true);
  avt_clear_screen ();
  avt_set_balloon_color (AVT_COLOR_START);
  avt_avatar_image_xpm (akfavatar_logo_xpm);
  avt_set_avatar_mode (AVT_HEADER);
  avt_set_balloon_size (10, 80);
  avt_underlined (true);
  avt_bold (true);
  avt_say (L"" PRGNAME);
  avt_normal_text ();
  avt_new_line ();
  avt_new_line ();
  avt_say (L"AKFAvatar ");
  avt_say (avt_wide_version ());
  avt_say (L", ");
  avt_say (avt_wide_copyright ());
  avt_new_line ();
  avt_say (L"" LUA_COPYRIGHT);
  avt_new_line ();
  avt_new_line ();
  if (german)
    avt_say (L"Verf\u00FCgbar f\u00FCr GNU\u2665Linux und Windows.");
  else
    avt_say (L"Available for GNU\u2665Linux and Windows.");
  avt_new_line ();
  avt_say (L"Homepage: ");
  avt_underlined (true);
  if (german)
    avt_say (L"http://akfavatar.nongnu.org/akfavatar.de.html");
  else
    avt_say (L"http://akfavatar.nongnu.org/");
  avt_underlined (false);
  avt_new_line ();
  if (german)
    avt_say (L"Lizenz: ");
  else
    avt_say (L"License: ");
  avt_say (avt_wide_license ());
  avt_new_line ();
  avt_new_line ();
  avt_bold (true);
  avt_say (L"F11");
  avt_bold (false);
  if (german)
    avt_say (L": Vollbild, ");
  else
    avt_say (L": Fullscreen, ");
  avt_bold (true);
  avt_say (L"Esc");
  avt_bold (false);
  if (german)
    avt_say (L": Ende/zur\u00FCck");
  else
    avt_say (L": end/back");

  avt_lock_updates (false);

  if (avt_wait_button () != AVT_NORMAL)
    exit (EXIT_SUCCESS);
}

static void
get_args (int argc, char *argv[], int script_index)
{
  int i;

  // create global table "arg" and fill it
  lua_newtable (L);

  // script name as arg[0]
  lua_pushinteger (L, 0);
  lua_pushstring (L, argv[script_index]);
  lua_settable (L, -3);

  // arg[1] ...
  for (i = script_index + 1; i < argc; i++)
    {
      lua_pushinteger (L, i - script_index);
      lua_pushstring (L, argv[i]);
      lua_settable (L, -3);
    }

  lua_setglobal (L, "arg");
}

static int
local_lua_dir (void)
{
  char basedir[4097];

  if (avt_base_directory (basedir, sizeof (basedir)) == -1)
    return -1;

  strcat (basedir, LUA_DIRSEP);
  strcat (basedir, "lua");

  return chdir (basedir);
}

static int
find_scripts (void)
{
  if (directory)
    return chdir (directory);	// don't try any other!
  else if (local_lua_dir () < 0
	   and chdir ("/usr/local/share/akfavatar/lua") < 0)
    return chdir ("/usr/share/akfavatar/lua");

  return 0;
}


int
main (int argc, char **argv)
{
  int script_index;

  setlocale (LC_ALL, "");

  script_index = check_options (argc, argv);

  // initialize Lua
  initialize_lua ();
  atexit (quit);
  handle_require_options (argc, argv);

  if (script_index)
    {
      const char *ext;

      if (mode != AVT_AUTOMODE)
	initialize ();

      ext = strrchr (argv[script_index], '.');
      if (ext and strcasecmp (EXT_DEMO, ext) == 0)
	avtdemo (argv[script_index]);
      else if (ext and strcasecmp (EXT_EXEC, ext) == 0)
	{
	  if (run_executable (argv[script_index]) != 0)
	    fatal ("executable", lua_tostring (L, -1));
	}
      else			// assume Lua code
	{
	  get_args (argc, argv, script_index);

	  if (luaL_loadfilex (L, argv[script_index], "t") != 0
	      or lua_pcall (L, 0, 0, 0) != 0)
	    {
	      if (lua_isstring (L, -1))
		fatal (argv[script_index], lua_tostring (L, -1));
	    }
	}
    }
  else				// no script at command-line
    {
      initialize ();
      start_screen ();
      find_scripts ();

      while (ask_file ())
	{
	  // reset settings
	  avt_set_status (AVT_NORMAL);

	  // restart Lua
	  lua_close (L);
	  initialize_lua ();

	  handle_require_options (argc, argv);
	}
    }

  return EXIT_SUCCESS;
}
