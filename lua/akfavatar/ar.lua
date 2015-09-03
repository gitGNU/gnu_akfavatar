--[[-------------------------------------------------------------------
Lua module to handle ar archives (unfinished)
currenty only for reading
member names are limited to 15 characters

Copyright (c) 2010,2011,2012 Andreas K. Foerster <akf@akfoerster.de>

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

local ar = {}

-- open an archive, mode = "r"
function ar:open(filename, mode)
  local f, msg
  local obj = { name=filename, mode=mode, member_size=0 }
  -- member_size is only > 0 when the member was opened

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
    return nil, string.format("mode '%s' not supported, yet", mode)
  end --> if mode

  setmetatable(obj, self)
  self.__index = self
  obj.file = f

  return obj
end

-- closes archive
function ar:close()
  self.file:close()
  self.file=nil
  self.name=nil
  self.mode=nil
  self.member_size=nil
end

-- get next member
-- on success it returns size, name, timestamp, uid, gid, mode
-- or -1 when the end is reached
-- on error it returns nil and an error message
function ar:next()
  if self.member_size > 0 then --> skip body of current member
    self.file:seek("cur", self.member_size + (self.member_size % 2))
    self.member_size = 0
  end

  local name = self.file:read(16)
  if name == nil then --> end reached
    return -1
  end

  name = string.match(name, "^(.-)/?%s*$")

  local timestamp = tonumber(self.file:read(12))
  local uid = tonumber(self.file:read(6))
  local gid = tonumber(self.file:read(6))
  local mode = tonumber(self.file:read(8), 8) -- given as octal!
  local size = tonumber(self.file:read(10))
  self.member_size = size

   -- check the magic entry
  if self.file:read(2) ~= "`\n" then
    return nil, self.name .. ": archive broken"
  end

  return size, name, timestamp, uid, gid, mode
end

function ar:rewind()
  local size, msg = self.file:seek("set", 8) --> skip the header
  self.member_size = 0

  if size == nil then
    return nil, msg
  else
    return true
  end
end

-- seeks a named member
-- on success it returns size, name, timestamp, uid, gid, mode
-- on error it returns nil and an error message
function ar:seek(member)
  local size, name, timestamp, uid, gid, mode

  size, name = self.file:seek("set", 8)
  self.member_size = 0
  if size == nil then
    return nil, name
  end

  repeat
    size, name, timestamp, uid, gid, mode = self:next()
    if size == nil then return nil, name
    elseif size == -1 then
      return nil, self.name .. ": " .. member .. ": member not found"
    end
  until name == member

  return size, name, timestamp, uid, gid, mode
end

-- gets content of a member as string
-- if no member name is given, it gets the current/next member
-- on error it returns nil and an error message
function ar:get(member)
  local size, msg

  if member then
    size, msg = self:seek(member)
  else --> no member given
    if self.member_size > 0 then
      size = self.member_size
    else --> no member opened
      size, msg = self:next()
      if size == -1 then
        return nil, self.name .. ": no further members"
      end
    end
  end

  if size == nil then
    return nil, msg
  end

  local result = self.file:read(size)
  if size % 2 ~= 0 then self.file:seek("cur", 1) end
  self.member_size = 0

  return result
end

-- loads the member as lua code
-- if no member name is given, it gets the current/next member
-- errors are propagated
function ar:loadlua(member)
  assert(load(assert(self:get(member)), member or self.name, "t"))
end

return ar
