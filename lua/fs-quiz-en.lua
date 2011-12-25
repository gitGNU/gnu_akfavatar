#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later
-- with ideas from Arne Babenhauserheide

-- when you make your own questionaire, you need not keep
-- the name from the copyright notice

local avt = require "lua-akfavatar"
require "akfavatar.questionary"

avt.initialize {
  title="Free Software Quiz",
  avatar=assert(avt.search("gnu-head.xpm")),
  encoding="UTF-8",
  audio=true
}


questionary {

  {"In which year was the GNU-project first announced?",
   1983, 83},

  {"What's the name of the founder of the Free Software movement\n"
   .. "and the GNU project?",
   "Richard M. Stallman", "Richard Stallman", "Stallman",
   "Richard Matthew Stallman", "RMS"},

  {"What does “GNU” stand for?",
   "GNU's not Unix", "GNU is not Unix"},

  {"What does “FSF” stand for?",
   "Free Software Foundation"},

  {"What does “GPL” stand for?",
   "GNU General Public License", "General Public License"
   --> Note: "GNU Public License" would be wrong!
   },

  {"Which of the GNU licenses requires that the source code is made available,\n"
   .. "when the program is accessed over a network? (Shortname, without version number)",
   "AGPL"},


  {"What is the name of the shell of the GNU projekt? (shortname)",
   "Bash"},

  {"What does “GCC” stand for?",
   "GNU Compiler Collection", "GNU C Compiler"},

  {"With which kernel is the GNU system commonly used?",
   "Linux"},

  {"What is the name of the developer of the kernel Linux?",
   "Linus Torvalds", "Linus B. Torvalds", "Linus Benedict Torvalds", "Torvalds"},

  {"Is it allowed to sell Free Software, that you didn't write your own,\n"
   .. "for a price?",
   yes},

}
