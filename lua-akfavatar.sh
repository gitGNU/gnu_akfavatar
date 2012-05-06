#!/bin/sh
# this file is in the Public Domain -- AKFoerster

# This script sets up search paths for the uninstalled package
# It is not meant for installation!

PWD="$(pwd)"
localdir=$(dirname "$0")

if [ "$localdir" = "." ]
then localdir="$PWD"
else if [ -d "$PWD/$localdir" ] # relative?
then localdir="$PWD/$localdir" # make absolute
fi
fi

# for finding Lua modules
# the ;; at the end adds the default path
LUA_PATH="${LUA_PATH:+$LUA_PATH;}$localdir/lua/?.lua;;"

# for finding Lua binary modules
# the ;; at the end adds the default path
LUA_CPATH="${LUA_CPATH:+$LUA_CPATH;}$localdir/?.so;$localdir/lua/?.so;;"

# for finding data (images, sounds, ...)
AVTDATAPATH="${AVTDATAPATH:+$AVTDATAPATH;}$localdir/data;/usr/local/share/akfavatar;/usr/share/akfavatar"

export LUA_PATH LUA_CPATH AVTDATAPATH

exec "$localdir/lua-akfavatar-bin" "$@"
