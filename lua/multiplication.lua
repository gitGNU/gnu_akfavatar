#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.positive"
require "akfavatar.negative"

-- edit to your needs:
local random_minimum = 1
local random_maximum = 10

-- default language: English -- for translations see below
local msg = {
  encoding            = "UTF-8",
  title               = "Multiply",
  question            = "What to exercise?",
  multiplication      = "Multiplication",
  multiples_of        = "Multiples of ",
  division            = "Division",
  division_by         = "Division by ",
  correct             = "correct",
  wrong               = "wrong",
  continue            = "Do you want to take another exercise?",
  multiplication_sign = "·",
  division_sign       = ":"
}

-- get the main language
os.setlocale("", "all") --> activate local language settings
msg.language = string.lower(string.match(os.setlocale(nil, "ctype"), "^%a+"))

-- translations

-- Deutsch (German)
if msg.language == "de" or msg.language == "german" then
  msg.title          = "Multiplizieren"
  msg.question       = "Was üben?"
  msg.multiplication = "Multiplizieren"
  msg.multiples_of   = "Vielfache von "
  msg.division       = "Teilen"
  msg.division_by    = "Teilen durch "
  msg.correct        = "richtig"
  msg.wrong          = "falsch"
  msg.continue       = "Willst du eine andere Übung machen?"
end

------------------------------------------------------------------
local specific_table = nil
local endRequest = false

-- symbols for the exercise
local multiplication = 1
local division = 2
local exercise = multiplication

function answerposition()
  -- previous line, column 30
  avt.move_xy(30, avt.where_y()-1)
end

function askResult()
  local line = avt.ask()
  if line == "" then endRequest = true end
  return tonumber(line)
end

function AskWhatToExercise()
  avt.tell(msg.question,
          "\n1) ", msg.multiplication,
          " (", random_minimum, "-", random_maximum, ")",
          "\n2) ", msg.multiples_of, "...",
          "\n3) ", msg.division,
          " (", random_minimum, "-", random_maximum, ")",
          "\n4) ", msg.division_by, "...")

  local c = avt.choice(2, 4, "1")

  if c == 1 then
    exercise = multiplication
    specific_table = nil
  elseif c == 2 then
    exercise = multiplication
    avt.set_balloon_size(1, 20)
    repeat
      avt.clear ()
      specific_table = tonumber(avt.ask(msg.multiples_of))
    until specific_table
  elseif c == 3 then
    exercise = division
    specific_table = nil
  elseif c == 4 then
    exercise = division
    avt.set_balloon_size(1, 20)
    repeat
      avt.clear ()
      specific_table = tonumber(avt.ask(msg.division_by))
    until specific_table
  else endRequest = true
  end

  avt.clear()
end

function sayCorrect()
  positive ()
  avt.set_text_color("dark green")
  answerposition ()
  avt.say(msg.correct)
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function sayWrong()
  negative ()
  answerposition ()
  avt.set_text_color("dark red")
  avt.say(msg.wrong)
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function query()
  local counter = 0
  local a, b, r, e
  local isCorrect

  avt.set_balloon_size(4, 40)

  while not endRequest do
    counter = counter + 1

    if specific_table then
      a = specific_table
    else
      a = math.random(random_minimum, random_maximum)
    end

    b = math.random(random_minimum, random_maximum)

    r = a * b;

    -- repeat asking the same question until correct
    repeat
      avt.say(string.format("%2d) ", counter))

      if exercise == multiplication then
        avt.say(a, msg.multiplication_sign, b, "=")
        e = askResult()
        isCorrect = (e == r)
      elseif exercise == division then
        avt.say(r, msg.division_sign, a, "=")
        e = askResult()
        isCorrect = (e == b)
      end

      if not endRequest then
        if isCorrect then sayCorrect() else sayWrong() end
        end
  until isCorrect or endRequest
  end -- while
end

function WantToContinue()
  avt.tell(msg.continue)

  return avt.decide()
end

function initialize()
  avt.set_background_color("tan")
  avt.set_balloon_color("floral white")
  avt.initialize {
    title = "AKFAvatar: " .. msg.title,
    shortname = msg.title,
    avatar = require "akfavatar.teacher",
    audio = true,
    encoding = msg.encoding
    }
  avt.move_in()

  -- initialize the random number generator
  math.randomseed(os.time())
end

-- main program

initialize()

repeat
  endRequest = false
  AskWhatToExercise()
  query()
until not WantToContinue()

avt.move_out()

