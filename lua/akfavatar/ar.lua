--[[-------------------------------------------------------------------
Lua module to handle ar archives (unfinished)
currenty only for reading
member names are limited to 15 characters

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

ar = {}

-- open an archive, mode = "r"
function ar:open(filename, mode)
  local f, msg
  local obj = { name=filename, mode=mode }

  if mode == "r" or not mode then
    f, msg = io.open(filename, "rb")
    if not f then
      return nil, msg
    end

    if f:read(8) ~= "!<arch>\n" then
      f:close()
      return nil, filename .. ": not an archive file"
    end
  else
    return nil, string.format("mode \"%s\" not supported", mode)
  end --> if mode

  setmetatable(obj, self)
  self.__index = self
  obj.f = f

  return obj
end

-- closes archive
function ar:close()
  self.f:close()
end

-- get next member - skip is the size of the previous member or 0
function ar:next(skip)
  if skip and skip > 0 then self.f:seek("cur", skip+(skip%2)) end

  local name = self.f:read(16)
  if name == nil then --> end reached
    return 0
  end

  name = string.match(name, "^(.-)/?%s*$")

  local timestamp = tonumber(self.f:read(12))
  local uid = tonumber(self.f:read(6))
  local gid = tonumber(self.f:read(6))
  local mode = string.match(self.f:read(8), "%d+")
  local size = tonumber(self.f:read(10))

   -- check the magic enty
  if self.f:read(2) ~= "`\n" then
    return nil, self.name .. ": archive broken"
  end

  return size, name, timestamp, uid, gid, mode
end

-- seeks a named member
function ar:seek(member)
  local size, name, timestamp, uid, gid, mode

  size, name = self.f:seek("set", 8) --> skip the header
  if size == nil then
    return nil, name
  end

  size = 0
  repeat
    size, name, timestamp, uid, gid, mode = self:next(size)
    if size == nil then return nil, name
    elseif size == 0 then 
      return nil, self.name .. ": " .. member .. ": member not found"
    end
  until name == member

  return size, name, timestamp, uid, gid, mode
end

-- gets a named member as string
function ar:get(member)
  local size, n = self:seek(member)
  if size == nil then
    return nil, n
  end

  local result = self.f:read(size)
  if size%2~=0 then self.f:seek("cur", 1) end

  return result
end

return ar

