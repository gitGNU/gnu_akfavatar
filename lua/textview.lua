#!/usr/bin/env lua-akfavatar
require "lua-akfavatar"

--[[
text-viewer
Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>

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

avt.initialize{title="Text Viewer"}
avt.move_in()

if arg[1]
  then io.input(arg[1])
  else io.input(avt.file_selection())
  end

avt.pager(io.read("*all"))
avt.move_out()

