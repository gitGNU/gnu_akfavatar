#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later
-- with ideas from Arne Babenhauserheide

require "lua-akfavatar"
require "akfavatar.query"

avt.initialize {
  title="Free Software Quiz",
  avatar=require "akfavatar.gnu_head",
  encoding="UTF-8",
  audio=true
}


-- This is not really needed for English, but crucial for other languages
querymessages {
  correct = "That's correct.",
  wrong = "Wrong!",
  again = "Try again?",
  correction = "The correct answer:",
  questions = "Questions",
  correctly_answered = "correct"
}

-- for yes/no questions - use without quotation marks
local yes, no = true, false

query {

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
