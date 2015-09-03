/*
 * AKFAvatar Ogg Vorbis decoder
 * based on stb_vorbis
 * Copyright (c) 2011,2012,2013,2015
 * Andreas K. Foerster <akf@akfoerster.de>
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
#define _XOPEN_SOURCE 600

#include "akfavatar.h"
#include "avtaddons.h"
#include <stdlib.h>
#include <iso646.h>


#ifdef __cplusplus
extern "C"
{
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

  extern int luaopen_vorbis (lua_State * L);

#ifdef __cplusplus
}
#endif

// internal name for audio data
#define AUDIODATA   "AKFAvatar-Audio"

#define to_bool(L, index)  ((bool) lua_toboolean ((L), (index)))

// modes for playing audio
static const char *const playmodes[] = { "load", "play", "loop", NULL };

static void
collect_garbage (lua_State * L)
{
  // make current sound collectable, if it's not playing
  if (not avt_audio_playing (NULL))
    {
      lua_pushnil (L);
      lua_setfield (L, LUA_REGISTRYINDEX, "AKFAvatar-current-sound");
    }

  lua_gc (L, LUA_GCCOLLECT, 0);
}

// registers audio structure at table on to of stack
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
  int playmode;

  collect_garbage (L);
  filename = (char *) luaL_checkstring (L, 1);
  playmode = luaL_checkoption (L, 2, "load", playmodes);

  audio_data = avt_load_vorbis_file (filename, playmode);

  if (not audio_data)
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
  lua_Integer size;
  avt_audio *audio_data;
  int playmode;

  collect_garbage (L);
  stream = (luaL_Stream *) luaL_checkudata (L, 1, LUA_FILEHANDLE);
  size = lua_tointeger (L, 2);	// nothing or 0 allowed
  playmode = luaL_checkoption (L, 3, "load", playmodes);

  if (not stream->closef)
    return luaL_error (L, "attempt to use a closed file");

  audio_data = avt_load_vorbis_stream (stream->f, size, false, playmode);

  if (not audio_data)
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
  int playmode;

  collect_garbage (L);
  vorbis_data = (void *) luaL_checklstring (L, 1, &len);
  playmode = luaL_checkoption (L, 2, "load", playmodes);

  audio_data = avt_load_vorbis_data (vorbis_data, (int) len, playmode);

  if (not audio_data)
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

  if (not lua_isnoneornil (L, 1))
    {
      char *filename;
      int playmode;

      filename = (char *) luaL_checkstring (L, 1);
      playmode = luaL_checkoption (L, 2, "load", playmodes);

      audio_data = avt_load_vorbis_file (filename, playmode);

      if (not audio_data)
	{
	  lua_getfield (L, LUA_REGISTRYINDEX,
			"AVTVORBIS-old_load_audio_file");
	  lua_pushvalue (L, 1);	// push filename
	  lua_pushstring (L, playmodes[playmode]);

	  if (lua_pcall (L, 2, 1, 0) != 0)
	    {
	      lua_pushnil (L);	// return nil on error
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
  lua_Integer maxsize;
  avt_audio *audio_data;
  int playmode;

  stream = (luaL_Stream *) luaL_checkudata (L, 1, LUA_FILEHANDLE);
  maxsize = lua_tointeger (L, 2);	// nothing or 0 allowed
  playmode = luaL_checkoption (L, 3, "load", playmodes);

  if (not stream->closef)
    return luaL_error (L, "attempt to use a closed file");

  audio_data = avt_load_vorbis_stream (stream->f, maxsize, false, playmode);

  if (not audio_data)
    {
      lua_getfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_stream");
      lua_pushvalue (L, 1);	// push stream handle
      lua_pushinteger (L, maxsize);
      lua_pushstring (L, playmodes[playmode]);

      if (lua_pcall (L, 3, 1, 0) != 0)
	{
	  lua_pushnil (L);	// return nil on error
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

  if (not lua_isnoneornil (L, 1))
    {
      size_t len;
      void *vorbis_data;
      int playmode;

      vorbis_data = (void *) luaL_checklstring (L, 1, &len);
      playmode = luaL_checkoption (L, 2, "load", playmodes);
      audio_data = avt_load_vorbis_data (vorbis_data, (int) len, playmode);

      if (not audio_data)	// call old avt.load_audio
	{
	  lua_getfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio");
	  lua_pushvalue (L, 1);	// push audio data
	  lua_pushstring (L, playmodes[playmode]);

	  if (lua_pcall (L, 2, 1, 0) != 0)
	    {
	      lua_pushnil (L);	// return nil on error
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
  // require "lua-akfavatar" and get the address of the table
  lua_getglobal (L, "require");
  lua_pushliteral (L, "lua-akfavatar");
  lua_call (L, 1, 1);		// also gets table avt

  // save original functions for chain loaders
  lua_getfield (L, -1, "load_audio_file");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_file");
  lua_getfield (L, -1, "load_audio_stream");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio_stream");
  lua_getfield (L, -1, "load_audio");
  lua_setfield (L, LUA_REGISTRYINDEX, "AVTVORBIS-old_load_audio");

  // redefine avt.load_audio_file and avt.load_audio
  lua_pushcfunction (L, lvorbis_load_file_chain);
  lua_setfield (L, -2, "load_audio_file");
  lua_pushcfunction (L, lvorbis_load_stream_chain);
  lua_setfield (L, -2, "load_audio_stream");
  lua_pushcfunction (L, lvorbis_load_chain);
  lua_setfield (L, -2, "load_audio");

  lua_pop (L, 1);		// pop avt

  luaL_newlib (L, vorbislib);

  return 1;
}
