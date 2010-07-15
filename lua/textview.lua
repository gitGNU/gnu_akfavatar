#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

avt.initialize{title="Text Viewer", encoding="UTF-8"}
avt.move_in()

-- what files to show with file_selection
function textfile(n)
  n = string.lower(n)
  return string.match(n, "%.txt$")
         or string.match(n, "%.text$")
         or string.match(n, "^readme")
         or string.match(n, "^changelog$")
         or string.match(n, "^authors$")
         or string.match(n, "^copying")
         or string.match(n, "^news$")
         or string.match(n, "^install$")
         or string.match(n, "%.lua$")
         or string.match(n, "%.c$")
         or string.match(n, "%.cpp$")
         or string.match(n, "%.pas$")
end

if arg[1]
  then io.input(arg[1])
  else io.input(avt.file_selection(textfile))
  end

avt.pager(io.read("*all"))
avt.move_out()

