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
require "akfavatar-graphic"

avt.initialize {
  title = "Clock",
  avatar = "none",
  }

local function draw_clockface(gr, radius, color)
  local clockface = gr:duplicate()

  clockface:home()
  clockface:color("floral white")
  clockface:disc(radius-1)
  clockface:color(color)

  -- draw minute points
  for i=1,60 do
    clockface:home()
    clockface:heading(i * 360/60)
    clockface:move(radius - 12)
    clockface:disc(2)
  end

  -- draw hour points
  for i=1,12 do
    clockface:home()
    clockface:heading(i * 360/12)
    clockface:move(radius - 12)
    clockface:disc(7)
  end

  -- show date
  clockface:home()
  clockface:heading(180)
  clockface:move(radius / 2)
  os.setlocale("", "time") --> for the formatting of the date
  clockface:text(os.date("%x", timestamp))

  return clockface
end -- draw clockface


local function clock()
  local color = "saddle brown"
  local s = math.min(graphic.fullsize())
  local gr, width, height = graphic.new(s, s)
  local radius = s / 2
  local timestamp, oldtime
  local time = os.time
  local date = os.date

  gr:color(color)
  local clockface = draw_clockface(gr, radius, color)
  -- FIXME: date doesn't change at midnight

  radius = radius - 12
  timestamp = time()

  repeat

    -- wait until timestamp actually changes (every second)
    while timestamp == oldtime do
      avt.wait(0.1)
      timestamp = time()
    end

    oldtime = timestamp

    gr:put(clockface) --> overwrites everything

    local time = date("*t", timestamp)

    -- hours pointer
    gr:home()
    gr:heading((time.hour*60/12 + time.min/12) * 360/60)
    gr:thickness(6)
    gr:draw(radius/2)

    -- minutes pointer
    gr:home()
    gr:heading(time.min * 360/60)
    gr:thickness(3)
    gr:draw(radius-14)

    -- seconds pointer
    gr:home()
    gr:heading(time.sec * 360/60)
    gr:thickness(1)
    gr:draw(radius-14)

    gr:show()
  until false

end

clock()
