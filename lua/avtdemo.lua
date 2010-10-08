#!/usr/bin/env lua-akfavatar

--[[-------------------------------------------------------------------
Making demos for Lua-AKFAvatar
Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>

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

local audio = avt.load_audio_string() --> empty audio infrastructure
local old_audio = audio
local initialized = false
local moved_in = false
local avatar = "default"
local title = "AKFAvatar-Demo"
local archive = false

local function initialize()
  avt.initialize {title=title, avatar=avatar, audio=true}
  avt.set_text_delay()
  avt.markup(true)
  initialized = true
end

local function avatar_image(name)
  if not archive or name=="default" or name=="none" then
    avatar = name
  else
    avatar = archive:get(name)
  end

  if initialized then avt.change_avatar_image(avatar) end
end

local function move(move_in)
  if not initialized then initialize() end
  if move_in then avt.move_in() else avt.move_out() end
  moved_in = move_in
end

local function credits(name)
  if not initialized then initialize() end
  avt.wait()
  
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
      audio = avt.load_audio_string(archive:get(name))
    else
      audio = avt.load_audio_file(name)
    end
end

local function show_image(name)
  if not initialized then initialize() else avt.wait() end

  if archive then
    avt.show_image_string(archive:get(name))
  else
    avt.show_image_file(name);
  end

  avt.wait(7)
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
  elseif "clear"==c then
    if not moved_in then move(true) end
    avt.clear()
  elseif "wait"==c then
    if not initialized then initialize() end
    if a=="" then avt.wait() else avt.wait(a) end
  elseif "effectpause"==c then
    if not moved_in then move(true) end
    avt.wait(2.7)
  elseif "pause"==c then
    if not moved_in then move(true) end
    avt.wait(2.7); avt_show_avatar(); avt.wait(4)
  elseif "image"==c then
    show_image(a)
  elseif "rawaudiosettings"==c then
    local rate,encoding,channels=string.match(a, "^(%d+)%s+(%S+)%s+(%S+)$")
    -- TODO
  elseif "audio"==c then
    if not initialized then initialize() end
    load_audio(a)
    audio:play()
  elseif "audioloop"==c then
    if not initialized then initialize() end
    load_audio(a)
    audio:loop()
  elseif "loadaudio"==c then
    load_audio(a)
  elseif "playaudio"==c then
    if not initialized then initialize() end
    audio:play()
  elseif "playaudioloop"==c then
    if not initialized then initialize() end
    audio:loop()
  elseif "stopaudio"==c then
    if not initialized then initialize() end
    avt.stop_audio()
  elseif "waitaudio"==c then
    avt.wait_audio_end()
  elseif "move"==c then
    move("in"==a)
  elseif "size"==c then
    if not moved_in then move(true) end
    avt.set_balloon_size(string.match(a, "^(%d+)%s*,%s*(%d+)$"))
  elseif "height"==c then
    if not moved_in then move(true) end
    avt.set_balloon_height(a)
  elseif "width"==c then
    if not moved_in then move(true) end
    avt.set_balloon_width(a)
  elseif "encoding"==c then
    avt.encoding(a)
  elseif "backgroundcolor"==c then
     avt.set_background_color(a)
  elseif "ballooncolor"==c then
     avt.set_balloon_color(a)
  elseif "textcolor"==c then
     avt.set_text_color(a)
  elseif "avatarimage"==c then
     avatar_image(a)
  elseif "avatarname"==c then
     avt.set_avatar_name(a)
  elseif "scrolling"==c then
     if "on"==a then avt.set_scroll_mode(1)
     elseif "off"==a then avt.set_scroll_mode(-1)
     end
  elseif "slow"==c then
     avt.set_text_delay(a~="off")
  elseif "back"==c then
    if not initialized then initialize() end
    local value, line = string.match(a, "^(%d+)%s+(.+)$")
    for i=1,value do avt.backspace() end
    avt.say(line)
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
  end

  return script
end

local function avtdemo(demofile)
  local empty = true

  if not demofile then return end

  for line in string.gmatch(get_script(demofile), "(.-)\r?\n") do
    if string.find(line, "^%s*#") or 
       (not initialized and string.find(line, "^%s*$")) then
      -- ignore
    elseif string.find(line, "^%-%-%-") then
      if moved_in then avt.flip_page() else move(true) end
      empty = true
    elseif string.find(line, "^%[") then
      if command(line) then break end
    else -- not a comment nor a command
      if not moved_in then move(true) end
      if not empty then avt.newline() end
      avt.say(line)
      empty = false
    end
  end

  if archive then
    archive:close()
    archive = false
  end

  avt.wait(2.7)
  avt.move_out()
end


if arg[1]
  then
    -- split directory and filename
    -- set the working directory, for required files could be there
    local dir, name = string.match(arg[1], "^(.-)([^/\\]+)$")
    avt.set_directory(dir)
    avtdemo(name)
  else
    avtdemo(
      avt.file_selection(
          function(n)
            return string.find(string.lower(n), "%.avt$")
          end))
end

