#!/usr/bin/env lua-akfavatar

require "lua-akfavatar"

avt.move_in()

if arg[1]
  then io.input(arg[1])
  else io.input(avt.file_selection())
  end

avt.pager(io.read("*all"))
avt.move_out()
avt.quit()

