/*
 * Starter for Lua-AKFAvatar programs in Lua
 * Copyright (c) 2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99 or C++, POSIX.1-2001
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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>		/* getcwd, chdir */
#include <locale.h>
#include <errno.h>

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

#define PRGNAME "Lua-AKFAvatar"	/* keep it short */
#define NAME_EXEC "AKFAvatar.lua"	/* name in archive file */

#define EXT_LUA   ".lua"
#define EXT_DEMO  ".avt"
#define EXT_EXEC  ".avtexe"
#define EXT_ABOUT ".about"

static lua_State *L;


static void
version (void)
{
  printf (PRGNAME "\n\nAKFAvatar %s, %s\n%s, %s\n\n%s\n\n", avt_version (),
	  avt_copyright (), LUA_RELEASE, LUA_COPYRIGHT, avt_license ());
  exit (EXIT_SUCCESS);
}

static void
help (void)
{
  puts (PRGNAME);
  puts ("Usage: lua-akfavatar [script [args]]");
  puts (" or:   lua-akfavatar --dir=/usr/local/share/akfavatar/lua\n");
  puts (" --help                    show this help");
  puts (" --version                 show version");
  puts (" -l name                   require library 'name'");
  puts
    (" --dir=<directory>         start in directory (for the filechooser)");
  exit (EXIT_SUCCESS);
}

/* used with atexit */
static void
quit (void)
{
  avt_quit ();
  lua_close (L);
}


static void
error_box (const char *msg)
{
  avt_set_status (AVT_NORMAL);
  avt_change_avatar_image (NULL);
  avt_set_balloon_color (0xFF, 0xAA, 0xAA);
  avt_normal_text ();
  avt_set_auto_margin (true);
  avt_set_scroll_mode (-1);
  avt_set_text_delay (0);
  avt_lock_updates (false);
  avt_bell ();
  avt_tell_mb (msg);
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
#ifdef _WIN32
    MessageBox (NULL, msg, PRGNAME, MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
#else
    fputs (msg, stderr);
#endif

  exit (EXIT_FAILURE);
}


/* require module and pushes nresults on the stack */
static void
require (const char *module, int nresults)
{
  lua_getglobal (L, "require");
  lua_pushstring (L, module);
  if (lua_pcall (L, 1, nresults, 0) != 0)
    {
      fatal ("require", lua_tostring (L, -1));
      lua_pop (L, 1);		/* pop message */
    }
}

static void
handle_require_options (int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc && argv[i][0] == '-'; i++)
    {
      if (strncmp (argv[i], "-l", 2) == 0)
	{
	  if (argv[i][2] != '\0')
	    require (argv[i] + 2, 0);
	  else
	    require (argv[++i], 0);
	}
    }
}

static int
check_options (int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc && argv[i][0] == '-'; i++)
    {
      if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
	version ();
      else if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
	help ();
      else if (strncmp (argv[i], "--dir=", 6) == 0)
	chdir (argv[i] + 6);
      else if (strncmp (argv[i], "-l", 2) == 0)
	{
	  /* ignore -l for now */
	  if (argv[i][2] == '\0')
	    i++;
	}
      else
	fatal ("unknown option", argv[i]);
    }

  /* no script found? */
  if (i >= argc)
    i = 0;

  /* return script-index */
  return i;
}

static void
initialize (void)
{
  avt_mb_encoding ("UTF-8");
  if (avt_start ("Lua-AKFAvatar", "AKFAvatar", AVT_AUTOMODE) 
      || avt_avatar_image_default ())
    fatal ("cannot initialize graphics", avt_get_error ());
}

static void
reset (void)
{
  avt_clear_screen ();
  avt_newline_mode (true);
  avt_set_auto_margin (true);
  avt_set_origin_mode (true);
  avt_set_scroll_mode (1);
  avt_reserve_single_keys (false);
  avt_set_background_color_name ("default");
  avt_set_balloon_color_name ("floral white");
  avt_markup (false);
  avt_text_direction (AVT_LEFT_TO_RIGHT);
  avt_normal_text ();
  avt_quit_audio ();
  avt_set_title ("Lua-AKFAvatar Starter", "AKFAvatar");
  avt_avatar_image_default ();
  avt_set_mouse_visible (true);
}

