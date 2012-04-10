#!/usr/bin/env lua-akfavatar

-- manpage viewer for GNU/Linux and FreeBSD
-- (does not work with most other systems)

-- Copyright (c) 2010,2011 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

local avt = require "lua-akfavatar"

avt.encoding("ISO-8859-1")
avt.set_title("Manpage")
avt.start()

function ask()
  avt.markup(true)
  avt.avatar_image("default")
  --avt.move_in()
  avt.set_balloon_size(3, 40)
  avt.say("man [_Option_ ...] [_Section_] _Page_ ...\n\nman ")
  local answer = avt.ask()
  return answer
end


local page

if arg[1]
  then page = table.concat(arg, " ")
  else page = ask()
  end

local manpage = assert(io.popen(
  "env MANWIDTH=80 GROFF_TYPESETTER=latin1 GROFF_NO_SGR=1 man -t "
  .. page .. " 2>&1"))

avt.avatar_image_file(avt.search("info.xpm"))
avt.set_balloon_size(0, 0)
-- manpages use the overstrike technique in the output
avt.markup(false)
avt.pager(manpage:read("*all"))
manpage:close()
