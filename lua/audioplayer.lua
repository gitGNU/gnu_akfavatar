#!/usr/bin/env lua-akfavatar

--[[---------------------------------------------------------------------
Audio player for AKFvatar
Copyright (c) 2011 Andreas K. Foerster <info@akfoerster.de>
License: GPL version 3 or later

Supported audio formats: Ogg Vorbis, Wave, AU
Supported playlist formats: M3U, PLS
--]]--------------------------------------------------------------------

local avt = require "lua-akfavatar"
local default_cover = require "akfavatar.audio1"
local vorbis = avt.optional "akfavatar-vorbis"

-- downloader application
-- url is appended, data should be dumped to stdout
local downloader = "curl --silent --location"


avt.initialize{
  title    = "Audio-Player",
  avatar   = "default",
  encoding = "UTF-8",
  audio    = true
  }

-- open URL with the tool "curl"
-- returns file handle
local function open_url(url)
  return assert(io.popen(downloader .. " " .. url, "r"))
end

local function get_url(url) -- might not work on Windows
  local f = open_url(url)
  local data = f:read("*all")
  f:close()
  return data
end

local function show_cover(dir)
  dir = dir or ""
  if not avt.show_image_file(dir.."cover") then
    avt.show_image_string(default_cover)
  end
end

-- replace file-URL with normal filename and remove \r
local function handle_list_entry(s)
  s = string.gsub(s, "\r", "")
  s = string.gsub(s, "^file://localhost/(%a:.*)$", "%1") -- Windows
  s = string.gsub(s, "^file:///(%a:.*)$", "%1") -- Windows
  s = string.gsub(s, "^file://localhost/", "/")
  s = string.gsub(s, "^file:///", "/")
  return s
end

local function load_audio(url)
  local audio

  if string.find(url, "^https?://") or string.find(url, "^s?ftps?://") then
    audio = avt.load_audio_string(get_url(url))
  else
    audio = avt.load_audio_file(handle_list_entry(url))
  end

  return audio
end

local function play_single(filename) --> play a single file
  local button
  local audio = load_audio(filename)

  if not audio then
    avt.tell(filename, ":\nunsupported audio file")
    avt.wait_button()
    return
  end

  show_cover(string.match(filename, "^(.-)[^\\/]+$"))
  audio:play()

  repeat
    button = avt.navigate("ps")
    if button == "p" then
      avt.pause_audio(true)
      button = avt.navigate("rs")
      if button == "r" then avt.pause_audio(false) end
    end
  until button == "s" -- until stop is pressed or sound stopped

  avt.stop_audio()
  audio:free()
end -- play_single

-- splits a filename into the name and the directory
-- problem: doesn't work on URLs
local function basename(filename)
  local dir, name = string.match(filename, "^(.-)([^\\/]+)$")
  return name, dir
end

local function m3u(url) --> M3U Playlist
  local list = {}
  local file

  if string.find(url, "^https?://") or string.find(url, "^s?ftps?://") then
    file = open_url(url)
  else --> local file
    local name, dir = basename(url)
    avt.set_directory(dir) --> list may contain relative paths
    file = assert(io.open(name))
  end

  for line in file:lines() do
    if not string.find(line, "^%s*#") then 
      table.insert(list, handle_list_entry(line))
    end
  end

  file:close()
  return list
end

local function pls(filename) --> PLS ShoutCast Playlist
  local list = {}
  local file

  if string.find(url, "^https?://") or string.find(url, "^s?ftps?://") then
    file = open_url(url)
  else --> local file
    local name, dir = basename(url)
    avt.set_directory(dir) --> list may contain relative paths
    file = assert(io.open(name))
  end

  if not string.match(file:read("*line"), "^%[playlist%]%s*$") then
    file:close()
    return nil
    end

  for line in file:lines() do
    local number, path = string.match(line, "^%s*File(%d+)=(.*)$")
    if number and path then
      list[tonumber(number)] = handle_list_entry(path)
    end
  end

  file:close()
  return list
end

local function play_list(list) --> plays a list of files (but no playlists)
  local filename, audio, button
  local number = 1

  -- no list?
  if not list then return end

  -- just one entry?
  if not list[2] then return play_single(list[1]) end

  show_cover()

  while list[number] and button ~= "s" do

    repeat -- get entry
      filename = list[number]
      if not filename then break end
      if audio then audio:free() end
      audio = load_audio(filename)
      -- remove unplayable entries
      if not audio then table.remove(list, number) end
    until audio or not list[number]

    if not audio then return end

    audio:play()

    repeat
      button = avt.navigate("bpsf")

      if button == "p" then
        avt.pause_audio(true)
        button = avt.navigate("brsf")
        if button == "r" then avt.pause_audio(false) end
      end

      if button == "b" then number = number - 1
      elseif button == "f" then number = number + 1
      end
    until button ~= "r" --> repeat if play pressed

  end --> while list[number]

  avt.stop_audio()
  if audio then audio:free() end
end -- play_list

local function play(e) --> play file or list
  local t = type(e)

  if "table" == t then
    play_list(e)
  elseif "string" == t then
    if string.find(string.lower(e), "%.m3u$") then
      play_list(m3u(e))
    elseif string.find(string.lower(e), "%.pls$") then
      play_list(pls(e))
    else
     play_single(e)
    end
  -- do nothing for nil or other types
  end
end

local function supported_file(n)
  n = string.lower(n)
  return (vorbis and string.find(n, "%.ogg$"))
         or string.find(n, "%.au$")
         or string.find(n, "%.wav$")
         or string.find(n, "%.m3u$")
         or string.find(n, "%.pls$")
end

local function play_select()
  local name

  -- go home
  avt.set_directory(os.getenv("HOME") or os.getenv("USERPROFILE"))

  local files = avt.directory_entries()
  table.sort(files)

  repeat
    local audio_files = {}
    local menu = {
      [1] = "* parent directory",
      [2] = "* play all..."
      }

    -- find supported entries
    for i, v in ipairs(files) do
      if not string.find(v, "^%.") then
        if supported_file(v) then
          table.insert(audio_files, v)
          table.insert(menu, v)
        elseif avt.entry_type(v) == "directory" then
          table.insert(menu, v .. "/")
        end
      end
    end

    avt.set_balloon_size(0, 0)
    avt.bold(true)
    avt.say "Audio Player\n"
    avt.bold(false)

    local item = avt.long_menu(menu)
    name = menu[item]

    avt.show_avatar()

    if item == 1 then
      avt.set_directory("..")
      files = avt.directory_entries()
      table.sort(files)
    elseif item == 2 then
      play_list(audio_files) --> sorts out playlists
    elseif string.find(name, "/$") then -- directory
      avt.set_directory(name)
      files = avt.directory_entries()
      table.sort(files)
    else
      play(name)
    end
  until not name
end

if AUDIO then
  play(AUDIO) -- global AUDIO may be a string or an array
elseif arg[2] then
  play(arg) -- more than one file
elseif arg[1] then
  play(arg[1]) -- single file
else
  play_select()
end
