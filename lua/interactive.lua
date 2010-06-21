#!/usr/bin/env lua-akfavatar

--[[
interactive Lua shell for AKFAvatar
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

require "lua-akfavatar"

local prompt = "> "
local cmd, func, err, success

print = function(...)
  local i, v
  for i,v in ipairs({...}) do
    if i > 1 then avt.next_tab() end
    avt.say(tostring(v))
    end
  avt.newline()
end

say = print --> an alias that fits better

avt.initialize()
avt.move_in()
avt.say(_VERSION, " / AKFAvatar ", avt.version(), "\n\n")

repeat
  avt.say(prompt)
  cmd = avt.ask()
  cmd = string.gsub(cmd, "^=", "return ", 1)
  func, err = loadstring(cmd)
  if not func
    then print(err)
    else
      local ret = {pcall(func)}
      success = table.remove(ret, 1)
      if not success -- error calling the function?
        then print(ret[1])
        else if ret[1] then print(unpack(ret)) end
      end
  end
until false
