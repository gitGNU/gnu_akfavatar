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
  left/right - choose column
  down - drop chip
  space - clear board
--]]--------------------------------------------------------------------

local avt = require "lua-akfavatar"
local graphic = require "akfavatar-graphic"

avt.translations = {
  ["keys:"] = {
    de = "Tasten:",
  }
}

local L = avt.translate

avt.encoding("UTF-8")
avt.title("Four in a row")
avt.start()
avt.start_audio()

local board_color = "saddle brown"
local chip = {[1] = "white", [2] = "black"}
local connect_color = "green"
local success = avt.load_audio_file(avt.search "hahaha.au") or avt.silent()
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


local function show_keys()
  screen:color("black")
  screen:textalign("left", "top")
  screen:text(L"keys:", 15, 10)
  screen:text("← →", 20, 25)
  screen:text(" ↓ ", 20, 33)
end


local function show_score()
  screen:eraser()
  screen:bar(1, 50, boardxoffset - 10, height)
  screen:color(chip[1])
  screen:disc(fheight/2, fheight, height/2 - fheight)
  screen:color(chip[2])
  screen:disc(fheight/2, fheight, height/2 + fheight)
  screen:color("black")
  screen:textalign("left", "center")
  screen:text(score[1], fheight*2, height/2 - fheight)
  screen:text(score[2], fheight*2, height/2 + fheight)
  screen:show()
end


-- get center position for column, row
local function get_position(col, row)
  return boardxoffset + (col-1)*fieldsize + fieldsize/2,
         (7-row)*fieldsize + fieldsize/2
end

-- show chip in that position - row 7 is above the board
-- if color is not given it clears the field
local function position(col, row, color)
  if col >= 1 and col <= 7 and row >= 1 and row <= 7 then
    if color then screen:color(color) else screen:eraser() end
    screen:disc(radius, boardxoffset + (col-1)*fieldsize + fieldsize/2,
                (7-row)*fieldsize + fieldsize/2)
  end
end


-- show chip of player above the board
local function above(column)
  screen:eraser()
  screen:bar(boardxoffset, 1, boardxoffset + boardwidth, fieldsize)
  position(column, 7, chip[player])
end


-- player drops in that column
local function drop(column)
  local number = filled[column]

  if number < 6 then
    number = number + 1
    for i=6,number,-1 do
      position(column, i+1) -- clear
      position(column, i, chip[player])
      screen:show()
      avt.wait(0.025)
    end
    chips = chips + 1
    filled[column] = number
    board[column][number] = player
    return true
  else -- column full
    avt.bell()
    return false
  end
end


local function clear_board()
  for row=1,6 do
    for col=1,7 do
      position(col, row)
      filled[col] = 0
      board[col] = {}
      end
    end

  chips = 0
end


-- check whether there are 4 in a row for last player
local function check(column)
  local row = filled[column]
  local num

  local function won(c, r)
    if player ~= board[c][r] then
      num=0
    else
      num = num+1
      if 1==num then --> start of success-row???
        screen:moveto(get_position(c, r))
      elseif 4==num then --> success
        success:play()
        screen:thickness(4)
        screen:color(connect_color)
        screen:lineto(get_position(c, r))
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


local function play()
  local won = false
  local column = 4

  local function select()
    local key
    local left, right, down = 0xF003, 0xF002, 0xF001
    local new = 32

    repeat
      above(column, player)
      screen:show()
      key=avt.get_key()
      if left==key and column>1 then column = column - 1
      elseif right==key and column < 7 then column = column + 1
      elseif new==key then clear_board()
      end
    until down==key
  end -- select

  local function next_player()
    if player==1 then player=2 else player=1 end
  end

  -- draw board
  screen:clear()
  screen:color(board_color)
  screen:bar(boardxoffset - 10, boardyoffset,
             boardxoffset + boardwidth + 10, height)
  clear_board()
  show_keys()
  show_score()

  repeat
    select()
    if drop(column, player) then
      won=check(column, player)
      if not won then next_player() end
    end
  until won or chips == 42

  avt.get_key()
end

repeat play() until false
