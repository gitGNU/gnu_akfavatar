/*
 * AKFAvatar Ogg Vorbis decoder
 * based on stb_vorbis
 * Copyright (c) 2011,2012 Andreas K. Foerster <info@akfoerster.de>
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

#define to_bool(L, index)  ((bool) lua_toboolean ((L), (index)))

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

/* registers audio structure at table on to of stack */
static void
make_audio_element (lua_State * L, avt_audio * audio_data)
{
  avt_audio **audio;

  audio = (avt_audio **) lua_newuserdata (L, sizeof (avt_audio *));
  *audio = audio_data;
  luaL_getmetatable (L, AUDIODATA);
  lua_setmetatable (L, -2);
}

static int
lvorbis_load_file (lua_State * L)
{
  char *filename;
  avt_audio *audio_data;
  bool play;

  collect_garbage (L);
  filename = (char *) luaL_checkstring (L, 1);
  play = to_bool (L, 2);

  audio_data = avta_load_vorbis_file (filename, play);

  if (audio_data == NULL)
    {
      lua_pushnil (L);
      return 1;
    }

  make_audio_element (L, audio_data);

  return 1;
}

static int
lvorbis_load_stream (lua_State * L)
{
  luaL_Stream *stream;
  lua_Unsigned size;
  avt_audio *audio_data;
  bool play;

  collect_garbage (L);
  stream = (luaL_Stream *) luaL_checkudata (L, 1, LUA_FILEHANDLE);
  size = lua_tounsigned (L, 2);	/* nothing or 0 allowed */
  play = to_bool (L, 3);

  if (stream->closef == NULL)
    return luaL_error (L, "attempt to use a closed file");

  audio_data = avta_load_vorbis_stream (stream->f, size, play);

  if (audio_data == NULL)
    {
      lua_pushnil (L);
      return 1;
    }

  make_audio_element (L, audio_data);

  return 1;
}

static int
lvorbis_load (lua_State * L)
{
  size_t len;
  void *vorbis_data;
  avt_audio *audio_data;
  bool play;

  collect_garbage (L);
  vorbis_data = (void *) luaL_checklstring (L, 1, &len);
  play = to_bool (L, 2);

  audio_data = avta_load_vorbis_data (vorbis_data, (int) len, play);

  if (audio_data == NULL)
    {
      lua_pushnil (L);
      return 1;
    }

  make_audio_element (L, audio_data);

  return 1;
}

static int
lvorbis_load_file_chain (lua_State * L)
{
  avt_audio *audio_data;

  audio_data = NULL;

  if (!lua_isnoneornil (L, 1))
    {
      char *filename;
      bool play;

      filename = (char *) luaL_checkstring (L, 1);
      play = to_bool (L, 2);

      audio_data = avta_load_vorbis_file (filename, play);

      if (!audio_data)
	{
	  lua_getfield (L, LUA_REGISTRYINDEX,
			"AVTVORBIS-old_load_audio_file");
	  lua_pushvalue (L, 1);	/* push filename */
	  lua_pushboolean (L, play);

	  if (lua_pcall (L, 2, 1, 0) != 0)
	    {
	      lua_pushnil (L);	/* return nil on error */
	      lua_pushliteral (L, "unsupported audio format");
	      return 2;
	    }

	  return 1;
	}
    }

  collect_garbage (L);

  make_audio_element (L, audio_data);

  return 1;
}

static int
lvorbis_load_stream_chain (lua_State * L)
{
  luaL_Stream *stream;
  lua_Unsigned maxsize;
  avt_audio *audio_data;
  bool play;

  stream = (luaL_Stream *) luaL_checkudata (L, 1, LUA_FILEHANDLE);
  maxsize = lua_tounsigned (L, 2);	/* nothing or 0 allowed */
  play = to_bool (L, 3);

  if (stream->closef == NULL)
    return luaL_error (L, "attempt to use a closed file");

  audio_data = avta_load_vorbis_stream (stream->f, maxsize, play);

  if (!audio_data)
    {
      lua_getfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_stream");
      lua_pushvalue (L, 1);	/* push stream handle */
      lua_pushunsigned (L, maxsize);
      lua_pushboolean (L, play);

      if (lua_pcall (L, 3, 1, 0) != 0)
	{
	  lua_pushnil (L);	/* return nil on error */
	  lua_pushliteral (L, "unsupported audio format");
	  return 2;
	}

      return 1;
    }

  make_audio_element (L, audio_data);
  collect_garbage (L);

  return 1;
}

static int
lvorbis_load_chain (lua_State * L)
{
  avt_audio *audio_data;

  audio_data = NULL;

  if (!lua_isnoneornil (L, 1))
    {
      size_t len;
      void *vorbis_data;
      bool play;

      vorbis_data = (void *) luaL_checklstring (L, 1, &len);
      play = to_bool (L, 2);
      audio_data = avta_load_vorbis_data (vorbis_data, (int) len, play);

      if (!audio_data)		/* call old avt.load_audio */
	{
	  lua_getfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio");
	  lua_pushvalue (L, 1);	/* push audio data */
	  lua_pushboolean (L, play);

	  if (lua_pcall (L, 2, 1, 0) != 0)
	    {
	      lua_pushnil (L);	/* return nil on error */
	      lua_pushliteral (L, "unsupported audio format");
	      return 2;
	    }

	  return 1;
	}
    }

  make_audio_element (L, audio_data);
  collect_garbage (L);

  return 1;
}


static const luaL_Reg vorbislib[] = {
  {"load_file", lvorbis_load_file},
  {"load_stream", lvorbis_load_stream},
  {"load", lvorbis_load},
  {NULL, NULL}
};

int
luaopen_vorbis (lua_State * L)
{
  /* require "lua-akfavatar" and get the address of the table */
  lua_getglobal (L, "require");
  lua_pushliteral (L, "lua-akfavatar");
  lua_call (L, 1, 1);		/* also gets table avt */

  /* save original functions for chain loaders */
  lua_getfield (L, -1, "load_audio_file");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_file");
  lua_getfield (L, -1, "load_audio_stream");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_stream");
  lua_getfield (L, -1, "load_audio");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio");

  /* redefine avt.load_audio_file and avt.load_audio */
  lua_pushcfunction (L, lvorbis_load_file_chain);
  lua_setfield (L, -2, "load_audio_file");
  lua_pushcfunction (L, lvorbis_load_stream_chain);
  lua_setfield (L, -2, "load_audio_stream");
  lua_pushcfunction (L, lvorbis_load_chain);
  lua_setfield (L, -2, "load_audio");

  lua_pop (L, 1);		/* pop avt */

  luaL_newlib (L, vorbislib);

  return 1;
}
