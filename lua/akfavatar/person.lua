-- this is a module
-- see barbie-en.lua for an example

require "lua-akfavatar"

-- nil means no change - for no image use image="none"
local P = {name="", image="none", background_color=nil, balloon_color=nil}

person = P --> module name

local current_avatar

local function tell(text)
  -- remove spurious spaces
  text = string.gsub(text, "^%s+", "")
  text = string.gsub(text, "\n[ \t]+", "\n")
  text = string.gsub(text, "%s+$", "")
  avt.tell(text)
  avt.wait()
end

function P:new(o)
  o = o or {} --> create empty object if none is given
  setmetatable(o, self)
  self.__index = self
  return o
end

P.info = P.new --> nicer alias

-- balloon is automatically sized for the text
function P:__call(text)
  if current_avatar~=self then self:activate() end
  if text then tell(text) end
end

function P:activate()
  if current_avatar~=self then
    if self.image then
      avt.change_avatar_image(self.image)
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

-- prepare balloon with specified size
function P:says(...)
  if current_avatar~=self then self:activate() end
  avt.say(...)
end

function P:asks()
  if current_avatar~=self then self:activate() end
  return avt.ask()
end

function P:comes_in()
  if current_avatar~=self then self:activate() end
  avt.move_in()
end

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

