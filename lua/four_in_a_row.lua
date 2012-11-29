#!/usr/bin/env lua-akfavatar

--[[---------------------------------------------------------------------
Four in a Row
Game for 2 players (no computer-logic yet)

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


Keys:
  left / right - choose column
  1 - 7 - choose column
  enter / down - drop chip
--]]--------------------------------------------------------------------

local avt = require "lua-akfavatar"
local graphic = require "akfavatar-graphic"

local color = {
  background = "default",
  board = "saddle brown",
  text = "black",
  numbers = "white",
  chip = {[1] = "white", [2] = "black"},
  connector = "green"
  }

-- false for no sound
local sound = {
  success = "hahaha.au",
  remis = "question.au",
  full = "harrumph.au"
  }

avt.translations = {
  -- avoid trademarked names
  ["Four in a row"] = {
    de = "Vier in einer Reihe",
    },

  ["keys:"] = {
    de = "Tasten:",
  }
}

------------------------------------------------------------------------

local L = avt.translate

avt.encoding("UTF-8")
avt.title(L"Four in a row")
avt.set_background_color(color.background)
avt.start()
avt.start_audio()

local logo = graphic.new(64, 64)
logo:put_file(avt.search("akf64.xpm"))

local score = {[1] = 0, [2] = 0}
local player = 1

local screen, width, height = graphic.new()
local fwidth, fheight = graphic.font_size()
local fieldsize = height / 7
local radius = fieldsize / 2 - 10
local boardwidth, boardheight = fieldsize * 7, height - fieldsize
local boardxoffset = 1 + width/2 - boardwidth/2
local boardyoffset = 1 + fieldsize
local filled = {} -- how many chips in a column
local chips = 0 -- how many chips alltogether
local board = {}

local success = avt.load_audio_file(avt.search(sound.success)) or avt.alert()
local remis = avt.load_audio_file(avt.search(sound.remis)) or avt.alert()
local full = avt.load_audio_file(avt.search(sound.full)) or avt.alert()


local function show_keys()
  screen:color(color.text)
  screen:textalign("left", "top")
  screen:moveto(15,10)
  screen:text(L"keys:")
  screen:moverel(0,fheight)
  screen:text("1-7  ← →")
  screen:moverel(0,fheight)
  screen:text("Enter ↓ ")
end


local function show_score()
  screen:eraser()
  screen:bar(1, boardyoffset, boardxoffset - 10, height)
  screen:color(color.chip[1])
  screen:disc(fheight/2, fheight, height/2 - fheight)
  screen:color(color.chip[2])
  screen:disc(fheight/2, fheight, height/2 + fheight)
  screen:color(color.text)
  screen:textalign("left", "center")
  screen:text(score[1], fheight*2, height/2 - fheight)
  screen:text(score[2], fheight*2, height/2 + fheight)
end


-- get center position for column, row
local function get_position(col, row)
  return boardxoffset + (col-1)*fieldsize + fieldsize/2,
         (7-row)*fieldsize + fieldsize/2
end


-- show chip in that position - row 7 is above the board
local function chip_position(col, row)
  if col >= 1 and col <= 7 and row >= 1 and row <= 7 then
    screen:color(color.chip[player])
    screen:disc(radius, get_position(col, row))
  end
end


local function clear_position(col, row)
  if col >= 1 and col <= 7 and row >= 1 and row <= 7 then
    screen:eraser()
    screen:disc(radius, get_position(col, row))
  end
end


-- show chip of player above the board
local function above(column)
  screen:eraser()
  screen:bar(boardxoffset, 1, boardxoffset + boardwidth, fieldsize)
  chip_position(column, 7)
end


-- player drops in that column
local function drop(column)
  local number = filled[column]

  if number < 6 then
    number = number + 1
    for i=6,number,-1 do
      clear_position(column, i+1)
      chip_position(column, i)
      screen:show()
      avt.wait(0.025)
    end
    chips = chips + 1
    filled[column] = number
    board[column][number] = player
    return true
  else -- column full
    full()
    return false
  end
end


local function clear_board()
  for row=1,6 do
    for col=1,7 do
      clear_position(col, row)
      filled[col] = 0
      board[col] = {}
      end
    end

  chips = 0
end


local function draw_board()
  screen:clear()

  screen:color(color.board)
  screen:bar(boardxoffset - 10, boardyoffset,
             boardxoffset + boardwidth + 10,
             height)

  clear_board()

  -- show numbers
  screen:color(color.numbers)
  screen:textalign("left", "top")
  screen:moveto(boardxoffset + 8, boardyoffset)
  for col=1,7 do
    screen:text(col)
    screen:moverel(fieldsize, 0)
  end

  show_keys()
  show_score()

  screen:put(logo, width - logo:width() - 5, 5)
end -- draw_board


-- check whether there are 4 in a row for last player
local function check(column)
  local row = filled[column]
  local num

  local function won(c, r)
    if player ~= board[c][r] then
      num=0 -- not same player
    else
      num = num+1
      if 1==num then --> possible start of success-row
        screen:moveto(get_position(c, r))
      elseif 4==num then --> success
        success() --> play sound
        screen:color(color.connector)
        screen:thickness(4)
        screen:disc(10)
        screen:lineto(get_position(c, r))
        screen:disc(10)
        score[player] = score[player] + 1
        show_score()
        return true
      end
    end
    return false
  end -- won

  -- check for vertical line
  num=0
  for r=1,6 do
    if won(column, r) then return true end
  end

  -- check for horizontal line
  num=0
  for c=1,7 do
    if won(c, row) then return true end
  end

  -- check for ascending diagonal line
  num=0
  local r = row - column
  for c=1,7 do
    r = r + 1
    if won(c, r) then return true end
  end

  -- check for descending diagonal line
  num=0
  local r = row + column
  for c=1,7 do
    r = r - 1
    if won(c, r) then return true end
  end

  return false
end


local function next_player()
  if player==1 then player=2 else player=1 end
end


local function play()
  local won = false
  local column = 4

  local function select_slot()
    local key

    repeat
      above(column, player)
      screen:show()
      key=avt.get_key()
      if avt.key.left==key and column>1 then column = column - 1
      elseif avt.key.right==key and column<7 then column = column + 1
      elseif key>=0x31 and key<=0x37 then column = key - 0x30
      end
    until avt.key.down==key or avt.key.enter==key
  end -- select_slot

  draw_board()

  repeat
    select_slot()
    if drop(column, player) then
      won=check(column, player)
      if not won then next_player() end
    end
  until won or chips == 42

  if not won then remis() end

  screen:show()
  avt.get_key()
end


repeat play() until false
