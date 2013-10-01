#!/usr/bin/env lua-akfavatar

--[[---------------------------------------------------------------------
Audio player for AKFvatar
Copyright (c) 2011,2012,2013 Andreas K. Foerster <info@akfoerster.de>
License: GPL version 3 or later

Supported audio formats: Ogg Vorbis, Wave, AU
Supported decoders: flac, opusdec, mpg123, (mpg321, madplay)
Supported playlist formats: M3U, PLS
--]]--------------------------------------------------------------------

local avt = require "lua-akfavatar"
local vorbis = avt.optional "akfavatar-vorbis"
local default_cover = assert(avt.search("audio1.xpm"))

-- use $input for input file name, $output for AU or WAVE output files
-- comment out if you don't even want those files to show up
local decoder_flac = 'flac --decode --silent --force --output-name="$output" "$input"'
local decoder_opus = 'opusdec --quiet --force-wav "$input" "$output"'
local decoder_mpeg = 'mpg123 --quiet --au "$output" "$input"'
--local decoder_mpeg = 'mpg321 --quiet --au "$output" "$input"'
--local decoder_mpeg = 'madplay -q -o "wave:$output" "$input"'
-- note: madplay supports only 8 bit for the snd format, so avoid that

-- downloader application
-- url is appended, data should be dumped to stdout
local downloader = "curl --silent --location"

avt.translations = {
  ["Audio-Player"] = {
    de="Audio-Abspieler"},

  ["play all..."] = {
    de="alle abspielen..."},
}

local L = avt.translate

avt.title(L"Audio-Player")
avt.start()
avt.start_audio()
avt.set_balloon_color("tan")
avt.encoding("UTF-8")

local function tempname()
  local tmp = os.tmpname()

  -- for Windows
  if string.find(tmp, "^\\") then
    tmp = os.getenv("TMP") .. tmp
  end

  -- print ("tmp:", tmp)

  return tmp
end

-- open URL with the tool "curl"
-- returns file handle
local function open_url(url)
  -- Windows needs the "rb" mode - most system don't accept it
  return assert(io.popen(downloader .. " " .. url, "rb")
            or io.popen(downloader .. " " .. url, "r"))
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
    avt.show_image_file(default_cover)
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

local function load_decoder(decoder, url)
  local tmp = tempname()

  local decode = string.gsub(decoder, "%$(%w+)",
                   {output=tmp, input=handle_list_entry(url)})

  local audio = nil
  if os.execute(decode) then
    audio = avt.load_audio_file(tmp, "play")
  end

  -- remove a file while it is opened works on most systems
  -- it is just really removed after closing
  os.remove(tmp)

  return audio
end

local function load_audio(url)
  local audio = nil
  local u = string.lower(url)

  if string.find(u, "^https?://") or string.find(u, "^s?ftps?://") then
    audio = avt.load_audio(get_url(url), "play")
  elseif string.find(u, "%.flac?$") then
    audio = load_decoder(decoder_flac, url)
  elseif string.find(u, "%.opus$") then
    audio = load_decoder(decoder_opus, url)
  elseif string.find(u, "%.mp[123]$") then
    audio = load_decoder(decoder_mpeg, url)
  else
    audio = avt.load_audio_file(handle_list_entry(url), "play")
  end

  return audio
end

local function play_single(filename) --> play a single file
  local audio, button

  show_cover(string.match(filename, "^(.-)[^\\/]+$"))

  audio = load_audio(filename)

  if not audio then
    avt.tell("cannot play audio file")
    avt.wait_button()
    return
  end

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
  local titles = {}
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
    else
      local title = string.match(line, "^%s*#EXTINF:%d*,%s*(.*)$")
      if title then
        table.insert(titles, title)
      end
    end
  end

  file:close()

  if titles == {} then
    titles = nil
  end

  return list, titles
end

local function pls(url) --> PLS ShoutCast Playlist
  local list = {}
  local titles = {}
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
      number = tonumber(number)
      path = handle_list_entry(path)
      list[number] = path
      if not titles[number] then
        titles[number] = path
      end
    else --> not a File entry
      local title
      number, title = string.match(line, "^%s*Title(%d+)=(.*)$")
      if number and title then
        titles[tonumber(number)] = title
      end
    end
  end

  file:close()

  if titles == {} then
    titles = nil
  end

  return list, titles
end

local function play_list(list, titles) --> plays a list of files (but no playlists)
  local filename, audio, button
  local number = 1

  -- no list?
  if not list or not list[1] then return end

  -- just one entry?
  if not list[2] then return play_single(list[1]) end

  -- eventually show playlist
  ::playlist::
  if titles then
    avt.set_balloon_size (rawlen (titles), 0)
    number = avt.menu(titles)
  end

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

  if titles then
    button = nil
    goto playlist
  end
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

-- which files to show in the file chooser
local function supported_file(n)
  n = string.lower(n)
  return (vorbis and string.find(n, "%.ogg$"))
         or (decoder_flac and string.find(n, "%.flac?$"))
         or (decoder_opus and string.find(n, "%.opus$"))
         or (decoder_mpeg and string.find(n, "%.mp[123]$"))
         or string.find(n, "%.au$")
         or string.find(n, "%.snd$")
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
      {"\xE2\x97\x84", "/up"},
      {"\xE2\x80\xA2 " .. L"play all...", "/all"}
      }

    -- find supported entries
    for i, v in ipairs(files) do
      if not string.find(v, "^%.") then
        if supported_file(v) then
          table.insert(audio_files, v)
          table.insert(menu, {avt.recode(v, "SYSTEM"), v})
        elseif avt.entry_type(v) == "directory" then
          table.insert(menu, {avt.recode(v, "SYSTEM") .. "\xE2\x96\xBA",
                              v .. "/"})
        end
      end
    end

    avt.set_balloon_size(0, 0)
    avt.bold(true)
    avt.say(L"Audio-Player")
    avt.bold(false)
    avt.newline()

    name = avt.long_menu(menu)

    avt.show_avatar()

    if name == "/up" then
      avt.set_directory("..")
      files = avt.directory_entries()
      table.sort(files)
    elseif name == "/all" then
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
