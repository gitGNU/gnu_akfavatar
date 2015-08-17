#!/usr/bin/env lua-akfavatar

--[[-------------------------------------------------------------------
Bücher
Copyright (c) 2015 Andreas K. Foerster <info@akfoerster.de>

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

local avt = require "lua-akfavatar"

local URL = "http://akfoerster.de/antiqua/index"
local downloader = "curl -sL --compressed -A 'AKFAvatar'"
local name, directory

avt.translations = {
  ["Books"] = {
    de="Bücher"},

  ["End of program"] = {
    de="Programm beenden"},

  ["Infos about the book"] = {
    de="Buchinformationen"},

  ["Please wait..."] = {  
    de="Bitte warten..."},

  ["Unknown option: "] = {
    de="Unbekannte Option: "},

  ["Sorry, but the program 'curl' is needed to connect "
   .."to the Internet."] = {
    de="Tut mir leid, aber das Programm 'curl' wird benötigt, "
            .."um eine Verbindung zum Internet aufzubauen."},

  ["Sorry, but I couldn't find the server name."] = {
    de="Tut mir leid, aber der Name des Servers konnte "
       .."nicht gefunden werden."},

  ["Sorry, but I couldn't connect to the server."] = {
    de="Tut mir leid, aber ich konnte keine Verbindung "
       .."zum Server aufbauen."},

  ["Sorry, but curl ended with error code %d."] = {
    de="Tut mir leid, aber curl endete mit Fehlercode %d."},

  ["Sorry, this book is no longer available"] = {
    de="Tut mir leid, aber dieses Buch ist nicht mehr verfügbar."},
}

local L = avt.translate


local function fetch(name)
  local f
  
  if directory then
    f = assert(io.open(directory..avt.dirsep..name..".txt"))
  else
    local base_URL = string.gsub(URL, "/[^/]*$", "/")
    f = assert(io.popen(downloader.." "..base_URL..name..".txt"))
  end

  local data = f:read("*all")
  local r, s, n = f:close()

  if not r and s == "exit" then
    if 127==n then
      error(L("Sorry, but the program 'curl' is needed to connect "
            .."to the Internet."), 0)
    elseif 6==n then
      error(L"Sorry, but I couldn't find the server name.", 0)
    elseif 7==n then
      eroor(L"Sorry, but I couldn't connect to the server.", 0)
    else
      error(string.format(L"Sorry, but curl ended with error code %d.", n), 0);
    end
  end

  -- empty files or just ">" in a file -> gone
  if not data or data:match("^%s*>?%s*$") then
    error(L"Sorry, this book is no longer available", 0)
  end

  -- redirection file
  local redirect = data:match("^%s*>%s*(%S+)%s*[\r\n]")
  if redirect then return fetch(redirect); end

  if data:match("^%*") then
    error(data, 0)
  end

  return data
end


local function postprocess(d)
  d = d:gsub("([^:])//", "%1")
  d = d:gsub("([^{]){(.-)}([^}])", "%1\x0E%2\x0F%3")
  d = d:gsub("([^%[])%[(.-)%]([^%]])", "%1%2%3")
  d = d:gsub("ſ", "s")
  return d
end


-- creates table of contents
local function contents(book)
  local toc = {{L"End of program", false},
               {L"Infos about the book", 1}}
  local nr = 0;

  -- iterate over lines
  for line in book:gmatch("(.-)\r?\n") do
    nr = nr + 1
    -- heading: space, chapter, dot, title
    local title = line:match("^%s+(%d+%..+)")
    if title then table.insert(toc, {title, nr}) end
  end

  return toc
end


-- shows book
local function show(name)
  avt.tell(L"Please wait...")

  local book = postprocess(fetch(name))
  local toc = contents(book)

  avt.set_title(book:match("^%s*(.-)%s*[\r\n]"), name)
  avt.set_balloon_size()

  local line_nr
  repeat
    avt.clear()
    line_nr = avt.menu(toc)
    if line_nr then avt.pager(book, line_nr) end
  until not line_nr
end


local function index(name)
  local idx = postprocess(fetch(name or "index"))
  local t = {}
  for line in idx:gmatch("(.-)\r?\n") do
    local short, title = line:match("^(%S+)%s*(.*)$")
    if short and short~="" and not short:match("[:/]") then
      if not title or title=="" then title = short end
      table.insert(t, {title, short})
    end
  end

  avt.clear()
  return avt.menu(t)
end


local function init()
  local env = os.getenv("LESENURL")
  if env then URL = env; end

  local i = 0
  while i<#arg do
    i = i + 1
    local v = arg[i]
    if not v:find("^-") then
      name = v
    elseif "-u"==v then
      i = i + 1
      URL = arg[i]
    elseif "-d"==v then
      i = i + 1
      directory = arg[i]
    else
      error(L"Unknown option: "..v, 0)
    end
  end
end


init()
avt.encoding("UTF-8")
avt.title(L"Books")
avt.set_background_color("sienna")
avt.start()
avt.set_balloon_color("antique white")

if not name then
  repeat
    name = index(name)
  until not name:match("^index")
end

show(name)
