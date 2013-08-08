#!/usr/bin/env lua-akfavatar

--[[---------------------------------------------------------------------
Four in a Row
Game for 1 or 2 players

Copyright (c) 2011,2012,2013 Andreas K. Foerster <info@akfoerster.de>

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

Mouse supported for AkFAvatar-9.24.2 or higher
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
  human_wins = "hey.au",
  computer_wins = "hahaha.au",
  remis = "question.au",
  full = "harrumph.au"
  }

avt.translations = {
  -- avoid trademarked names
  ["Four in a row"] = {
    de = "Vier in einer Reihe"},

  ["1 Player"] = {
    de = "1 Spieler"},

  ["2 Players"] = {
    de = "2 Spieler"},

  ["keys:"] = {
    de = "Tasten:"}
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
local players = 2
local player = 1
local who_starts = 1
local slow = false
local mouse = 0xE800

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
local holes = {}

local human_wins = avt.load_audio_file(avt.search(sound.human_wins)) or avt.alert()
local computer_wins = avt.alert() -- loaded only when needed
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
    if slow then
      chip_position(column, number)
    else
      for i=6,number,-1 do
        clear_position(column, i+1)
        chip_position(column, i)
        screen:show()
        avt.wait(0.025)
      end
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
      holes[col] = {}
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


local function show_winning_row(column, row)
  -- play sound
  if players==2 or player==1 then human_wins() else computer_wins() end

  -- draw connector - pen position assumed to be at start
  screen:color(color.connector)
  screen:thickness(4)
  screen:disc(10)
  screen:lineto(get_position(column, row))
  screen:disc(10)

  score[player] = score[player] + 1
  show_score()
end


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
        show_winning_row(c, r)
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
  r = row + column
  for c=1,7 do
    r = r - 1
    if won(c, r) then return true end
  end

  return false
end


local function next_player(player)
  if player==1 then return 2 else return 1 end
end


-- check speed of display
local function speed_test()
  local delay = avt.ticks()
  screen:show ()
  if avt.ticks() - delay > 100 then slow = true end
end


local function select_slot(column)
  local key

  repeat
    above(column, player)
    screen:show()
    key=avt.get_key()
    if avt.key.left==key and column>1 then column = column - 1
    elseif avt.key.right==key and column<7 then column = column + 1
    elseif key>=0x31 and key<=0x37 then column = key - 0x30
    elseif mouse==key then
      local x = graphic.get_pointer_position() - boardxoffset
      if x > 0 and x < 7*fieldsize then
        column = math.ceil(x / fieldsize)
        above(column, player)
        key = avt.key.down
      end
    end
  until avt.key.down==key or avt.key.enter==key

 return column
end -- select_slot


-- analyze line of chips
local function analyze(line)
  local l = {}
  local result = {}

  -- turn line into a string
  for p=1,7 do
    if not line[p] then
      l[p] = "." -- empty
    elseif line[p] == player then
      l[p] = "P" -- Player
    else
      l[p] = "O" -- Oponent
    end
  end

  local str = table.concat(l)

  -- now do some pattern matching on the string
  local pos

  pos = string.find(str, "P.PP.P", 1, true)
  if pos then
    result[pos+1] = player
    result[pos+4] = player
    return result
  end

  pos = string.find(str, ".PPP.", 1, true)
  if pos then
    result[pos] = player
    result[pos+4] = player
    return result
  end

  pos = string.find(str, "PPP.", 1, true)
  if pos then
    result[pos+3] = player
    return result
  end

  pos = string.find(str, ".PPP", 1, true)
  if pos then
    result[pos] = player
    return result
  end

  pos = string.find(str, "P.PP", 1, true)
  if pos then
    result[pos+1] = player
    return result
  end

  pos = string.find(str, "PP.P", 1, true)
  if pos then
    result[pos+2] = player
    return result
  end

  return result
end


local function mark_hole(column, row, pl)
  if pl and column>0 and row>0 and column<=7 and row<=6 then
    local h = holes[column][row]
    if h and h ~= pl then
      holes[column][row] = 3 -- hole for both players
    else
      holes[column][row] = pl
    end
  end
end


local function seek_hole(column)
  local row = filled[column]
  local line -- line to analyze

  -- check for horizontal line
  line = {}
  for c=1,7 do line[c] = board[c][row] end
  line = analyze(line)
  for c=1,7 do mark_hole(c, row, line[c]) end

  -- check for vertical line
  line = {}
  for r=1,6 do line[r] = board[column][r] end
  line = analyze(line)
  for r=1,6 do mark_hole(column, r, line[r]) end

  -- check for ascending diagonal line
  line = {}
  local r = row - column
  for c=1,7 do r=r+1; line[c] = board[c][r] end
  line = analyze(line)
  r = row - column
  for c=1,7 do r=r+1; mark_hole(c, r, line[c]) end

  -- check for descending diagonal line
  line = {}
  r = row + column
  for c=1,7 do r = r - 1; line[c] = board[c][r] end
  line = analyze(line)
  r = row + column
  for c=1,7 do r = r - 1; mark_hole(c, r, line[c]) end
end


-- computer logic
local function compute(column)

  if chips==0 then
    return 4
  elseif chips==1 then -- one chip -> go next to it
    for c=1,6 do
      if board[c][1] then return (c+1) end
    end
  end

  -- look if I can win
  for c=1,7 do
    local f = holes[c][filled[c]+1]
    if f==player or f==3 then return c end
  end

  -- close direct threats
  for c=1,7 do
    if holes[c][filled[c]+1] then return c end
  end

  -- seek field that is not indirectly threatend
  -- avoid current column
  for i, c in ipairs{4,3,5,2,6,1,7} do
    if c~=column and filled[c]<6 and not holes[c][filled[c]+2] then
      return c
    end
  end

  -- if column is not indirectly threatend, use that
  if filled[column]<6 and not holes[column][filled[column]+2] then
    return column
  end

  -- all are indirectly threatened => give up an own threat
  for i, c in ipairs{4,3,5,2,6,1,7} do
    if holes[c][filled[c]+2]==player then
      return c
    end
  end

  -- last resort: find any free column - game lost
  for c=1,7 do
    if filled[c] < 6 then return c end
  end

  return column -- should never be reached
end


local function play()
  local won = false
  local column = 4

  draw_board()
  speed_test()

  player = who_starts

  repeat
    if 2==players or 1==player
      then column = select_slot(column)
      else column = compute(column)
    end

    if drop(column, player) then
      won=check(column)
      if not won then
        if 1==players then seek_hole(column) end
        player = next_player(player)
      end
    end
  until won or chips == 42

  if not won then remis() end

  who_starts = next_player(who_starts)

  screen:show()

  avt.wait_audio_end()
  avt.clear_keys()
  avt.get_key()
end


avt.tell(L"Four in a row", "\n",
         L"1 Player", "\n",
         L"2 Players")

players = avt.choice(2, 2, "1")

if players==1 then
  computer_wins = avt.load_audio_file(avt.search(sound.computer_wins))
     or avt.alert()
end

if graphic.set_pointer_buttons_key then
  graphic.set_pointer_buttons_key (mouse)
else
  avt.set_mouse_visible(false)
end

repeat play() until false
