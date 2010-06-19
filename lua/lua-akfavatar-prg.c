/*
 * Starter for AKFAvatar programs in Lua
 * Copyright (c) 2009 Andreas K. Foerster <info@akfoerster.de>
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
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

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
  lua_close (L);

  if (avt_initialized ())
    avt_quit ();
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

static void
ask_file (void)
{
  char filename[256];

  if (avt_initialize ("Lua AKFAvatar Starter", "AKFAvatar",
		      avt_default (), AVT_AUTOMODE))
    avta_error ("cannot initialize graphics", avt_get_error ());

  avta_file_selection (filename, sizeof (filename), &is_lua);

  if (*filename)
    {
      if (luaL_dofile (L, filename) != 0)
	avta_error (lua_tostring (L, -1), NULL);
    }
}

static void
get_args (int argc, char *argv[], int script_index)
{
  int i;
  char buf[1024];

  for (i = script_index + 1; i < argc; i++)
    {
      snprintf (buf, sizeof (buf), "table.insert(arg, '%s')", argv[i]);
      if (luaL_dostring (L, buf) != 0)
	exit (EXIT_FAILURE);
    }
}

int
main (int argc, char **argv)
{
  int script_index;

  avta_prgname (PRGNAME);
  script_index = check_options (argc, argv);

  /* initialize Lua */
  L = lua_open ();
  if (L == NULL)
    avta_error (argv[0], "cannot open Lua: not enough memory");

  luaL_openlibs (L);
  luaopen_akfavatar (L);

  atexit (quit);

  /* mark 'lua-akfavatar' as loaded and initialize arg */
  if (luaL_dostring (L, "package.loaded['lua-akfavatar']=true; arg={}") != 0)
    return EXIT_FAILURE;

  if (script_index)
    {
      if (script_index + 1 < argc)
	get_args (argc, argv, script_index);

      if (luaL_dofile (L, argv[script_index]) != 0)
	avta_error (lua_tostring (L, -1), NULL);
    }
  else
    ask_file ();

  return EXIT_SUCCESS;
}
