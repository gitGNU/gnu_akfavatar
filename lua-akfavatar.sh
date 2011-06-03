#!/bin/sh
# this file is in the Public Domain -- AKFoerster

# This script sets up search paths for the uninstalled package
# It is not meant for installation!

PWD="$(pwd)"
localdir=$(dirname "$0")
test "$localdir" = "." && localdir="$PWD" || localdir="$PWD/$localdir"

# for finding Lua modules
# the ;; at the end adds the default path
LUA_PATH="$localdir/lua/?.lua;;"
export LUA_PATH

# for finding Lua binary modules
# the ;; at the end adds the default path
LUA_CPATH="$localdir/?.so;$localdir/lua/?.so;;"
export LUA_CPATH

# On HP-UX change LD_LIBRARY_PATH to SHLIB_PATH
# On AIX change LD_LIBRARY_PATH to LIBPATH
# On Darwin/MacOS X change LD_LIBRARY_PATH to DYLD_LIBRARY_PATH
LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}$localdir"
export LD_LIBRARY_PATH

exec "$localdir/lua-akfavatar-bin" "$@"
