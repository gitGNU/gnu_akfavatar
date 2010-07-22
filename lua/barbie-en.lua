#!/usr/bin/env lua-akfavatar

-- this is just an example for Lua-AKFAvatar with different Avatars
-- you are encouraged to make changes to this script as you like

require "lua-akfavatar"
require "akfavatar.person"
require "akfavatar.female_user"
require "akfavatar.male_user"

avt.initialize {title="Ken & Barbie", shortname="Barbie", 
                encoding="UTF-8", avatar="none"}

avt.set_text_delay ()

local ken, barbie, story --> introducing the local actors

--------------------------------------------------------------------------
ken = person: new {name="Ken", image=male_user,
                   background_color="sky blue", balloon_color="ghost white"}

barbie = person: new {name="Barbie", image=female_user,
                      background_color="pink", balloon_color="floral white"}

story = person: new {image="none", background_color="gray", 
                     balloon_color="tan"}
------------------------------------------------------------------------

story [[
This is a story about Ken and Barbie.

Okay, this is just a short demo for
AKFAvatar. It shows, how you can easily
write stories with more than just one
Avatar. You are encouraged to make changes
to this script as you like.

Enjoy...
]]

ken: comes_in ()
ken [[
Hello Barbie!
How are you?
]]

barbie "I'm fine. Thank you!"

barbie "Do I look beautiful today?"

if ken: affirms () then
  ken "You look sooo gorgeous!"
  barbie [[
  Oh, thank you.
  You are so nice!
  ]]

  barbie "Ken... I want a baby."
  ken "Okay, let's do it!"
  ken: leaves ()
  barbie: leaves () --> barbie follows immeadiately
  story [[
  The rest of this story is left to the
  imagination of the audience.
  ]]
else --> ken says something negative
  ken "Well, you looked better some time."
  barbie "Oh, you're such a dork!"
  barbie: leaves ()
  ken: waits (4.5)
  ken "Barbie? ..."
  ken: waits (3.5)
  ken "Barbie!!!"
  ken: waits (3.7)
  story [[
    And so their divorce lawyers became rich.

    Happy End! ... for the lawyers.
    ]]
end -- end of if avt.decide()

