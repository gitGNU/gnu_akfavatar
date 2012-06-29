--[[-------------------------------------------------------------------
Lua module for translations
Copyright (c) 2011,2012 Andreas K. Foerster <info@akfoerster.de>

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

local l = {}
local translations = {}
local language = avt.language


-- just for backward compatibility, don't use it
function l.getlanguage()
  return language
end


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
