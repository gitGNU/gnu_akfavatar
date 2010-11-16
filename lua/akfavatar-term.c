/*
 * AKFAvatar Terminal emulation for Lua 5.1
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

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

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
 * 1) encoding is the system encoding to be used
 * 2) working directory (may be nil)
 */
static int
lterm_run (lua_State * L)
{
  int fd;
  const char *encoding = lua_tostring (L, 1);
  const char *working_dir = lua_tostring (L, 2);	/* NULL is okay */

  if (!encoding || !*encoding)
    encoding = strdup(avt_get_mb_encoding());

  fprintf (stderr, "encoding: %s\n", encoding);
  fd = avta_term_start (encoding, working_dir, NULL);
  if (fd != -1)
    avta_term_run (fd);

  if (avt_get_status () != AVT_NORMAL)
    quit (L);

  return 0;
}

static const struct luaL_reg termlib[] = {
  {"run", lterm_run},
  {NULL, NULL}
};

int
luaopen_term (lua_State * L)
{
  luaL_register (L, "term", termlib);
  return 1;
}
