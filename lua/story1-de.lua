#!/usr/bin/env lua-akfavatar

-- Dies ist nur ein kleines Beispiel für Lua-AKFAvatar 
-- mit unterschiedlichen Avataren.
-- Bitte nach Belieben Änderungen vornehmen.

require "lua-akfavatar"
require "akfavatar.person"

-- Übersetzungen von Anweisungen
person.kommt_herein = person.comes_in
person.geht_heraus = person.leaves
person.bestaetigt = person.affirms
person.wartet = person.waits

--------------------------------------------------------------------------

Mann = person:
  info {
  name = "Ken",
  image = require "akfavatar.male_user",
  background_color = "sky blue",
  balloon_color = "ghost white"
  }

Frau = person:
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

avt.initialize {title = Mann.name .. " & " .. Frau.name,
                shortname = Frau.name,
                encoding="UTF-8",
                avatar="none"
                }

avt.set_text_delay ()
avt.markup (true)

------------------------------------------------------------------------

Erzaehler
[[
_*$Mann und $Frau*_

Dies ist nur ein kurzes Demo für AKFAvatar.
Es soll zeigen, wie man einfach Geschichten
mit mehr als nur einem Avatar schreiben kann.
Bitte mal in das Skript schauen und es nach
Belieben anpassen.

*Viel Spaß...*
]]

Mann: kommt_herein ()

Mann
[[
Hallo $Frau!
Wie geht's dir?
]]

Frau "Mir geht es gut, danke!"

Frau "Sehe ich heute schön aus?"

if Mann: bestaetigt () then
  Mann "Du siehst _phantastisch_ aus!"

  Frau
  [[
  Oh, danke.
  Du bist so lieb!
  ]]

  Frau "$Mann... Ich möchte ein Baby."
  Mann "Okay, auf geht's!"
  Mann: geht_heraus ()
  Frau: geht_heraus () --> Frau folgt sofort

  Erzaehler
  [[
  Der Rest dieser Geschichte bleibt der Phantasie
  des Zuschauers überlassen...
  ]]
else --> der Mann sagt etwas Negatives
  Mann "Nun, du hast schonmal besser ausgesehen."
  Frau "Oh, du bist so'n *Blödmann*!"
  Frau: geht_heraus ()
  Mann: wartet (4.5)
  Mann "$Frau? ..."
  Mann: wartet (3.5)
  Mann "*$Frau!!!*"
  Mann: wartet (3.7)

  Erzaehler
  [[
  Und so wurden ihre Scheidungsanwälte reich.

  *Happy End!* ... für die Anwälte.
  ]]
end --> Ende von if Mann: bestaetigt ()

