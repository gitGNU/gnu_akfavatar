#!/usr/bin/env lua-akfavatar

-- This file is dedicated to the Public Domain (CC0)
-- http://creativecommons.org/publicdomain/zero/1.0/

local avt = require "lua-akfavatar"

-- translations for expressions
avt.translations = {
  ["Greeting"] = { de="Grüßen" },
  ["Hello %s!"] = {
    de="Hallo %s!",
    nl="Hallo %s!",
    tr="Merhaba %s!",
    it="Ciao, %s!",
    pl="Cześć %s!",
    pt="Olá %s!",
    es="¡Hola, %s!",
    fr="Salut %s!",
    sv="Hej %s!",
    da="Hej %s!",
    el="Καλημέρα %s!",
    ru="Здравствуй, %s!",
    bg="Здравей, %s!"},
  ["unknown"] = { de="Unbekannter" }
  }

-- please send corrections and further translations to info@akfoerster.de

local L = avt.translate

avt.encoding("UTF-8")
avt.title(L"Greeting")
avt.start()
avt.avatar_image_file(avt.search "female_user.xpm")

-- try to get realname for a username - nil on error
-- works on POSIX compatible systems, if the name is set
local function get_realname(username)
  if not username then return nil end

  local f = io.open ("/etc/passwd", "r")
  if not f then return nil end

  local pattern = "^"..username..":[^:]*:%d*:%d*:([^,:]+)"
  local name

  for l in f:lines() do
    name = string.match(l, pattern)
    if name then break end
  end

  f:close()

  -- eventually recode name from system specific charset
  if name then name = avt.recode(name, "SYSTEM") end

  return name
end

-- the username can be in different environment variables
-- depending on the system
local username =
  os.getenv "LOGNAME" or os.getenv "USER" or os.getenv "USERNAME"

local realname = get_realname(username)

-- both realname and username may still be nil!
avt.tell(string.format(L"Hello %s!",
         realname or username or L"unknown"))

avt.wait_button()
