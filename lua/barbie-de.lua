#!/usr/bin/env lua-akfavatar

-- Dies ist nur ein kleines Beispiel für Lua-AKFAvatar 
-- mit unterschiedlichen Avataren.
-- Bitte nach Belieben Änderungen vornehmen.

require "lua-akfavatar"
require "akfavatar.person"
require "akfavatar.female_user"
require "akfavatar.male_user"

avt.initialize {title="Ken & Barbie", shortname="Barbie", 
                encoding="UTF-8", avatar="none"}

avt.set_text_delay ()

local ken, barbie, story --> Einführung der lokalen Schauspieler

--------------------------------------------------------------------------
ken = person: new {name="Ken", image=male_user,
                   background_color="sky blue", balloon_color="ghost white"}

barbie = person: new {name="Barbie", image=female_user,
                      background_color="pink", balloon_color="floral white"}

story = person: new {image="none", background_color="gray", 
                     balloon_color="tan"}
------------------------------------------------------------------------

story [[
Dies ist eine Geschichte über Ken und Barbie.

Okay, es ist nur ein kurzes Demo für
AKFAvatar. Es soll zeigen, wie man einfach
Geschichten mit mehr als nur einem Avatar
schreiben kann. Bitte mal in das Skript
schauen und es nach Belieben anpassen.

Viel Spaß...
]]

ken: comes_in ()
ken [[
Hallo Barbie!
Wie geht's dir?
]]

barbie "Mir geht es gut, danke!"

barbie "Sehe ich heute schön aus?"

if ken: affirms () then
  ken "Du siehst phantastisch aus!"
  barbie [[
  Oh, danke.
  Du bist so lieb!
  ]]

  barbie "Ken... Ich möchte ein Baby."
  ken "Okay, auf geht's!"
  ken: leaves ()
  barbie: leaves () --> Barbie folgt sofort
  story [[
  Der Rest dieser Geschichte bleibt der Phantasie
  des Zuschauers überlassen.
  ]]
else --> Ken sagt etwas Negatives
  ken "Nun, du hast schonmal besser ausgesehen."
  barbie "Oh, du bist so'n Arsch!"
  barbie: leaves ()
  ken: waits (4.5)
  ken "Barbie? ..."
  ken: waits (3.5)
  ken "Barbie!!!"
  ken: waits (3.7)
  story [[
    Und so wurden ihre Scheidungsanwälte reich.

    Happy End! ... für die Anwälte.
    ]]
end --> Ende von if avt.decide()

