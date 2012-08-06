#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- Dies ist nur ein kleines Beispiel für Lua-AKFAvatar 
-- mit unterschiedlichen Avataren.
-- Bitte nach Belieben Änderungen vornehmen.

-- Die eigentliche Geschichte fängt nach der letzten Trennlinie an

local avt = require "lua-akfavatar"
local person = require "akfavatar.person"

-- Übersetzungen von Anweisungen
person.kommt_herein = person.comes_in
person.geht_heraus = person.leaves
person.bestaetigt = person.affirms
person.wartet = person.waits
person.denkt = person.thinks

--------------------------------------------------------------------------

Mann = person:
  info {
  name = "Ken",
  image = assert(avt.search("male_user.xpm")),
  background_color = "sky blue",
  balloon_color = "ghost white"
  }

Frau = person:
  info {
  name = "Barbie",
  image = assert(avt.search("female_user.xpm")),
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

avt.encoding("UTF-8")
avt.title(Frau.name .. " & " .. Mann.name, Frau.name)
avt.start()
avt.set_text_delay () --> den Langsamschreibmodus aktivieren
avt.markup (true) --> Verwende "_" für Unterstreichen, "*" für Fettdruck

------------------------------------------------------------------------

Erzaehler
[[
_*$Frau und $Mann*_

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

Frau: denkt "Wie nett er heute doch ist..."

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

