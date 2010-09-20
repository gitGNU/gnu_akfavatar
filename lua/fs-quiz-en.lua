#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.query"

avt.initialize {
  title="Free Software Quiz",
  avatar=require "akfavatar.gnu_head",
  encoding="UTF-8",
  audio=true
}

query {

  -- This is not really needed for English, but crucial for other languages
  correct = "That's correct.",
  wrong = "Wrong!",
  again = "Try again?",
  correction = "The correct answer:",


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

  {"What is the name of the shell of the GNU projekt? (shortname)",
   "Bash"},

  {"What does “GCC” stand for?",
   "GNU Compiler Collection", "GNU C Compiler"},

  {"With which kernel is the GNU system commonly used?",
   "Linux"},

  {"What is the name of the developer of the kernel Linux?",
   "Linus Torvalds", "Linus B. Torvalds", "Linus Benedict Torvalds", "Torvalds"},

}
