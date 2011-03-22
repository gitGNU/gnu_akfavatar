/*
 * AKFAvatar Terminal emulation for Lua 5.1
 * ATTENTION: this is work in progress, ie. not finished yet
 * Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
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

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <pwd.h>

#ifndef NO_LANGINFO
#  include <langinfo.h>
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static char *startdir = NULL;
static lua_State *term_L;	/* lua_State of the running terminal */

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

/*
 * ececute program in terminal
 * accepts a program and its parameters as separate arguments
 * if no arguments are given, it starts the default shell
 */
static int
lterm_execute (lua_State * L)
{
  int fd;
  int n, i;
  char encoding[80];
  char *argv[256];

  if (!avt_initialized ())
    luaL_error (L, "execute: AKFAvatar not initialized");

#ifdef NO_LANGINFO
  /* get encoding from AKFAvatar settings */
  strncpy (encoding, avt_get_mb_encoding (), sizeof (encoding));
#else
  /* get encoding from system settings */
  /* conforming to SUSv2, POSIX.1-2001 */
  strncpy (encoding, nl_langinfo (CODESET), sizeof (encoding));
#endif

  encoding[sizeof (encoding) - 1] = '\0';	/* enforce termination */

  n = lua_gettop (L);		/* number of options */

  /* number of arguments is already limited in Lua, but I want to be save */
  if (n > 255)
    luaL_error (L, "execute: too many arguments");

  if (n >= 1)			/* start program */
    {
      for (i = 0; i < n; i++)
	argv[i] = (char *) luaL_checkstring (L, i + 1);
      argv[n] = NULL;

      fd = avta_term_start (encoding, startdir, argv);
    }
  else				/* start shell */
    fd = avta_term_start (encoding, startdir, NULL);


  if (fd != -1)
    {
      term_L = L;
      avta_term_run (fd);
      term_L = NULL;
    }

  if (startdir)
    {
      free (startdir);
      startdir = NULL;
    }

  if (avt_get_status () != AVT_NORMAL)
    quit (L);

  return 0;
}

/*
 * set the start directory for the next execute command
 * none or nil as attribute cancels the directory
 */
static int
lterm_startdir (lua_State * L)
{
  if (startdir)
    {
      free (startdir);
      startdir = NULL;
    }

  if (!lua_isnoneornil (L, 1))
    startdir = strdup (luaL_checkstring (L, 1));

  return 0;
}

static int
lterm_homedir (lua_State * L AVT_UNUSED)
{
  char *home;

  home = getenv ("HOME");

  /* when the variable is not set, dig deeper */
  if (home == NULL || *home == '\0')
    {
      struct passwd *user_data;
      user_data = getpwuid (getuid ());	/* POSIX.1-2001 */
      if (user_data != NULL && user_data->pw_dir != NULL
	  && *user_data->pw_dir != '\0')
	home = user_data->pw_dir;
    }

  if (home)
    {
      if (startdir)
	free (startdir);

      startdir = strdup (home);
    }

  return 0;
}

static int
lterm_color (lua_State * L)
{
  luaL_checktype (L, 1, LUA_TBOOLEAN);
  avta_term_nocolor ((avt_bool_t) ! lua_toboolean (L, 1));

  return 0;
}

static int
APC_command (wchar_t * command)
{
  char cmd[1024];		/* just ASCII! */
  unsigned int p;

  if (!term_L)
    return -1;

  /* get ASCII from command */
  p = 0;
  while (*command && p < sizeof (cmd) - 1)
    {
      if (*command <= L'\x7F')	/* ASCII? */
	cmd[p++] = (char) *command;
      command++;
    }

  cmd[p] = '\0';

  if (luaL_loadstring (term_L, cmd) != 0)
    return lua_error (term_L);

  lua_call (term_L, 0, 0);
  avta_term_update_size ();	/* in case the size changed */

  return 0;
}

static const struct luaL_reg termlib[] = {
  {"startdir", lterm_startdir},
  {"homedir", lterm_homedir},
  {"color", lterm_color},
  {"execute", lterm_execute},
  {NULL, NULL}
};

int
luaopen_term (lua_State * L)
{
  term_L = NULL;
  luaL_register (L, "term", termlib);
  avta_term_register_apc (APC_command);
  return 1;
}
