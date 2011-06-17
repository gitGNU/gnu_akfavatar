#!/usr/bin/env lua-akfavatar

--[[-------------------------------------------------------------------
Clock for AKFAvatar
Copyright (c) 2011 Andreas K. Foerster <info@akfoerster.de>

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
require "akfavatar-canvas"

avt.initialize {
  title = "Clock",
  avatar = "none",
  }

local function clock(c, timestamp)
  timestamp = timestamp or os.time()

  local time = os.date("*t", timestamp)
  local width, height = c:width(), c:height()
  local xcenter, ycenter = width/2, height/2
  local radius = math.min(width, height) / 2 - 10
  local v

  c:textalign("center", "center")
  c:text(os.date("%x", timestamp), xcenter, height * 3/4)

  -- show hour points
  c:thickness(7)
  for i=1,12 do
    c:putdot(xcenter + radius * math.sin(2*math.pi*i/12),
             ycenter - radius * math.cos(2*math.pi*i/12))
  end

  -- show minute points
  c:thickness(2)
  for i=1,60 do
    c:putdot(xcenter + radius * math.sin(2*math.pi*i/60),
             ycenter - radius * math.cos(2*math.pi*i/60))
  end

  -- hours pointer
  v = 2 * math.pi * ((time.hour % 12) * 5 + time.min / 12) / 60
  c:thickness(5)
  c:line (xcenter, ycenter, 
          xcenter + (radius/2) * math.sin(v),
          ycenter - (radius/2) * math.cos(v))

  -- minutes pointer
  v = 2 * math.pi * time.min / 60
  c:thickness(3)
  c:line (xcenter, ycenter, 
          xcenter + (radius - 12) * math.sin(v),
          ycenter - (radius - 12) * math.cos(v))
end

local c = canvas.new()
c:color "saddle brown"
clock(c)
c:show()
avt.wait(5.0)
