#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later
-- mit Ideen von Arne Babenhauserheide

require "lua-akfavatar"
require "akfavatar.questionary"

avt.initialize {
  title="Free Software Quiz",
  avatar=require "akfavatar.gnu_head",
  encoding="UTF-8",
  audio=true
}


-- Übersetzungen:
questionarymessages {
  correct = "Das ist richtig.",
  wrong = "Falsch!",
  again = "Nochmal versuchen?",
  correction = "Die richtige Antwort lautet:",
  questions = "Fragen",
  correctly_answered = "davon richtig"
}

-- Für ja/nein Fragen ohne Anführungsstriche
local ja, nein = true, false

questionary {

  {"In welchem Jahr wurde das GNU-Projekt erstmalig angekündigt?",
   1983, 83},

  {"Wie heißt der Gründer der Free Software Bewegung und des GNU-Projektes?",
   "Richard M. Stallman", "Richard Stallman", "Stallman",
    "Richard Matthew Stallman", "RMS"},

  {"Wofür steht der Ausdruck „GNU“?",
   "GNU's not Unix", "GNU is not Unix"},

  {"Wofür steht „FSF“?",
   "Free Software Foundation"},

  {"Wofür steht „GPL“?",
   "GNU General Public License", "General Public License"
   --> Anmerkung: "GNU Public License" wäre falsch!
   },

  {"Welche der GNU Lizenzen verlangt die Weitergabe des Quellcodes bei Zugriff\n"
   .. "über ein Netzwerk? (Kurzname, ohne Versionsnummer)",
   "AGPL"},

  {"Wie heißt die Shell des GNU Projektes? (Kurzname)",
   "Bash"},

  {"Wofür steht „GCC“?",
   "GNU Compiler Collection", "GNU C Compiler"},

  {"Mit welchem Kernel wird das GNU System meistens verwendet?",
   "Linux"},

  {"Wie heißt der Entwickler des Kernels Linux?",
   "Linus Torvalds", "Linus B. Torvalds", "Linus Benedict Torvalds", "Torvalds"},

  {"Darf man Freie Software, die man nicht selbst geschrieben hat, verkaufen?",
   ja},

}
