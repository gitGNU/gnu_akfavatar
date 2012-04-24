#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- this is just an example for Lua-AKFAvatar with different Avatars
-- you are encouraged to make changes to this script as you like

-- the story itself starts after the last deviding line

local avt = require "lua-akfavatar"
local person = require "akfavatar.person"

--------------------------------------------------------------------------
man = person:
  info {
  name = "Ken",
  image = assert(avt.search("male_user.xpm")),
  background_color = "sky blue",
  balloon_color = "ghost white"
  }

woman = person:
  info {
  name = "Barbie",
  image = assert(avt.search("female_user.xpm")),
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

avt.encoding("UTF-8")
avt.title(woman.name .. " & " .. man.name, woman.name)
avt.start()
avt.set_text_delay ()  --> activate the slowprint mode
avt.markup (true) --> use "_" for underlined text and "*" for bold

------------------------------------------------------------------------

story
[[
_*$woman and $man*_

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
