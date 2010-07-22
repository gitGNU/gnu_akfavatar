#!/usr/bin/env lua-akfavatar

-- Dies ist nur ein kleines Beispiel für Lua-AKFAvatar 
-- mit unterschiedlichen Avataren.
-- Bitte nach Belieben Änderungen vornehmen.

require "lua-akfavatar"
require "akfavatar.person"

avt.initialize {title="Ken & Barbie", shortname="Barbie", 
                encoding="UTF-8", avatar="none"}

avt.set_text_delay ()

--------------------------------------------------------------------------
local Ken, Barbie, Erzaehler --> Einführung der lokalen Schauspieler

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

Erzaehler = person:
  info {
  image = "none",
  background_color = "gray",
  balloon_color = "tan"
  }
------------------------------------------------------------------------

Erzaehler
[[
Dies ist eine Geschichte über Ken und Barbie.

Okay, es ist nur ein kurzes Demo für
AKFAvatar. Es soll zeigen, wie man einfach
Geschichten mit mehr als nur einem Avatar
schreiben kann. Bitte mal in das Skript
schauen und es nach Belieben anpassen.

Viel Spaß...
]]

Ken: comes_in ()

Ken
[[
Hallo Barbie!
Wie geht's dir?
]]

Barbie "Mir geht es gut, danke!"

Barbie "Sehe ich heute schön aus?"

if Ken: affirms () then
  Ken "Du siehst phantastisch aus!"

  Barbie
  [[
  Oh, danke.
  Du bist so lieb!
  ]]

  Barbie "Ken... Ich möchte ein Baby."
  Ken "Okay, auf geht's!"
  Ken: leaves ()
  Barbie: leaves () --> Barbie folgt sofort

  Erzaehler
  [[
  Der Rest dieser Geschichte bleibt der Phantasie
  des Zuschauers überlassen.
  ]]
else --> Ken sagt etwas Negatives
  Ken "Nun, du hast schonmal besser ausgesehen."
  Barbie "Oh, du bist so'n Arsch!"
  Barbie: leaves ()
  Ken: waits (4.5)
  Ken "Barbie? ..."
  Ken: waits (3.5)
  Ken "Barbie!!!"
  Ken: waits (3.7)

  Erzaehler
  [[
  Und so wurden ihre Scheidungsanwälte reich.

  Happy End! ... für die Anwälte.
  ]]
end --> Ende von if Ken: affirms ()