/* check if this program can handle the file */
static bool
check_filename (const char *filename)
{
  const char *ext = strrchr (filename, '.');

  return (ext && (strcasecmp (EXT_LUA, ext) == 0
		  || strcasecmp (EXT_DEMO, ext) == 0
		  || strcasecmp (EXT_EXEC, ext) == 0
		  || strcasecmp (EXT_ABOUT, ext) == 0));
}

static void
initialize_lua (void)
{
  /* from lua-avt.c */
  extern int luaopen_akfavatar_embedded (lua_State * L);

  L = luaL_newstate ();
  if (L == NULL)
    fatal ("cannot open Lua", "not enough memory");

  luaL_checkversion (L);
  lua_gc (L, LUA_GCSTOP, 0);
  luaL_openlibs (L);
  lua_gc (L, LUA_GCRESTART, 0);

  /* register loader functions for: "lua-akfavatar" */
  /* (users should not be able to leave the require command away) */
  lua_getglobal (L, "package");
  lua_getfield (L, -1, "preload");
  lua_pushcfunction (L, luaopen_akfavatar_embedded);
  lua_setfield (L, -2, "lua-akfavatar");
  lua_pop (L, 2);
}

static void
arg0 (const char *filename)
{
  /* arg[0] = filename */
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
      /* on a normal quit-request there is nil on the stack */
      if (lua_isstring (L, -1))
	fatal ("akfavatar.avtdemo", lua_tostring (L, -1));
      lua_pop (L, 1);		/* pop message (or the nil) */
    }
}

/* returns 0 on success, or -1 on error with message on stack */
static int
run_executable (const char *filename)
{
  int status;
  size_t size;
  char *script, *start;

  script = avta_arch_get_data (filename, NAME_EXEC, &size);

  if (script == NULL)
    {
      lua_pushfstring (L, "%s: error in executable", filename);
      return -1;
    }

  start = script;

  /* skip UTF-8 BOM */
  if (start[0] == '\xEF' && start[1] == '\xBB' && start[2] == '\xBF')
    {
      start += 3;
      size -= 3;
    }

  /* skip #! line */
  if (start[0] == '#' && start[1] == '!')
    while (*start != '\n')
      {
	start++;
	size--;
      }

  status = luaL_loadbufferx (L, start, size, filename, "t");
  free (script);

  arg0 (filename);

  if (status != 0 || lua_pcall (L, 0, 0, 0) != 0)
    {
      /* on a normal quit-request there is nil on the stack */
      if (lua_isstring (L, -1))
	return -1;		/* message already on stack */
      else
	lua_pop (L, 1);		/* remove nil */
    }

  return 0;
}

static void
show_text (const char *filename)
{
  avt_change_avatar_image (NULL);	/* no avatar */
  avt_set_balloon_size (0, 0);
  avt_set_balloon_color_name ("tan");
  /* text file must be UTF-8 encoded (or plain ASCII) */
  avta_pager_file (filename, 1);
}

static bool
ask_file (void)
{
  char filename[256];
  char lua_dir[4096 + 1];
  const char *ext;

  avt_set_balloon_size (0, 0);
  if (avta_file_selection (filename, sizeof (filename), &check_filename))
    return false;

  if (*filename)
    {
      if (!getcwd (lua_dir, sizeof (lua_dir)))
	lua_dir[0] = '\0';

      ext = strrchr (filename, '.');

      if (!ext)
	return false;		/* shouldn't happen */
      else if (strcasecmp (EXT_DEMO, ext) == 0)
	avtdemo (filename);
      else if (strcasecmp (EXT_EXEC, ext) == 0)
	{
	  if (run_executable (filename) != 0)
	    {
	      error_box (lua_tostring (L, -1));
	      lua_pop (L, 1);
	    }
	}
      else if (strcasecmp (EXT_ABOUT, ext) == 0)
	show_text (filename);
      else			/* assume Lua code */
	{
	  arg0 (filename);
	  if (luaL_loadfilex (L, filename, "t") != 0
	      || lua_pcall (L, 0, 0, 0) != 0)
	    {
	      /* on a normal quit-request there is nil on the stack */
	      if (lua_isstring (L, -1))
		error_box (lua_tostring (L, -1));
	      lua_pop (L, 1);	/* pop message (or the nil) */
	    }
	}

      /* go back to the lua directory if it was changedby the script */
      if (lua_dir[0] != '\0')
	chdir (lua_dir);

      return true;		/* run this again */
    }

  return false;
}

