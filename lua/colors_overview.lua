#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2011 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- this gives an overview which color names are available
-- You can mix any deliberate color with thew hexadecimasl RGB notation

require "lua-akfavatar"

avt.initialize {
  title = "colors overview",
  shortname = "colors",
  avatar = "none"
  }

avt.set_background_color(avt.color_selection())
avt.clear_screen()
avt.wait_button()
