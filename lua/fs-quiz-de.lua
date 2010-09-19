#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.query"

avt.initialize {
  title="Free Software Quiz", 
  avatar=require "akfavatar.gnu_head",
  audio=true
}

query {

  -- Übersetzungen:
  correct = "Das ist richtig.",
  wrong = "Falsch!",
  again = "Nochmal versuchen?",
  correction = "Die richtige Antwort lautet:",


  {"In welchem Jahr wurde das GNU-Projekt erstmalig angekündigt?",
   {1983, 83}},

  {"Wie heißt der Gründer der Free Software Bewegung und des GNU-Projektes?",
   {"Richard M. Stallman", "Richard Stallman", "Stallman"}},

  {"Wofür steht der Ausdruck „GNU“?",
   {"GNU's not Unix", "GNU is not Unix"}},

  {"Wofür steht „FSF“?",
   "Free Software Foundation"},

  {"Wie heißt die Shell des GNU Projektes? (Kurzname)",
   "Bash"},

  {"Wofür steht „GCC“?",
   {"GNU Compiler Collection", "GNU C Compiler"}},

  {"Mit welchem Kernel wird das GNU System meistens verwendet?",
   "Linux"},

  {"Wie heißt der Entwickler des Kernels Linux?",
   {"Linus Torvalds", "Torvalds"}},
}