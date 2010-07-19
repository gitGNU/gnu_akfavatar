#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

local prompt = "> "
local cmd, func, err, success

print = function(...)
  local i, v
  for i,v in ipairs({...}) do
    if i > 1 then avt.next_tab() end
    avt.say(tostring(v))
    end
  avt.newline()
end

say = print --> an alias that fits better

avt.initialize{title="Lua-AKFAvatar", encoding="UTF-8", audio=true}
avt.move_in()
avt.say(_VERSION, " / AKFAvatar ", avt.version(), "\n\n")

repeat
  avt.say(prompt)
  cmd = avt.ask()
  cmd = string.gsub(cmd, "^=", "return ", 1)
  func, err = loadstring(cmd)
  if not func
    then print(err)
    else
      local ret = {pcall(func)}
      success = table.remove(ret, 1)
      if not success -- error calling the function?
        then print(ret[1])
        else if ret[1]~=nil then print(unpack(ret)) end
      end
  end
until false
