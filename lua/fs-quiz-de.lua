#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012 Andreas K. Foerster <akf@akfoerster.de>
-- Lizenz: GPL Version 3 oder neuer
-- mit Ideen von Arne Babenhauserheide

-- Wenn man einen eigenen Fragebogen erstellt, muss man den Namen
-- im Copyright-Vermerk nicht übernehmen

require "akfavatar.questionary"

{
  avatar = "gnu-head.xpm",
  lang = "de",

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
   true},

}
