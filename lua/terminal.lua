#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar-term"

avt.initialize {
  title = "AKFAvatar Terminal",
  shortname = "Terminal",
  audio = true, --> for the bell ("\a")
  mode = "auto",
  avatar = avt.search("computer.xpm") or "none"
}

-- 25x80 is a traditional default, but other sizes are okay
avt.set_balloon_size(25, 80)

-- you could define global functions, which would be
-- accessible for the APC interface

-- guest programs can check this variable to see
-- if and what APC commands are accessible
term.setenv("APC", _VERSION .. ", lua-akfavatar")

term.color(true)
term.homedir()
term.execute(unpack(arg))
term.unsetenv("APC")

