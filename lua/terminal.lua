#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar-term"

avt.initialize {
  title = "AKFAvatar Terminal",
  shortname = "Terminal",
  avatar = "default",  -- use "none" for most space
  audio = true, -- for the bell ("\a")
  mode = "auto"
  }

term.color(true)

if arg[1] then
  term.execute(unpack(arg))
else
  term.execute() --> execute a shell
end
