-- German wrapper for "akfavatar.questionary"
-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "akfavatar.questionary"

questionarymessages {
  encoding = "UTF-8",
  correct = "Das ist richtig.",
  wrong = "Falsch!",
  again = "Nochmal versuchen?",
  correction = "Die richtige Antwort lautet:",
  questions = "Fragen",
  correctly_answered = "davon richtig"
}

Fragebogen = questionary

-- Für ja/nein Fragen, ohne Anführungsstriche
ja, nein = true, false

return Fragebogen
