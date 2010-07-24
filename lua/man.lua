#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

-- we don't know yet, which avatar to use - "none" is fastest
avt.initialize{title="Manpage", avatar="none", encoding="ISO-8859-1"}

function underlined(text)
  return string.gsub(text, ".", "_\b%1")
end

function ask()
  avt.change_avatar_image("default")
  --avt.move_in()
  avt.set_balloon_size(3, 40)
  avt.say("man [", underlined("Option"), " ...] [",
          underlined("Section"), "] ",
          underlined("Page"), " ...\n\nman ")
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

avt.change_avatar_image(require "akfavatar.info_icon")
avt.set_balloon_size(0, 0)
avt.pager(manpage:read("*all"))
manpage:close()


