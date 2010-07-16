/*
 * Starter for Lua-AKFAvatar programs in Lua
 * Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
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
#include <libgen.h>		/* for dirname */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* keep it short */
#define PRGNAME "Lua-AKFAvatar"

static lua_State *L;

/* from lua-akfavatar.c */
extern int luaopen_akfavatar (lua_State * L);

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
  puts ("usage: lua-akfavatar [script [args]]");
  exit (EXIT_SUCCESS);
}

/* used with atexit */
static void
quit (void)
{
  avt_quit ();
  lua_close (L);
}

static int
check_options (int argc, char *argv[])
{
  int i;

  i = 1;
  while (i < argc && argv[i][0] == '-')
    {
      if (!strcmp (argv[i], "--version") || !strcmp (argv[i], "-v"))
	version ();
      else if (!strcmp (argv[i], "--help") || !strcmp (argv[i], "-h"))
	help ();
      else
	avta_error ("unknown option", argv[i]);
      i++;
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
  if (avt_initialize ("Lua-AKFAvatar", "AKFAvatar",
		      avt_default (), AVT_AUTOMODE))
    avta_error ("cannot initialize graphics", avt_get_error ());
}

static void
initialize_lua (void)
{
  L = luaL_newstate ();
  if (L == NULL)
    avta_error ("cannot open Lua", "not enough memory");

  luaL_openlibs (L);

  /* register loader function for: require "lua-akfavatar" */
  /* (users should not be able to leave the require command away) */
  lua_getglobal (L, "package");
  lua_getfield (L, -1, "preload");
  lua_pushcfunction (L, luaopen_akfavatar);
  lua_setfield (L, -2, "lua-akfavatar");
  lua_pop (L, 2);
}

static avt_bool_t
is_lua (const char *filename)
{
  if (strcasecmp (filename, "luac.out") == 0)
    return AVT_TRUE;
  else
    {
      const char *ext = strrchr (filename, '.');
      return (avt_bool_t) (ext && strcasecmp (ext, ".lua") == 0);
    }
}

static avt_bool_t
ask_file (void)
{
  char filename[256];

  avt_set_balloon_size (0, 0);
  if (avta_file_selection (filename, sizeof (filename), &is_lua))
    return AVT_FALSE;

  if (*filename)
    {
      /* arg[0] = filename */
      lua_newtable (L);
      lua_pushinteger (L, 0);
      lua_pushstring (L, filename);
      lua_settable (L, -3);
      lua_setglobal (L, "arg");

      if (luaL_dofile (L, filename) != 0)
	{
	  /* on a normal quit-request there is nil on the stack */
	  if (lua_isstring (L, -1))
	    avta_error (lua_tostring (L, -1), NULL);
	  lua_pop (L, 1);	/* pop message (or the nil) */
	}

      return AVT_TRUE;		/* run this again */
    }

  return AVT_FALSE;
}

static void
start_screen (void)
{
  avt_move_in ();
  avt_set_balloon_size (6, 80);
  avt_underlined (AVT_TRUE);
  avt_bold (AVT_TRUE);
  avt_say_mb (PRGNAME);
  avt_normal_text ();
  avt_new_line ();
  avt_new_line ();
  avt_say (L"AKFAvatar ");
  avt_say_mb (avt_version ());
  avt_say (L", ");
  avt_say_mb (avt_copyright ());
  avt_new_line ();
  avt_say_mb (LUA_RELEASE);
  avt_say (L", ");
  avt_say_mb (LUA_COPYRIGHT);
  avt_new_line ();
  avt_new_line ();
  avt_say_mb (avt_license ());
  avt_wait_button ();
}

static void
goto_script_directory (char *p)
{
  char *path, *dir;

  path = strdup (p);
  dir = dirname (path);

  if (strcmp (".", dir) != 0)
    chdir (dir);

  free (path);
}

static void
get_args (int argc, char *argv[], int script_index)
{
  int i;

  /* create global table "arg" and fill it */
  lua_newtable (L);
  for (i = script_index; i < argc; i++)
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

  script_index = check_options (argc, argv);
  avta_prgname (PRGNAME);

  /* initialize Lua */
  initialize_lua ();
  atexit (quit);

  if (script_index)
    {
      goto_script_directory (argv[script_index]);
      get_args (argc, argv, script_index);

      if (luaL_dofile (L, argv[script_index]) != 0)
	{
	  if (lua_isstring (L, -1))
	    avta_error (lua_tostring (L, -1), NULL);
	  return EXIT_SUCCESS;	/* errors are catched by avta_error */
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
	  avt_set_background_color_name ("default");
	  avt_set_balloon_color_name ("floral white");

	  /* script may have called avt.quit() */
	  if (avt_initialized ())
	    {
	      avt_set_title ("Lua-AKFAvatar Starter", "AKFAvatar");
	      avt_change_avatar_image (avt_default ());
	      avt_normal_text ();
	    }
	  else
	    initialize ();

	  /* restart Lua */
	  lua_close (L);
	  initialize_lua ();
	}
    }

  return EXIT_SUCCESS;
}
