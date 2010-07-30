#!/usr/bin/env lua-akfavatar

-- this is just an example for Lua-AKFAvatar with different Avatars
-- you are encouraged to make changes to this script as you like

require "lua-akfavatar"
require "akfavatar.person"

--------------------------------------------------------------------------
man = person:
  info {
  name = "Ken",
  image = require "akfavatar.male_user",
  background_color = "sky blue",
  balloon_color = "ghost white"
  }

woman = person:
  info {
  name = "Barbie",
  image = require "akfavatar.female_user",
  background_color = "pink",
  balloon_color = "floral white"
  }

story = person:
  info {
  image = "none",
  background_color = "gray",
  balloon_color = "tan"
  }

------------------------------------------------------------------------

avt.initialize {title = man.name .. " & " .. woman.name,
                shortname = woman.name,
                encoding = "UTF-8",
                avatar = "none"
                }

avt.set_text_delay ()
avt.markup (true)

------------------------------------------------------------------------

story
[[
_*$man and $woman*_

This is just a short demo for AKFAvatar.
It shows, how you can easily write stories
with more than just one Avatar. You are
encouraged to make changes to this script
as you like.

*Enjoy...*
]]

man: arrives ()

man
[[
Hello $woman!
How are you?
]]

woman "I'm fine. Thank you!"

woman "Do I look beautiful today?"

if man: affirms () then
  man "You look sooo _gorgeous_!"

  woman
  [[
  Oh, thank you.
  You are so nice!
  ]]

  woman "$man... I want a baby."
  man "Okay, let's do it!"
  man: leaves ()
  woman: leaves () --> woman follows immeadiately

  story
  [[
  The rest of this story is left to the
  imagination of the audience.
  ]]
else --> man says something negative
  man "Well, you looked better some time."
  woman "Oh, you're such an *idiot*!"
  woman: leaves ()
  man: waits (4.5)
  man "$woman? ..."
  man: waits (3.5)
  man "*$woman!!!*"
  man: waits (3.7)

  story
  [[
  And so their divorce lawyers became rich.

  *Happy End!* ... for the lawyers.
  ]]
end -- end of if man: affirms ()
