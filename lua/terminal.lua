#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar-term"

avt.initialize {
  title = "AKFAvatar Terminal",
  shortname = "Terminal",
  avatar = "none",
  audio = true, -- for the bell ("\a")
  mode = "auto"
  }

-- guest programs can check this variable to see
-- if and what APC commands are accessible
term.setenv("APC", _VERSION .. ", lua-akfavatar")

term.color(true)
term.homedir()
term.execute(unpack(arg))
term.unsetenv("APC")
