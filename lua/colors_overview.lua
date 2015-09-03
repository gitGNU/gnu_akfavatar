#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2011,2012 Andreas K. Foerster <akf@akfoerster.de>
-- License: GPL version 3 or later

-- this gives an overview which color names are available
-- You can mix any deliberate color with the hexadecimal RGB notation

local avt = require "lua-akfavatar"

avt.title("colors overview", "colors")
avt.start()

local color = avt.color_selection()
if color then
  avt.set_background_color(color)
  avt.clear_screen()
  avt.wait_button()
end
