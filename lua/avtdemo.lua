#!/usr/bin/env lua-akfavatar

--[[-------------------------------------------------------------------
Making demos for Lua-AKFAvatar
Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
--]]-------------------------------------------------------------------

require "lua-akfavatar"
require "akfavatar.avtdemo"

if arg[1]
  then
    -- split directory and filename
    -- set the working directory, for required files could be there
    local dir, name = string.match(arg[1], "^(.-)([^/\\]+)$")
    avt.set_directory(dir)
    avtdemo(name)
  else
    avtdemo(
      avt.file_selection(
          function(n)
            return string.find(string.lower(n), "%.avt$")
          end))
end

