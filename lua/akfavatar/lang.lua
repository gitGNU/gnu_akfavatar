--[[-------------------------------------------------------------------
Lua module for translations
Copyright (c) 2011,2012 Andreas K. Foerster <info@akfoerster.de>

This module was written for AKFAvatar, but doesn't depend on it.

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

local l = {}

-- return language as 2 letter abbreviation
function l.getlanguage()
  local locale, language
  local find = string.find
  local getenv = os.getenv

  -- those environment variables and their order is standardized by POSIX
  -- unfortionally the content is marked as "implementation-defined"
  locale = getenv "LC_ALL" or getenv "LC_MESSAGES" or getenv "LANG"
           or os.setlocale("", "ctype") or "C"

  locale = string.lower(locale) --> make case-insensitive

  -- some systems use English names (sigh)
  -- the list is incomplete... (sigh)
  if locale == "c" or locale == "posix" then language = "en" -- assumption
  elseif find(locale, "^english") then language = "en"
  elseif find(locale, "^irish") then language = "ga"
  elseif find(locale, "^german") then language = "de"
  elseif find(locale, "^turkish") then language = "tr"
  elseif find(locale, "^french") then language = "fr"
  elseif find(locale, "^italian") then language = "it"
  elseif find(locale, "^spanish") then language = "es"
  elseif find(locale, "^portoguese") then language = "pt"
  elseif find(locale, "^dutch") then language = "nl"
  elseif find(locale, "^esperanto") then language = "eo"
  elseif find(locale, "^danish") then language = "da"
  elseif find(locale, "^finnish") then language = "fi"
  elseif find(locale, "^swedish") then language = "sv"
  elseif find(locale, "^norwegian") then language = "no"
  elseif find(locale, "^polish") then language = "pl"
  elseif find(locale, "^czech") then language = "cs"
  elseif find(locale, "^hungarian") then language = "hu"
  elseif find(locale, "^croatian") then language = "hr"
  elseif find(locale, "^serbian") then language = "sr"
  elseif find(locale, "^slovak") then language = "sk"
  elseif find(locale, "^slovene") then language = "sl"
  elseif find(locale, "^estonian") then language = "et"
  elseif find(locale, "^greek") then language = "el"
  elseif find(locale, "^hebrew") then language = "he"
  elseif find(locale, "^yiddish") then language = "yi"
  elseif find(locale, "^arabic") then language = "ar"
  elseif find(locale, "^russian") then language = "ru"
  elseif find(locale, "^belarusian") then language = "be"
  elseif find(locale, "^ukrainian") then language = "uk"
  elseif find(locale, "^afrikaans") then language = "af"
  elseif find(locale, "^chinese") then language = "zh"
  elseif find(locale, "^thai") then language = "th"
  elseif find(locale, "^japanese") then language = "ja"
  else language = string.sub(locale, 1, 2) --> assume it starts with the code
  end

  return language
end


local translations = {}
local language = l.getlanguage()


function l.use(lang)
  language = lang
end


function l.translations(t)
  translations = t
end


function l.translate (s)
  local tr = translations[s]
  return (tr and tr[language]) or s
end


return l
