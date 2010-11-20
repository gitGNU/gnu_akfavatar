#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.utf8"

local default_encoding = "UTF-8"

avt.initialize{title="Text Viewer", encoding=default_encoding}

-- what files to show with file_selection
function textfile(n)
  n = string.lower(n)
  return string.find(n, "%.te?xt$")
         or string.find(n, "%.info$")
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
  else io.input(avt.file_selection(textfile))
  end

local text = io.read("*all")

-- note: WINDOWS-1252 is a superset of ISO-8859-1
local text_encoding = utf8.check_unicode(text) or "WINDOWS-1252"

if text_encoding ~= default_encoding then avt.encoding(text_encoding) end

avt.pager(text)
