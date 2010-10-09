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
#include <unistd.h>		/* getcwd, chdir */
#include <locale.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* keep it short */
#define PRGNAME "Lua-AKFAvatar"

static lua_State *L;

/* from lua-avt.c */
extern int luaopen_akfavatar (lua_State * L);

/* from lbase64.c */
extern int luaopen_base64 (lua_State * L);

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
      else if (!strncmp (argv[i], "--dir=", 6))
	chdir (argv[i] + 6);
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

static avt_bool_t
is_lua (const char *filename)
{
  /* never show lua-akfavatar.lua! It's a module */
  if (strcasecmp (filename, "lua-akfavatar.lua") == 0)
    return AVT_FALSE;
  else
    {
      const char *ext = strrchr (filename, '.');
      return (avt_bool_t) (ext && strcasecmp (ext, ".lua") == 0);
    }
}

struct load_file_t
{
  FILE *f;
  char buffer[BUFSIZ];
};

static const char *
file_reader (lua_State * L AVT_UNUSED, void *data, size_t * size)
{
  struct load_file_t *fd;

  fd = (struct load_file_t *) data;

  *size = fread (fd->buffer, 1, sizeof (fd->buffer), fd->f);

  if (*size > 0)
    return (const char *) fd->buffer;
  else
    return NULL;
}

static int
load_file (const char *filename)
{
  int status;
  int c;
  struct load_file_t fd;

  fd.f = fopen (filename, "r");
  if (fd.f == NULL)
    {
      lua_pushfstring (L, "%s: %s", filename, strerror (errno));
      return LUA_ERRFILE;
    }

  /* scan for UTF-8 BOM (workaround for Windows notepad.exe) */
  if ((c = getc (fd.f)) == 0xEF && (c = getc (fd.f)) == 0xBB
      && (c = getc (fd.f)) == 0xBF)
    c = getc (fd.f);

  if (c == '#')
    {
      /*
       * skip to end of line
       * '\n' will be pushed back so linenumbers stay correct
       */
      do
	{
	  c = getc (fd.f);
	}
      while (c != '\n' && c != EOF);
    }

  if (c == LUA_SIGNATURE[0])
    {
      lua_pushfstring (L, "%s: binary rejected", filename);
      return LUA_ERRFILE;
    }

  ungetc (c, fd.f);

  lua_pushfstring (L, "@%s", filename);
  status = lua_load (L, file_reader, &fd, lua_tostring (L, -1));
  lua_remove (L, -2);		/* remove the filename */

  if (fclose (fd.f) != 0)
    {
      lua_pop (L, 1);		/* pop chunk or previous error msg */
      lua_pushfstring (L, "%s: %s", filename, strerror (errno));
      return LUA_ERRFILE;
    }

  return status;
}

/* loadfile with workaround for UTF-8 BOM and rejecting binaries */
static int
new_loadfile (lua_State * L)
{
  if (load_file (luaL_checkstring (L, 1)) != 0)
    {
      lua_pushnil (L);
      lua_insert (L, -2);	/* nil before error message */
      return 2;
    }

  return 1;
}

/* dofile with workaround for UTF-8 BOM and rejecting binaries */
static int
new_dofile (lua_State * L)
{
  const char *filename;

  filename = luaL_checkstring (L, 1);

  /* clear the stack */
  lua_settop (L, 0);

  if (load_file (filename) != 0)
    lua_error (L);

  lua_call (L, 0, LUA_MULTRET);

  /* only the results are left on the stack */
  return lua_gettop (L);
}

static void
initialize_lua (void)
{
  L = luaL_newstate ();
  if (L == NULL)
    avta_error ("cannot open Lua", "not enough memory");

  luaL_openlibs (L);

  /* register loader functions for: "lua-akfavatar" and "base64" */
  /* (users should not be able to leave the require command away) */
  lua_getglobal (L, "package");
  lua_getfield (L, -1, "preload");
  lua_pushcfunction (L, luaopen_akfavatar);
  lua_setfield (L, -2, "lua-akfavatar");
  lua_pushcfunction (L, luaopen_base64);
  lua_setfield (L, -2, "base64");
  lua_pop (L, 2);

  /* replace loadfile/dofile */
  lua_pushcfunction (L, new_loadfile);
  lua_setglobal (L, "loadfile");
  lua_pushcfunction (L, new_dofile);
  lua_setglobal (L, "dofile");
}

static avt_bool_t
ask_file (void)
{
  char filename[256];
  char lua_dir[4096 + 1];

  avt_set_balloon_size (0, 0);
  if (avta_file_selection (filename, sizeof (filename), &is_lua))
    return AVT_FALSE;

  if (*filename)
    {
      if (!getcwd (lua_dir, sizeof (lua_dir)))
	lua_dir[0] = '\0';

      /* arg[0] = filename */
      lua_newtable (L);
      lua_pushinteger (L, 0);
      lua_pushstring (L, filename);
      lua_settable (L, -3);
      lua_setglobal (L, "arg");

      if (load_file (filename) != 0 || lua_pcall (L, 0, 0, 0) != 0)
	{
	  /* on a normal quit-request there is nil on the stack */
	  if (lua_isstring (L, -1))
	    avta_error (lua_tostring (L, -1), NULL);
	  lua_pop (L, 1);	/* pop message (or the nil) */
	}

      if (lua_dir[0] != '\0')
	chdir (lua_dir);

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
  avta_prgname (PRGNAME);

  /* initialize Lua */
  initialize_lua ();
  atexit (quit);

  if (script_index)
    {
      get_args (argc, argv, script_index);

      if (load_file (argv[script_index]) != 0 || lua_pcall (L, 0, 0, 0) != 0)
	{
	  if (lua_isstring (L, -1))
	    avta_error (lua_tostring (L, -1), NULL);
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

	  /* script may have called avt.quit() */
	  if (avt_initialized ())
	    {
	      avt_set_background_color_name ("default");
	      avt_set_balloon_color_name ("floral white");
	      avt_markup (AVT_FALSE);
	      avt_normal_text ();
	      avt_quit_audio ();
	      avt_set_title ("Lua-AKFAvatar Starter", "AKFAvatar");
	      avt_change_avatar_image (avt_default ());
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
