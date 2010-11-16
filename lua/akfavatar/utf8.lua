--[[-------------------------------------------------------------------
Lua module for handling UTF-8 strings
Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>

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

local u8 = {}
utf8 = u8


-- counts number of characters in an UTF-8 encoded string
-- control characters and invisible characters are included, too
function u8.len (s)
  local len = 0

  for i=1,#s do
    local b = string.byte(s, i)
    -- count any byte except follow-up bytes
    if b<128 or b>191 then len = len + 1 end
  end

  return len
end


-- like string.char but accepts higher numbers
-- and returns an UTF-8 encoded string
function u8.char (...)
  local r = ""

  for i,c in ipairs(arg) do
    if c < 0 or c > 0x10FFFF
        or (c >= 0xD800 and c <= 0xDFFF) then
      r = r .. "\239\191\189" --> inverted question mark
    elseif c <= 0x7F then
      r = r .. string.char (c)
    elseif c <= 0x7FF then
      r = r .. string.char (0xC0 + math.floor(c/0x40),
                            0x80 + c%0x40)
    elseif c <= 0xFFFF then
      r = r .. string.char (0xE0 + math.floor(c/0x1000),
                            0x80 + math.floor(c/0x40)%0x40,
                            0x80 + c%0x40)
    else --> c > 0xFFFF and c <= 0x10FFFF
      r = r .. string.char (0xF0 + math.floor(c/0x40000),
                            0x80 + math.floor(c/0x1000)%0x40,
                            0x80 + math.floor(c/0x40)%0x40,
                            0x80 + c%0x40)
    end
  end

  return r
end


-- return the codepoint of the given character (first character)
-- returns nil on error (but that's not a real validity check)
function u8.codepoint (c)
  local b1, b2, b3, b4 = string.byte(c, 1, 4)
  if not b1 then return nil
  elseif b1 <= 0x7F then --> ASCII
    return b1
  elseif b1 <= 0xDF then --> 2 Byte sequence
    return (b1 - 0xC0) * 0x40 + (b2 - 0x80)
  elseif b1 <= 0xEF then --> 3 Byte sequence
    return (b1 - 0xE0) * 0x1000 + (b2 - 0x80) * 0x40 + (b3 - 0x80)
  elseif b1 <= 0xF4 then --> 4 Byte sequence
    return (b1 - 0xF0) * 0x40000 + (b2 - 0x80) * 0x1000
           + (b3 - 0x80) * 0x40 + (b4 - 0x80)
  else --> invalid
    return nil
  end
end


-- iterator returning the characters of an UTF-8 string
-- a character may be a single or multi-byte string
function u8.characters (s)
  local pos = 1

  return function ()
   local b = string.byte(s, pos)
   if b then
     if b <= 0x7F then --> ASCII
       pos = pos + 1
       return string.char(b)
     else --> multibyte character
     local ch = ""
     repeat
       ch = ch .. string.char(b)
       pos = pos + 1
       b = string.byte(s, pos)
     until b == nil or b < 128 or b > 191
     return ch
     end
   end
 end
end


-- like string.reverse, but for UTF-8 encoded strings
function u8.reverse (s)
  local r = ""
  local i = 1
  local len = #s

  repeat
    local b = string.byte(s, i)
    if b < 128 then  --> ASCII
      r = string.char(b) .. r
      i = i + 1
    else --> multibyte character
      local ch = ""
      repeat
        ch = ch .. string.char(b)
        i = i + 1
        b = string.byte(s, i)
      until b == nil or b < 128 or b > 191
      r = ch .. r
    end
  until i > len

  return r
end


-- string.rep works flawlessly with UTF-8, too
u8.rep = string.rep


-- replaces nummerical XML-entities with UTF8-characters
-- named entities or tags are not changed
function u8.xml (s)
  s = string.gsub(s, "&#[xX](%x+);",
    function (c) return u8.char(tonumber(c, 16)) end)
  s = string.gsub(s, "&#(%d+);",
    function (c) return u8.char(tonumber(c, 10)) end)
  return s
end


-- Byte Order Mark
-- not really needed for UTF8, but sometimes used as signature
u8.bom = "\239\187\191"  --> = u8.char(0xFEFF)


-- check, if s starts with a UTF8-BOM
function u8.check_bom (s)
  if string.find(s, "^\239\187\191") then return true else return false end
end


-- check if s is a UTF-8 string
-- it's just for checking if it is UTF-8 or not, not a validity check
-- note: plain ASCII is also valid UTF-8
function u8.check (s)
  local b1, b2, b3, b4, b5
  local limit = 30 --> how many multibyte characters to check
  local counter = 0

  -- s2 is one or more multibyte characters
  for s2 in string.gmatch (s, "[\128-\255]+") do
    local start = 1

    repeat
      b1, b2, b3, b4, b5 = string.byte(s2, start, start + 4)
      counter = counter + 1

      -- b1 must be a start byte
      if b1 < 0xC2 or b1 > 0xF4 then return false

      -- check 2 byte sequence
      elseif (b1 >= 0xC2 and b1 <= 0xDF)
         and (b2 == nil or b2 < 0x80 or b2 > 0xBF
           or (b3 ~= nil and b3 >= 0x80 and b3 <= 0xBF)) then return false

      -- check 3 byte sequence
      elseif (b1 >= 0xE0 and b1 <= 0xEF)
         and (b2 == nil or b2 < 0x80 or b2 > 0xBF
           or b3 == nil or b3 < 0x80 or b3 > 0xBF
           or (b4 ~= nil and b4 >= 0x80 and b4 <= 0xBF)) then return false

      -- check 4 byte sequence
      elseif (b1 >= 0xF0 and b1 <= 0xF4)
         and (b2 == nil or b2 < 0x80 or b2 > 0xBF
           or b3 == nil or b3 < 0x80 or b3 > 0xBF
           or b4 == nil or b4 < 0x80 or b4 > 0xBF
           or (b5 ~= nil and b5 >= 0x80 and b5 <= 0xBF)) then  return false
      end

      -- check for another character
      if b1 >= 0xC2 and b1 <= 0xDF and b3 ~= nil then start = start + 2
      elseif b1 >= 0xE0 and b1 <= 0xEF and b4 ~= nil then start = start + 3
      elseif b1 >= 0xF0 and b1 <= 0xF4 and b5 ~= nil then start = start + 4
      else start = 0 end

    until start == 0 or counter >= limit

    if counter >= limit then break end
  end

  return true --> nothing invalid found
end

return u8
