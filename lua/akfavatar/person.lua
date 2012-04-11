-- Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- this is a module
-- see story1-en.lua for an example

local avt = require "lua-akfavatar"

-- nil means no change - for no image use image="none"
local P = {name="", image="none", background_color=nil, balloon_color=nil}

local current_avatar

function P:new(o)
  o = o or {} --> create empty object if none is given
  setmetatable(o, self)
  self.__index = self
  return o
end

P.info = P.new --> nicer alias

-- replace $person with the name
local function expand (s)
 return (string.gsub(s, "%$(%w+)", function(p) return _G[p]["name"] or _G[p] end))
end

-- balloon is automatically sized for the text
function P:__call(...)
  if current_avatar~=self then self:activate() end
  local text = table.concat ({...})
  if text then
    -- remove spurious spaces
    text = string.gsub(text, "^%s*(.-)%s*$", "%1")
    text = string.gsub(text, "\n[ \t]+", "\n")
    avt.tell(expand(text))
    avt.wait()
  end
end

function P:activate()
  if current_avatar~=self then
    if self.image then
      if not avt.avatar_image(self.image) then
        avt.avatar_image_file(self.image)
        end
      end
    if self.name then
      avt.set_avatar_name(self.name)
      end
    if self.background_color then
      avt.set_background_color(self.background_color)
      end
    if self.balloon_color then
      avt.set_balloon_color(self.balloon_color)
      end

    current_avatar = self
    end
end

function P:text_size(height, width)
  if current_avatar~=self then self:activate() end
  avt.set_balloon_size(height, width)
end

-- say something without changing the balloon size
function P:says(...)
  if current_avatar~=self then self:activate() end
  avt.say(expand(table.concat ({...})))
end

function P:asks()
  if current_avatar~=self then self:activate() end
  return avt.ask()
end

function P:comes_in()
  if current_avatar~=self then self:activate() end
  avt.move_in()
end

P.arrives = P.comes_in

function P:leaves()
  if current_avatar~=self then self:activate() end
  avt.show_avatar()
  avt.move_out()
end

function P:affirms()
  if current_avatar~=self then self:activate() end
  avt.show_avatar()
  return avt.decide()
end

-- waits with no balloon shown - otherwise use avt.wait(s)
function P:waits(seconds)
  if current_avatar~=self then self:activate() end
  avt.show_avatar()
  avt.wait(seconds)
end

return P

