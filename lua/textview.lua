#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

avt.initialize{title="Text Viewer", encoding="UTF-8"}
avt.move_in()

-- what files to show with file_selection
function textfile(n)
  n = string.lower(n)
  return string.find(n, "%.te?xt$")
         or string.find(n, "^readme")
         or string.find(n, "^changelog$")
         or string.find(n, "^authors$")
         or string.find(n, "^copying")
         or string.find(n, "^news$")
         or string.find(n, "^install$")
         or string.find(n, "%.lua$")
         or string.find(n, "%.cc?$")
         or string.find(n, "%.hh?$")
         or string.find(n, "%.c++$")
         or string.find(n, "%.cpp?$")
         or string.find(n, "%.cxx$")
         or string.find(n, "%.pas$")
         or string.find(n, "%.p[pl]?$")
         or string.find(n, "%.s$")
end

if arg[1]
  then io.input(arg[1])
  else io.input(avt.file_selection(textfile))
  end

avt.pager(io.read("*all"))
avt.move_out()

