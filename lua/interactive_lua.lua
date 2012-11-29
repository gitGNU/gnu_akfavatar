#!/usr/bin/env lua-akfavatar

-- based on code from the Lua wiki - author unknown

avt = require "lua-akfavatar"

print = avt.print --> redefine the print command
say = print --> an alias that fits better

local history = {}

local function interactive (cmd)

  local function input (prompt)
    local pos = 0

    avt.save_position ()
    local line, ch = avt.input (prompt, nil, -1, 1)

    while ch == avt.key.up or ch == avt.key.down do
      if ch == avt.key.up then
        if history[pos+1] then
          pos = pos + 1
        end
      elseif ch == avt.key.down then
        if history[pos] then
          pos = pos - 1
        end
      end
      avt.restore_position ()
      line, ch = avt.input (prompt, history[pos], -1, 1)
    end

    if line ~= "" then table.insert (history, 1, line) end

    return line
  end

  local function error_message (...)
    avt.bell ()
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
    -- replace "=" at the beginning with "return "
    cmd = string.gsub (input ("> "), "^=", "return ", 1)
  else
    local cmd2 = input (">> ")
    if cmd2 == "" then return "" end --> empty line cancels command
    cmd = cmd .. " " .. cmd2
  end

  local func, err = load (cmd)

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

avt.encoding("UTF-8")
avt.title("Lua-AKFAvatar")
avt.start()
avt.start_audio()
avt.avatar_image_file(avt.search("computer.xpm") or "default")
avt.set_avatar_mode("footer")

-- avt.move_in ()
avt.underlined(true)
avt.say(_VERSION, " / AKFAvatar ", avt.version (), "\n\n")
avt.normal_text ()

while true do interactive () end
