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

local oldtime = -1

local function clock(gr, show_date, timestamp)
  timestamp = timestamp or os.time()

  -- only draw when timestamp changes
  -- timestamp changes every second
  if timestamp == oldtime then return end
  oldtime = timestamp

  local time = os.date("*t", timestamp)
  local width, height = gr:width(), gr:height()
  local radius = math.min(width, height) / 2 - 10

  gr:clear()

  if show_date then
    gr:draw(false)
    gr:home()
    gr:back(height / 4)
    gr:textalign("center", "center")
    gr:text(os.date("%x", timestamp))
  end

  -- draw dots
  gr:draw(false)

  -- show minute points
  gr:thickness(2)
  gr:draw(false) --> don't draw while moving
  for i=1,60 do
    gr:home()
    gr:heading(i * 360/60)
    gr:forward(radius)
    gr:putdot()
  end

  -- show hour points
  gr:thickness(7)
  for i=1,12 do
    gr:home()
    gr:heading(i * 360/12)
    gr:forward(radius)
    gr:putdot()
  end

  -- draw pointers
  gr:draw(true)

  -- hours pointer
  gr:home()
  gr:heading((time.hour*60/12 + time.min/12) * 360/60)
  gr:thickness(6)
  gr:forward(radius/2)

  -- minutes pointer
  gr:home()
  gr:heading(time.min * 360/60)
  gr:thickness(3)
  gr:forward(radius-12)

  -- seconds pointer
  gr:home()
  gr:heading(time.sec * 360/60)
  gr:thickness(1)
  gr:forward(radius-12)

  gr:show()
end

local s = math.min(graphic.fullsize())
local gr = graphic.new(s, s)
gr:color "saddle brown"
os.setlocale("", "time") --> for the formatting of the date

-- close with <Esc>-key or the close-button of the window
while true do
  clock(gr, true)
  avt.wait(0.1)
end
