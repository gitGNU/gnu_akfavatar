#!/usr/bin/env lua

--[[
hello world for AKFAvatar
Copyright (c) 2009 Andreas K. Foerster

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


-- this loads the binding to AKFAvatar
require "lua-avt"

-- initialize with default settings
avt.initialize()

-- the strings in this program are in UTF-8
avt.encoding("UTF-8")

-- activate the slowprint mode (optional)
avt.set_text_delay()

-- move the avatar in (optional)
avt.move_in()

-- set the size of the balloon (optional)
avt.set_balloon_size(8, 20)

-- say something
avt.say [[
Hello world
Bonjour le monde
Hallo Welt
Hej Världen
Καλημέρα κόσμε
Здравствуй мир
]]

avt.newline()

-- avt.say accepts strings and numbers
avt.say("π≈", math.pi)

-- wait for a button to be pressed
avt.wait_button()

-- move the avatar out (optional)
avt.move_out()

-- close the window and clean up
avt.quit()
