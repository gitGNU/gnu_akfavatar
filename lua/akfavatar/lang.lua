--[[-------------------------------------------------------------------
Lua module for translations
Copyright (c) 2011 Andreas K. Foerster <info@akfoerster.de>

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

--[[
Usage:
  lang.getlanguage()		detect language code
  lang.use("de")		use that language
  lang.translations {}		define the translations (see example)
  lang.translate(s)		returns the translated string

Example:

  lang.translations {
    ["That's live!"] = {
      de="So ist das Leben!",
      fr="C'est la vie!" },
  }

  local L = lang.translate
  print(L"That's live!")

--]]

local l = {}
lang = l

-- return language as 2 letter abbreviation
function l.getlanguage()
  local locale, language
  local find = string.find
  local getenv = os.getenv

  locale = getenv "LC_ALL" or getenv "LC_MESSAGES" or getenv "LANG"
           or os.setlocale("", "ctype") or "C"

  -- some systems use English names (sigh)
  -- the list is incomplete... (sigh)
  if locale == "C" or locale == "POSIX" then language = "en" -- assumption
  elseif find(locale, "^English") then language = "en"
  elseif find(locale, "^Irish") then language = "ga"
  elseif find(locale, "^German") then language = "de"
  elseif find(locale, "^Turkish") then language = "tr"
  elseif find(locale, "^French") then language = "fr"
  elseif find(locale, "^Italian") then language = "it"
  elseif find(locale, "^Spanish") then language = "es"
  elseif find(locale, "^Portoguese") then language = "pt"
  elseif find(locale, "^Dutch") then language = "nl"
  elseif find(locale, "^Esperanto") then language = "eo"
  elseif find(locale, "^Danish") then language = "da"
  elseif find(locale, "^Finnish") then language = "fi"
  elseif find(locale, "^Swedish") then language = "sv"
  elseif find(locale, "^Norwegian") then language = "no"
  elseif find(locale, "^Polish") then language = "pl"
  elseif find(locale, "^Czech") then language = "cs"
  elseif find(locale, "^Hungarian") then language = "hu"
  elseif find(locale, "^Croatian") then language = "hr"
  elseif find(locale, "^Serbian") then language = "sr"
  elseif find(locale, "^Slovak") then language = "sk"
  elseif find(locale, "^Slovene") then language = "sl"
  elseif find(locale, "^Estonian") then language = "et"
  elseif find(locale, "^Greek") then language = "el"
  elseif find(locale, "^Hebrew") then language = "he"
  elseif find(locale, "^Yiddish") then language = "yi"
  elseif find(locale, "^Arabic") then language = "ar"
  elseif find(locale, "^Russian") then language = "ru"
  elseif find(locale, "^Belarusian") then language = "be"
  elseif find(locale, "^Ukrainian") then language = "uk"
  elseif find(locale, "^Afrikaans") then language = "af"
  elseif find(locale, "^Chinese") then language = "zh"
  elseif find(locale, "^Thai") then language = "th"
  elseif find(locale, "^Japanese") then language = "ja"
  else language = string.sub(locale, 1, 2) --> assume it starts with the code
  end

  return language
end


local translations
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
