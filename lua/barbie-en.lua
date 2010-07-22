#!/usr/bin/env lua-akfavatar

-- this is just an example for Lua-AKFAvatar with different Avatars
-- you are encouraged to make changes to this script as you like

require "lua-akfavatar"
require "akfavatar.person"

avt.initialize {title="Ken & Barbie", shortname="Barbie", 
                encoding="UTF-8", avatar="none"}

avt.set_text_delay ()

--------------------------------------------------------------------------
local Ken, Barbie, story --> introducing the local actors

Ken = person:
  info {
  name = "Ken",
  image = require "akfavatar.male_user",
  background_color = "sky blue",
  balloon_color = "ghost white"
  }

Barbie = person:
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

story
[[
This is a story about Ken and Barbie.

Okay, this is just a short demo for
AKFAvatar. It shows, how you can easily
write stories with more than just one
Avatar. You are encouraged to make changes
to this script as you like.

Enjoy...
]]

Ken: comes_in ()

Ken
[[
Hello Barbie!
How are you?
]]

Barbie "I'm fine. Thank you!"

Barbie "Do I look beautiful today?"

if Ken: affirms () then
  Ken "You look sooo gorgeous!"

  Barbie
  [[
  Oh, thank you.
  You are so nice!
  ]]

  Barbie "Ken... I want a baby."
  Ken "Okay, let's do it!"
  Ken: leaves ()
  Barbie: leaves () --> Barbie follows immeadiately

  story
  [[
  The rest of this story is left to the
  imagination of the audience.
  ]]
else --> Ken says something negative
  Ken "Well, you looked better some time."
  Barbie "Oh, you're such a dork!"
  Barbie: leaves ()
  Ken: waits (4.5)
  Ken "Barbie? ..."
  Ken: waits (3.5)
  Ken "Barbie!!!"
  Ken: waits (3.7)

  story
  [[
  And so their divorce lawyers became rich.

  Happy End! ... for the lawyers.
  ]]
end -- end of if Ken: affirms ()
