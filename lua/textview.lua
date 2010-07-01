#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

avt.initialize{title="Text Viewer", encoding="UTF-8"}
avt.move_in()

if arg[1]
  then io.input(arg[1])
  else io.input(avt.file_selection())
  end

avt.pager(io.read("*all"))
avt.move_out()

