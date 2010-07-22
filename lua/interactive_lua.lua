#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

print = avt.print --> define the print command
say = print --> an alias that fits better

local function interactive (cmd)
  local function say_line (s) avt.say (s); avt.newline () end

  if not cmd then avt.say ("> ") else avt.say (">> ") end
  local line = avt.ask ()

  if not cmd then --> first line
    -- replace = at the beginning with return
    cmd = string.gsub (line, "^=", "return ", 1)
  else
    cmd = cmd .. " " .. line
  end

  local func, err = loadstring (cmd)

  if func == nil
    then
      -- '<eof>' at the end means the command is incomplete
      if (string.find(err, "'<eof>'$"))
        then return interactive (cmd)
        else say_line (err) --> say error mesage
      end
    else --> complete function
      local ret = {pcall(func)}
      local success = table.remove(ret, 1)
      if not success --> error calling the function?
        then say_line (ret[1]) --> error msg
        else if ret[1] ~= nil then say_line (unpack (ret)) end
      end
  end

  return cmd
end

avt.initialize {title="Lua-AKFAvatar", encoding="UTF-8", audio=true}
avt.move_in ()
avt.say(_VERSION, " / AKFAvatar ", avt.version (), "\n\n")

while true do interactive () end
