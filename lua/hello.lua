#!/usr/bin/env lua-akfavatar

--[[
hello world for AKFAvatar
Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
]]

-- Note: Since this is a trivial example, you don't have to keep my
-- Copyright notice, but you may replace it with your own.

require "lua-akfavatar" --> makes sure Lua-AKFAvatar is used

avt.initialize{title="Hello World", avatar="default", mode=avt.window_mode}
avt.encoding("UTF-8") --> the strings in this program are in UTF-8
avt.set_text_delay() --> activate the slowprint mode (optional)
avt.move_in() --> move the avatar in (optional)
avt.set_balloon_size(8, 20) --> set the size of the balloon (optional)

-- say something:
avt.say [[
Hello world
Bonjour le monde
Hallo Welt
Hej Världen
Καλημέρα κόσμε
]]

avt.newline()
avt.say("π≈", math.pi) --> avt.say accepts strings and numbers
avt.wait_button() --> wait for a button to be pressed
avt.move_out() --> move the avatar out (optional)
avt.quit() --> close the window and clean up
