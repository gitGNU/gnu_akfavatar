/*
 * AKFAvatar Ogg Vorbis decoder
 * based on stb_vorbis
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


#ifdef __cplusplus
extern "C"
{
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
  extern int luaopen_vorbis (lua_State * L);
}
#endif


#define STB_VORBIS_HEADER_ONLY 1
#define STB_VORBIS_NO_PUSHDATA_API 1

#include "stb_vorbis.c"

/* internal name for audio data */
#define AUDIODATA   "AKFAvatar-Audio"

static void
collect_garbage (lua_State * L)
{
  if (!avt_audio_playing (NULL))
    {
      lua_pushnil (L);
      lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-current-sound");
    }

  lua_gc (L, LUA_GCCOLLECT, 0);
}

static int
lvorbis_load_file (lua_State * L)
{
  char *filename;
  avt_audio_t *audio_data;
  avt_audio_t **audio;

  collect_garbage (L);
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

  collect_garbage (L);
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

static int
lvorbis_load_file_chain (lua_State * L)
{
  avt_audio_t *audio_data;
  avt_audio_t **audio;

  audio_data = NULL;

  if (!lua_isnoneornil (L, 1))
    {
      char *filename;
      filename = (char *) luaL_checkstring (L, 1);

      audio_data = avta_load_vorbis_file (filename);

      if (!audio_data)
	{
	  lua_getfield (L, LUA_REGISTRYINDEX,
			"AVTVORBIS-old_load_audio_file");
	  lua_pushvalue (L, 1);	/* push filename */

	  if (lua_pcall (L, 1, 1, 0) != 0)
	    {
	      lua_pushnil (L);	/* return nil on error */
	      lua_pushliteral (L, "unsupported audio format");
	      return 2;
	    }

	  return 1;
	}
    }

  collect_garbage (L);

  /* create audio structure */
  audio = (avt_audio_t **) lua_newuserdata (L, sizeof (avt_audio_t *));
  *audio = audio_data;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);

  return 1;
}

static int
lvorbis_load_string_chain (lua_State * L)
{
  avt_audio_t *audio_data;
  avt_audio_t **audio;

  audio_data = NULL;

  if (!lua_isnoneornil (L, 1))
    {
      size_t len;
      void *vorbis_data;

      vorbis_data = (void *) luaL_checklstring (L, 1, &len);
      audio_data = avta_load_vorbis_data (vorbis_data, (int) len);

      if (!audio_data)		/* call old avt.load_audio_string */
	{
	  lua_getfield (L, LUA_REGISTRYINDEX,
			"AVTVORBIS-old_load_audio_string");
	  lua_pushvalue (L, 1);	/* push audio data */

	  if (lua_pcall (L, 1, 1, 0) != 0)
	    {
	      lua_pushnil (L);	/* return nil on error */
	      lua_pushliteral (L, "unsupported audio format");
	      return 2;
	    }

	  return 1;
	}
    }

  collect_garbage (L);

  /* create audio structure with loaded audio_data */
  audio = (avt_audio_t **) lua_newuserdata (L, sizeof (avt_audio_t *));
  *audio = audio_data;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);

  return 1;
}


static const luaL_Reg vorbislib[] = {
  {"load_file", lvorbis_load_file},
  {"load_string", lvorbis_load_string},
  {NULL, NULL}
};

int
luaopen_vorbis (lua_State * L)
{
  /* require "lua-akfavatar" and get the address of the table */
  lua_getglobal (L, "require");
  lua_pushliteral (L, "lua-akfavatar");
  lua_call (L, 1, 1);		/* also gets table avt */

  /* save old avt.load_audio_file and avt.load_audio_string for chain loader */
  lua_getfield (L, -1, "load_audio_file");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_file");
  lua_getfield (L, -1, "load_audio_string");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_string");

  /* redefine avt.load_audio_file and avt.load_audio_string */
  lua_pushcfunction (L, lvorbis_load_file_chain);
  lua_setfield (L, -2, "load_audio_file");
  lua_pushcfunction (L, lvorbis_load_string_chain);
  lua_setfield (L, -2, "load_audio_string");

  lua_pop (L, 1);		/* pop avt */

  luaL_newlib (L, vorbislib);

  return 1;
}
