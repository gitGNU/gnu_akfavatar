#!/usr/bin/env lua-akfavatar

--[[-------------------------------------------------------------------
Clock for AKFAvatar
Copyright (c) 2011,2012 Andreas K. Foerster <info@akfoerster.de>

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

local avt = require "lua-akfavatar"
local graphic = require "akfavatar-graphic"

avt.title("Clock")
avt.start()

local function draw_clockface(gr, radius, color)
  local clockface = gr:duplicate()

  clockface:home()
  clockface:color("floral white")
  clockface:disc(radius-3)
  clockface:color(color)
  clockface:thickness(3)
  clockface:circle(radius-3)

  -- background image
  avt.set_bitmap_color("dark blue")
  clockface:put_file(avt.search("gnu-head.xbm"),
    clockface:width() / 2 - 268 / 2,
    clockface:height() / 4 - (253/2) + 20)

  -- draw minute points
  for minute=1,60 do
    clockface:home()
    clockface:heading(minute * 360/60)
    clockface:move(radius - 18)
    clockface:disc(2)
  end

  -- draw hours
  local textdist = 40
  clockface:textalign("center", "center")
  for hour=1,12 do
    clockface:home()
    clockface:heading(hour * 360/12)
    clockface:move(radius - textdist)
    clockface:text(hour)
    clockface:move(textdist - 18)
    clockface:disc(7)
  end

  -- show date
  clockface:home()
  clockface:heading(180)
  clockface:move(radius / 2)
  os.setlocale("", "time") --> for the formatting of the date
  clockface:text(os.date("%x", timestamp))

  -- middle dot
  clockface:home()
  clockface:disc(10)

  return clockface
end -- draw clockface


local function clock()
  local color = "saddle brown"
  local s = math.min(graphic.fullsize())
  local gr, width, height = graphic.new(s, s)
  local radius = s / 2
  local pointerlength = radius - 35
  local timestamp, oldtime
  local time = os.time
  local date = os.date

  gr:color(color)
  local clockface = draw_clockface(gr, radius, color)
  -- FIXME: date doesn't change at midnight

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
    gr:draw(pointerlength/2)

    -- minutes pointer
    gr:home()
    gr:heading(time.min * 360/60)
    gr:thickness(3)
    gr:draw(pointerlength)

    -- seconds pointer
    gr:home()
    gr:heading(time.sec * 360/60)
    gr:thickness(1)
    gr:draw(pointerlength)

    gr:show()
  until avt.key_pressed()

  avt.clear_keys()
end

clock()
