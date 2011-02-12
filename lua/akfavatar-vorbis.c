/*
 * AKFAvatar Ogg Vorbis decoder
 * based on stb_vorbis
 * ATTENTION: this is work in progress, ie. not finished yet
 * Copyright (c) 2011 Andreas K. Foerster <info@akfoerster.de>
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

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define STB_VORBIS_HEADER_ONLY 1
#define STB_VORBIS_NO_PUSHDATA_API 1
#include "stb_vorbis.c"

/* internal name for audio data */
#define AUDIODATA   "AKFAvatar-Audio"

static int
lvorbis_load_file (lua_State * L)
{
  char *filename;
  avt_audio_t *audio_data;
  avt_audio_t **audio;

  filename = (char *) luaL_checkstring (L, 1);

  audio_data = avta_load_vorbis_file (filename);

  if (audio_data == NULL)
    {
      lua_pushnil (L);
      return 1;
    }

  /* create audio structure */
  audio = (avt_audio_t **) lua_newuserdata (L, sizeof (avt_audio_t *));
  *audio = audio_data;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);

  return 1;
}

static int
lvorbis_load_string (lua_State * L)
{
  size_t len;
  void *vorbis_data;
  avt_audio_t *audio_data;
  avt_audio_t **audio;

  vorbis_data = (void *) luaL_checklstring (L, 1, &len);

  audio_data = avta_load_vorbis_data (vorbis_data, (int) len);

  if (audio_data == NULL)
    {
      lua_pushnil (L);
      return 1;
    }

  /* create audio structure */
  audio = (avt_audio_t **) lua_newuserdata (L, sizeof (avt_audio_t *));
  *audio = audio_data;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);

  return 1;
}

static const struct luaL_reg vorbislib[] = {
  {"load_file", lvorbis_load_file},
  {"load_string", lvorbis_load_string},
  {NULL, NULL}
};

int
luaopen_vorbis (lua_State * L)
{
  /* require "lua-akfavatar" to get definition for audio data */
  lua_getglobal (L, "require");
  lua_pushliteral (L, "lua-akfavatar");
  lua_call (L, 1, 0);

  luaL_register (L, "vorbis", vorbislib);
  return 1;
}
