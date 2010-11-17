#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar-term"

avt.initialize {
  title = "Terminal",
  avatar = "default",  -- use "none" for most space
  encoding = "UTF-8",
  mode = "window"}

term.execute ()
