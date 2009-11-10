/*
 * Lua 5.1 for AKFAvatar (application)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "akfavatar.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* from lua-avt.c */
extern int luaopen_avt (lua_State * L);


void
avatar_init (int argc, char *argv[])
{
  int mode = AVT_AUTOMODE;
  int i;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "--fullscreen") || !strcmp (argv[i], "-f"))
	mode = AVT_FULLSCREEN;
      if (!strcmp (argv[i], "--window") || !strcmp (argv[i], "-w"))
	mode = AVT_WINDOW;
    }

  if (avt_initialize ("AKFAvatar-Lua", "AKFAvatar", avt_default (), mode))
    {
      fprintf (stderr, "cannot initialize graphics: %s\n", avt_get_error ());
      exit (1);
    }

  atexit (avt_quit);

  avt_mb_encoding ("UTF-8");
}

static int
lua_print (lua_State * L)
{
  int n, i;
  const char *s;
  size_t len;

  n = lua_gettop (L);

  for (i = 1; i <= n; i++)
    {
      if (i > 1)
	avt_next_tab ();
      if (lua_isstring (L, i))
	{
	  s = lua_tolstring (L, i, &len);
	  avt_say_mb_len (s, len);
	}
      else if (lua_isnil (L, i))
	avt_say (L"nil");
      else if (lua_isboolean (L, i))
	avt_say (lua_toboolean (L, i) ? L"true" : L"false");
      else
	avt_say_mb (luaL_typename (L, i));
    }

  avt_new_line ();

  return 0;
}

static void
prompt (lua_State * L)
{
  char buf[256];
  int error;
  int status;

  do
    {
      avt_say (L"> ");
      status = avt_ask_mb (buf, sizeof (buf));
      error = luaL_loadbuffer (L, buf, strlen (buf), "line");
      if (error == 0)
	lua_pcall (L, 0, 0, 0);
      if (error)
	{
	  avt_say_mb (lua_tostring (L, -1));
	  avt_new_line ();
	  lua_pop (L, 1);	/* pop error message from the stack */
	}
    }
  while (status == AVT_NORMAL);
}

int
main (int argc, char **argv)
{
  lua_State *L;

  /* initialize AKFAvatar */
  avatar_init (argc, argv);

  /* initialize Lua */
  L = lua_open ();
  luaL_openlibs (L);
  luaopen_avt (L);

  if (luaL_dostring (L, "package.loaded['lua-avt']=package.loaded['avt']"))
    {
      lua_close (L);
      return 1;
    }

  lua_register (L, "print", lua_print);

  if (argc == 2)
    (void) luaL_dofile (L, argv[1]);
  else
    prompt (L);

  /* cleanup Lua */
  lua_close (L);
  return 0;
}
