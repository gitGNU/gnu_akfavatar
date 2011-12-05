#!/usr/bin/env lua-akfavatar

--[[-------------------------------------------------------------------
Function Plotter for AKFAvatar
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

-- how many units to show on the x axis at the default scale
x_units = 2 * 10

-- zoom factor
zoom = 1.25

-- animation when drawing the first time? (true or false)
animation = true

-- allow comma as decimal mark? (true or false)
-- when set to true, you cannot use functions with more than one argument
comma = true

deg = math.deg
rad = math.rad
sqrt = math.sqrt
exp = math.exp
log = math.log
log10 = math.log10
pi = math.pi
sin = math.sin
sinh = math.sinh
asin = math.asin
cos = math.cos
cosh = math.cosh
acos = math.acos
tan = math.tan
tanh = math.tanh
atan = math.atan

-- add your own functions and constants here

-------------------------------------------------------------------------------
require "lua-akfavatar"
require "akfavatar-graphic"

avt.initialize {
  title = "Function Plotter",
  avatar = assert(avt.search("teacher.xpm")),
  encoding = "UTF-8"
  }

local gr, width, height = graphic.new()
local xoffset, yoffset = width/2, height/2
local scale = width / x_units

local function px(x) -- physical x
  return x * scale + xoffset
end

local function lx(x) -- logical x
  return (x - xoffset) / scale
end

local function py(y) -- physical y
  return y * -scale + yoffset
end

local function ly(y) -- logical y
  return -(y - yoffset) / scale
end

local function drawgrid(step)
  gr:thickness(1)

  if 1 == step then
    gr:color "grey75" -- light for 1's
  elseif 10 == step then
    gr:color "grey55" -- darker for 10's
  else
    return
  end

  for x = step, lx(width), step do
    gr:line (px(x), 1, px(x), height)
  end

  for x = -step, lx(1), -step do
    gr:line (px(x), 1, px(x), height)
  end

  for y = step, ly(1), step do
    gr:line (1, py(y), width, py(y))
  end

  for y = -step, ly(height), -step do
    gr:line (1, py(y), width, py(y))
  end
end

local function grid()
  local s = 5 --> size of the marks
  local step

  gr:thickness(1)
  gr:clear()

  if scale > 10 then
    step = 1
    drawgrid(1)
    drawgrid(10)
  elseif scale > 1 then
    step = 10
    drawgrid(10)
  else
    step = 0
  end

  gr:color "black"

  -- cross
  gr:line (1, yoffset, width, yoffset)
  gr:line (xoffset, 1, xoffset, height)

  -- ticks
  if step > 0 then
    for x = step, lx(width), step do
      gr:line (px(x), yoffset - s,
              px(x), yoffset + s)
    end

    for x = -step, lx(1), -step do
      gr:line (px(x), yoffset - s,
              px(x), yoffset + s)
    end

    for y = step, ly(1), step do
      gr:line (xoffset - s, py(y),
              xoffset + s, py(y))
    end

    for y = -step, ly(height), -step do
      gr:line (xoffset - s, py(y),
              xoffset + s, py(y))
    end
  end
end

local function function_error()
  avt.tell("The function was not\nin a correct notation!")
  avt.wait_button()
end

local function reset()
  xoffset = width / 2
  yoffset = height / 2
  scale = width / x_units
end

local function textplate(s)
  local fwidth, fheight = gr:font_size()
  local p, pwidth, pheight = graphic.new((#s*fwidth)+8, fheight+8, "gold")

  p:center()
  p:textalign("center", "center")
  p:color "black"
  p:text(s)

  return p
end

local function plot(f, fstr)
  local choice
  local animate = animation
  local zoom_factor = zoom
  local functionplate = textplate(fstr)

  reset()

  repeat
    grid()

    if animate then gr:put(functionplate, 4, 4) end

    gr:thickness(2)
    gr:color "royal blue"  --> don't tell my old teacher what pen I use ;-)

    local old_value = f(lx(0))
    gr:moveto(0, py(old_value)) --> offscreen

    for x=1,width do
      local value = f(lx(x))
      local y = py(value)

      -- an invalid number (NaN) is not equal to itself
      if old_value ~= old_value then
        gr:moveto (x, y)
      elseif value*old_value >= 0 or math.abs(old_value - value) < 10
        then gr:lineto (x, y)
        else -- jump with changed sign => assume infinity
          if old_value < 0 then
            gr:lineto (x-1, height)
            gr:line (x, 0, x, y)
          else
            gr:lineto (x-1, 0)
            gr:line (x, height, x, y)
          end
      end

      -- show each time we reach a full value and it's visible
      if animate and x % scale == 0 and y >= 1 and y <= height then
        gr:show()
        avt.wait(0.2)
      end

      old_value = value
    end

    gr:put(functionplate, 4, 4)

    gr:show() --> show final result
    animate = false --> only animate the first time

    choice = avt.navigate "+-udlrsx"
    if "+"==choice then scale = scale * zoom_factor
    elseif "-"==choice then scale = scale / zoom_factor
    elseif "l"==choice then xoffset = xoffset + width/4
    elseif "r"==choice then xoffset = xoffset - width/4
    elseif "u"==choice then yoffset = yoffset + height/4
    elseif "d"==choice then yoffset = yoffset - height/4
    elseif "s"==choice then reset()
    end
  until "x"==choice
end

local function plot_string(s)
  if comma then
    s = string.gsub(s, ",", ".") --> many countries use a comma as decimal mark
  end

  local fs = loadstring("function f(x) return "..s.." end")
  if fs and pcall(fs) then
    -- check if it produces a number
    local okay, v = pcall(f, 1)
    if okay and type(v)=="number" then
      plot(f, "f(x)="..s)
    else
      return function_error()
    end
  else
    return function_error()
  end
end

if arg[1] then
  plot_string(arg[1])
else
  repeat
    avt.set_balloon_size(2, 0)
    avt.say "Enter the function:\n"
    local funcstring = avt.ask "f(x)="
    if funcstring~="" then plot_string(funcstring) end
  until funcstring == ""
end
