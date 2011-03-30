--[[-------------------------------------------------------------------
Moldule for making demos for Lua-AKFAvatar
Copyright (c) 2010,2011 Andreas K. Foerster <info@akfoerster.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
--]]-------------------------------------------------------------------

require "lua-akfavatar"
require "akfavatar.ar"
pcall(require, "akfavatar-vorbis")

local audio, old_audio, initialized, moved_in, avatar, avatarname
local ballooncolor, textcolor, title, archive, do_wait, txt

local function initialize()
  avt.initialize {title=title, avatar=avatar, audio=true}
  if avatarname then avt.set_avatar_name(avatarname) end
  if ballooncolor then avt.set_balloon_color(ballooncolor) end
  if textcolor then avt.set_text_color(textcolor) end
  avt.set_text_delay()
  avt.markup(true)
  initialized = true
end

local function wait(s)
  if do_wait then
    avt.wait_audio_end()
    avt.wait(s)
    do_wait = false
    end
end

local function avatar_image(name)
  if not archive or name=="default" or name=="none" then
    avatar = name
  else
    avatar = archive:get(name)
  end

  if initialized then avt.change_avatar_image(avatar) end
end

local function set_avatar_name(name)
  avatarname = name
  if initialized then avt.set_avatar_name(avatarname) end
end

local function set_balloon_color(name)
  ballooncolor = name
  if initialized then avt.set_balloon_color(ballooncolor) end
end

local function set_text_color(name)
  textcolor = name
  if initialized then avt.set_text_color(textcolor) end
end

local function show_text()
  wait()
  if txt then
    if not moved_in then
      avt.move_in() -- move not defined, yet
      moved_in = true
    end
    avt.tell(txt)
    txt = nil
    do_wait = true
  end
end

local function move(move_in)
  if not initialized then initialize() end

  if move_in then
    avt.move_in()
  else
    show_text()
    wait()
    avt.move_out()
  end

  moved_in = move_in
end

local function credits(name)
  if not initialized then initialize() end
  show_text()
  wait()

  if archive then
    avt.credits(assert(archive:get(name)), true)
  else
    local f = assert(io.open(name, "r"))
    avt.credits(f:read("*all"), true)
    f:close()
  end
end

local function load_audio(name)
    old_audio:free()
    old_audio = audio --> keep, it might still be playing!
    if archive then
      audio = avt.load_audio_string(archive:get(name)) or avt.silent()
    else --> not an archive
      audio = avt.load_audio_file(name) or avt.silent()
    end
end


local function show_image(name)
  if not initialized then initialize() end
  show_text()
  wait()

  if archive then
    avt.show_image_string(archive:get(name))
  else
    avt.show_image_file(name);
  end

  avt.wait(7)
  do_wait = false
end

