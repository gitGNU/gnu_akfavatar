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

local oldtime, clockface

local function draw_clockface(gr, radius, color)
  if clockface then
    clockface:clear()
  else
    clockface = gr:duplicate()
  end

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
  clockface:text(os.date("%x", timestamp))

end -- draw clockface


local function clock(gr, color, timestamp)
  timestamp = timestamp or os.time()

  -- only draw when timestamp changes (every second)
  if timestamp == oldtime then return end
  oldtime = timestamp

  local time = os.date("*t", timestamp)
  local width, height = gr:size()
  local radius = math.min(width, height) / 2

  if not clockface then draw_clockface(gr, radius, color) end
  -- FIXME: date doesn't change at midnight

  gr:put(clockface)
  radius = radius - 12

  gr:color(color)

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
  gr:draw(radius-12)

  gr:show()
end

local s = math.min(graphic.fullsize())
local gr = graphic.new(s, s)

os.setlocale("", "time") --> for the formatting of the date

-- close with <Esc>-key or the close-button of the window
while true do
  clock(gr, "saddle brown")
  avt.wait(0.1)
end
