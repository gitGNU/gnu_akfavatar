#!/usr/bin/env lua-akfavatar
require "lua-akfavatar"

--[[
Manpage viewer for AKFAvatar (just runs on some systems)
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

avt.initialize{title="Manpage"}
avt.encoding("ISO-8859-1")

function underlined(text)
  return string.gsub(text, ".", "_\b%1")
end

function ask()
  avt.set_balloon_size(3, 40)
  avt.say("man [", underlined("Option"), " ...] ",
          "[", underlined("Section"), "] ",
          underlined("Page"), " ...\n\n",
          "man ")
  local answer = avt.ask()
  return answer
end

avt.move_in()

local page

if arg[1]
  then page = table.concat(arg, " ")
  else page = ask()
  end

local manpage = io.popen(
  "env MANWIDTH=80 GROFF_TYPESETTER=latin1 GROFF_NO_SGR=1 man -t "
  .. page .. " 2>&1")

avt.set_balloon_size(0, 0)
avt.pager(manpage:read("*all"))
manpage:close()

avt.move_out()

