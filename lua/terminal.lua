#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- global variables, so they can be used in APC commands
avt = require "lua-akfavatar"
term = require "akfavatar-term"

avt.encoding("")
avt.title("AKFAvatar Terminal", "Terminal")
avt.start()
avt.start_audio()
avt.avatar_image_file(avt.search "computer.xpm")
avt.set_avatar_mode "footer"
avt.set_balloon_color "black"

-- 25x80 is a traditional default, but other sizes are okay
avt.set_balloon_size(25, 80)

-- you could define global functions, which would be
-- accessible for the APC interface

-- guest programs can check this variable to see
-- if and what APC commands are accessible
term.setenv("APC", _VERSION .. ", lua-akfavatar")

term.color(true)
term.homedir()
term.execute(table.unpack(arg))
term.unsetenv("APC")

