#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

local avt = require "lua-akfavatar"

-- all is done in memory and it needs time, so we have to set a limit
local limit = 0x100000  --> 1MB

avt.translations = {
  ["Hex Viewer"] = {
    de = "Hex-Betrachter"},

  ["please wait..."] = {
    de = "Bitte warten..."},

  ["size limit reached!"] = {
    de = "Größenbeschränkung erreicht!"}
}

local L = avt.translate

avt.encoding("UTF-8")
avt.title(L"Hex Viewer")
avt.start()
avt.avatar_image("none")
avt.set_balloon_color("tan")

local filename = arg[1]

if not filename then
  avt.say(L"Hex Viewer")
  avt.set_directory(os.getenv("HOME") or os.getenv("USERPROFILE"))
  filename = avt.file_selection()
  if not filename or filename=="" then return end
end

avt.tell(L"please wait...")

local f = assert(io.open(filename, "rb"))
local data = f:read(limit)
f:close()


local function line(offset)
  local l, b

  local center = offset+8

  l = string.format("%05X \xE2\x95\x91", offset)

  for position = offset+1, offset+16 do
    b = string.byte(data, position)
    if not b then break end
    l = l .. string.format(" %02X", b)
    if position == center then l = l .. " \xE2\x94\x82" end
  end

  -- hack to count actual utf-8 characters
  local _, len = string.gsub(l, "[^\128-\191]", "")

  -- eventually fill space
  if len < 57 then l = l .. string.rep(" ", 57 - len) end

  l = l .. " \xE2\x95\x91 "
      .. string.gsub(string.sub(data, offset+1, offset+16),
                     "[\1-\31\127-\255]", "\0")

  return l
end


local len = #data
local offset = 0
local lines = {filename}

repeat
  table.insert(lines, line(offset))
  avt.update()
  offset = offset + 16
until offset >= len

if len >= limit then table.insert(lines, L"size limit reached!") end

-- reusing the same variable sets the table free for garbage collection
lines = table.concat(lines, "\n")

avt.set_balloon_size(0, 76)
avt.pager(lines)
