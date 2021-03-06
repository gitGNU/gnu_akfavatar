#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012,2013
-- Andreas K. Foerster <akf@akfoerster.de>
-- License: GPL version 3 or later

local avt = require "lua-akfavatar"

avt.title("Text Viewer")
avt.start()
avt.avatar_image("none")
avt.set_balloon_color("tan")

-- what files to show with file_selection
function textfile(n)
  n = string.lower(n)
  return string.find(n, "%.te?xt$")
         or string.find(n, "%.info$")
         or string.find(n, "%.about$")
         or string.find(n, "^readme")
         or string.find(n, "^liesmich")
         or string.find(n, "^changelog$")
         or string.find(n, "^authors$")
         or string.find(n, "^copying")
         or string.find(n, "^o?news$")
         or string.find(n, "^install$")
         or string.find(n, "%.lua$")
         or string.find(n, "%.c?sh$")
         or string.find(n, "%.cc?$")
         or string.find(n, "%.hh?$")
         or string.find(n, "%.c%+%+$")
         or string.find(n, "%.cpp?$")
         or string.find(n, "%.cxx$")
         or string.find(n, "%.pas$")
         or string.find(n, "%.p[ply]?$")
         or string.find(n, "%.s$")
end

if arg[1]
  then io.input(arg[1])
  else
    local file = avt.file_selection(textfile)
    if not file or file=="" then return end
    io.input(file)
  end

local text = io.read("*all")

if avt.detect_utf8(text, 5120) then
  avt.encoding("UTF-8")
else
  avt.encoding("WINDOWS-1252")
end

-- note: WINDOWS-1252 is a superset of ISO-8859-1
-- you should not use WINDOWS-1252 for your own texts, though!

avt.pager(text)
