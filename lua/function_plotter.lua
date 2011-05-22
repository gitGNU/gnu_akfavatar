#!/usr/bin/env lua-akfavatar

require "lua-akfavatar"
require "akfavatar-canvas"

-- scale means: so many pixels for the value 1
local scale = 40

avt.initialize {
  title = "Function Plotter",
  avatar = require "akfavatar.teacher",
  encoding = "UTF-8"
  }

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
tan = math.tan
tanh = math.tanh
atan = math.atan

local c, width, height = canvas.new()
local halfwidth, halfheight = width/2, height/2

local function px(x) -- physical x
  return x * scale + halfwidth
end

local function lx (x) -- logical x
  return (x - halfwidth) / scale
end

local function py (y) -- physical y
  return y * -scale + halfheight
end

local function ly (y) -- logical y
  return -(y - halfheight) / scale
end

local function cross()
  local s = 5 --> size of the marks

  c:thickness(1)
  c:color "black"

  c:line (1, halfheight, width, halfheight)
  c:line (halfwidth, 1, halfwidth, height)

  for x = 1, lx(width) do
    c:line (px(x), halfheight - s,
            px(x), halfheight + s)
  end

  for x = -1, lx(1), -1 do
    c:line (px(x), halfheight - s,
            px(x), halfheight + s)
  end

  for y = 1, ly(1) do
    c:line (halfwidth - s, py(y),
            halfwidth + s, py(y))
  end

  for y = -1, ly(height), -1 do
    c:line (halfwidth - s, py(y),
            halfwidth + s, py(y))
  end

end

local function function_error()
  error("The function was not in a correct notation", 0)
end

local function plot(f)
  cross()

  c:thickness(2)
  c:color "royal blue"  --> don't tell my old teacher what pen I use ;-)

  local old_y = math.huge --> something offscreen

  for x=1,width do
    local okay, y = pcall(f, lx(x))

    if not okay or type(y)~="number" then function_error() end

    y = py(y)
    if math.abs(old_y - y) < height
      then c:lineto (x, y)
      else c:moveto (x, y) --> don't draw huge jumps
    end

    -- show each time we reach a full value and it's visible
    -- if you don't like animations, just remove this part
    if x % scale == 0 and y >= 1 and y <= height then 
      c:show()
      avt.wait(0.2)
    end

    old_y = y
  end

  c:show() -- show final result
  avt.wait_button()
end

local function plot_string(s)
  local fs = loadstring("function f(x) return "..s.." end")
  if fs and pcall(fs) then
    plot(f)
  else
    function_error()
  end
end

local funcstring = arg[1]
if not funcstring then
  avt.set_balloon_height(2)
  avt.say "Enter the function:\n"
  funcstring = avt.ask "f(x)="
end

if funcstring and funcstring ~= "" then
  plot_string(funcstring)
end