local function command(cmd)
  local c, a = string.match(cmd, "^%[(%S+)%s*(.-)%s*%]")

  if "AKFAvatar"==c then
  -- ignore
  elseif "title"==c then
    title = a
    if initialized then avt.set_title(title) end
  elseif "datadir"==c then
    avt.set_directory(a)
  elseif "flip"==c then
    if not moved_in then move(true) end
    avt.flip_page()
    do_wait = false
  elseif "clear"==c then
    if not moved_in then move(true) end
    avt.clear()
  elseif "wait"==c then
    if not initialized then initialize() end
    show_text()
    do_wait = true
    if a=="" then wait() else wait(tonumber(a)) end
  elseif "effectpause"==c then
    -- ignore - for backward compatibility
  elseif "pause"==c then
    if not moved_in then move(true) end
    show_text()
    wait(2.7); avt_show_avatar(); avt.wait(4)
    do_wait = false
  elseif "image"==c then
    show_image(a)
  elseif "rawaudiosettings"==c then
    --local rate,encoding,channels=string.match(a, "^(%d+)%s+(%S+)%s+(%S+)$")
    error("command no longer supported: rawaudiosettings")
  elseif "audio"==c then
    if not initialized then initialize() end
    load_audio(a)
    avt.wait_audio_end()
    audio:play()
  elseif "audioloop"==c then
    if not initialized then initialize() end
    load_audio(a)
    audio:loop()
  elseif "loadaudio"==c then
    load_audio(a) -- deprecated
  elseif "playaudio"==c then
    if not initialized then initialize() end
    avt.wait_audio_end()
    audio:play()
  elseif "playaudioloop"==c then  -- deprecated
    if not initialized then initialize() end
    avt.wait_audio_end()
    audio:loop()
  elseif "stopaudio"==c then
    if not initialized then initialize() end
    avt.stop_audio()
  elseif "waitaudio"==c then
    show_text()
    avt.wait_audio_end()
  elseif "move"==c then
    move("in"==a)
  elseif "size"==c then
    -- ignore - for backward compatibility
  elseif "height"==c then
    -- ignore - for backward compatibility
  elseif "width"==c then
    -- ignore - for backward compatibility
  elseif "encoding"==c then
    avt.encoding(a)
  elseif "backgroundcolor"==c then
     avt.set_background_color(a)
  elseif "ballooncolor"==c then
     set_balloon_color(a)
  elseif "textcolor"==c then
     set_text_color(a)
  elseif "avatarimage"==c then
     avatar_image(a)
  elseif "avatarname"==c then
     set_avatar_name(a)
  elseif "scrolling"==c then
     if "on"==a then avt.set_scroll_mode(1)
     elseif "off"==a then avt.set_scroll_mode(-1)
     end
  elseif "slow"==c then
     avt.set_text_delay(a~="off")
  elseif "back"==c then
    -- just for backward compatibility!
    local value, line = string.match(a, "^(%d+)%s+(.+)$")
    if txt then txt = string.sub(txt, 1, -value-1) .. line end
  elseif "right-to-left"==c then
    avt.right_to_left(true)
  elseif "left-to-right"==c then
    avt.right_to_left(false)
  elseif "credits"==c then
     credits(a)
  elseif "stop"==c then
     return true
  elseif "end"==c then
    if not initialized then initialize() end
     avt.wait(); avt.move_out(); avt.wait()
     do_wait = false
     return true
  else
    error("unknown command: " .. cmd)
  end

  return false --> no end, yet
end

local function get_script(demofile)
  local script

  local f = assert(io.open(demofile, "r"))

  -- check if it's an archive
  if f:read("*line") == "!<arch>" then
    -- the skript has to be the first entry
    if not string.find(f:read("*line"), "^AKFAvatar%-demo") then
      f:close()
      error "not an AKFAvatar archive"
    end
    f:close()
    archive = assert(ar:open(demofile, "r"))
    script = archive:get("AKFAvatar-demo")
  else -- not an archive
    f:seek("set") --> rewind to start
    script = f:read("*all")
    f:close()

    -- eventually change the working directory
    if string.find(demofile, "[\\/]") then
      avt.set_directory (string.match(demofile, "^(.+)[\\/]"))
    end
  end

  return script
end

function avtdemo(demofile)
  if not demofile then return end

  -- reset settings
  audio = avt.silent()
  old_audio = audio
  initialized = false
  moved_in = false
  avatar = "default"
  title = "AKFAvatar-Demo"
  archive = false
  do_wait = false

  for line in string.gmatch(get_script(demofile), "(.-)\r?\n") do
    if string.find(line, "^%s*#") or
       (not initialized and string.find(line, "^%s*$")) then
      -- ignore
    elseif string.find(line, "^%-%-%-") then
      if not moved_in then move(true) end
      show_text()
      wait()
    elseif string.find(line, "^%[") then
      if command(line) then break end
    else -- not a comment nor a command
      if not initialized then initialize() end
      if txt then txt = txt .. "\n" .. line else txt = line end
    end
  end

  if archive then
    archive:close()
    archive = false
  end

  show_text()
  wait()
  avt.move_out()
end

return avtdemo