static void
start_screen (void)
{
  if (avt_move_in () != AVT_NORMAL)
    exit (EXIT_SUCCESS);
  avt_set_balloon_size (9, 80);
  avt_underlined (true);
  avt_bold (true);
  avt_say_mb (PRGNAME);
  avt_normal_text ();
  avt_new_line ();
  avt_new_line ();
  avt_say (L"AKFAvatar ");
  avt_say_mb (avt_version ());
  avt_say (L", ");
  avt_say_mb (avt_copyright ());
  avt_new_line ();
  avt_say_mb (LUA_COPYRIGHT);
  avt_new_line ();
  avt_new_line ();
  avt_say_mb ("Homepage: ");
  avt_underlined (true);
  avt_say_mb ("http://akfavatar.nongnu.org/");
  avt_underlined (false);
  avt_new_line ();
  avt_say_mb (avt_license ());
  avt_new_line ();
  avt_new_line ();
  avt_bold (true);
  avt_say_mb ("F11");
  avt_bold (false);
  avt_say_mb (": Fullscreen, ");
  avt_bold (true);
  avt_say_mb ("Esc");
  avt_bold (false);
  avt_say_mb (": end/back");

  if (avt_wait_button () != AVT_NORMAL)
    exit (EXIT_SUCCESS);
}

static void
get_args (int argc, char *argv[], int script_index)
{
  int i;

  /* create global table "arg" and fill it */
  lua_newtable (L);

  /* script name as arg[0] */
  lua_pushinteger (L, 0);
  lua_pushstring (L, argv[script_index]);
  lua_settable (L, -3);

  /* arg[1] ... */
  for (i = script_index + 1; i < argc; i++)
    {
      lua_pushinteger (L, i - script_index);
      lua_pushstring (L, argv[i]);
      lua_settable (L, -3);
    }

  lua_setglobal (L, "arg");
}

int
main (int argc, char **argv)
{
  int script_index;

  setlocale (LC_ALL, "");

  script_index = check_options (argc, argv);

  /* initialize Lua */
  initialize_lua ();
  atexit (quit);
  handle_require_options (argc, argv);

  if (script_index)
    {
      const char *ext;

      ext = strrchr (argv[script_index], '.');
      if (ext && strcasecmp (EXT_DEMO, ext) == 0)
	avtdemo (argv[script_index]);
      else if (ext && strcasecmp (EXT_EXEC, ext) == 0)
	{
	  if (run_executable (argv[script_index]) != 0)
	    fatal ("executable", lua_tostring (L, -1));
	}
      else			/* assume Lua code */
	{
	  get_args (argc, argv, script_index);

	  if (luaL_loadfilex (L, argv[script_index], "t") != 0
	      || lua_pcall (L, 0, 0, 0) != 0)
	    {
	      if (lua_isstring (L, -1))
		fatal (argv[script_index], lua_tostring (L, -1));
	    }
	}
    }
  else				/* no script at command-line */
    {
      initialize ();
      start_screen ();
      while (ask_file ())
	{
	  /* reset settings */
	  avt_set_status (AVT_NORMAL);

	  /* restart Lua */
	  lua_close (L);
	  initialize_lua ();

	  /* script may have called avt.quit() */
	  /* also closing Lua may have accidently closed AKFAvatar */
	  if (avt_initialized ())
	    reset ();
	  else
	    initialize ();

	  handle_require_options (argc, argv);
	}
    }

  return EXIT_SUCCESS;
}
