#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

print = avt.print --> define the print command
say = print --> an alias that fits better

local function interactive (cmd)

  local function error_message (...)
    avt.set_text_color ("dark red")
    avt.print (...)
    avt.normal_text ()
  end

  local function show (success, ...)
    if select ("#", ...) > 0 then
      if success
        then avt.print (...)
        else error_message (...)
      end --> if success
    end --> if select
  end --> function show

  if avt.where_x () > 1 then avt.newline () end

  if not cmd then --> first line
    -- replace = at the beginning with return
    cmd = string.gsub (avt.ask ("> "), "^=", "return ", 1)
  else
    cmd = cmd .. " " .. avt.ask (">> ")
  end

  local func, err = loadstring (cmd)

  if func then show (pcall(func))
    else --> error
      -- '<eof>' at the end means the command is incomplete
      if (string.find(err, "'<eof>'$"))
        then return interactive (cmd)
        else error_message (err)
      end
  end

  return cmd
end

avt.initialize {title="Lua-AKFAvatar", encoding="UTF-8", audio=true}
avt.move_in ()
avt.underlined(true)
avt.say(_VERSION, " / AKFAvatar ", avt.version (), "\n\n")
avt.normal_text ()

while true do interactive () end
