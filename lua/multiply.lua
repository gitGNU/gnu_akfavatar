#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

-- edit to your needs:
local a_minimum = 2
local a_maximum = 9
local b_minimum = 2
local b_maximum = 9

local multiplicationSign = "·"
local divisionSign       = ":"

-- get the main language
local locale = os.setlocale ("", "ctype")

-- the language is at the beginning of the locale
if string.find(locale, "^de") or string.find(locale, "^[Gg]erman")
then -- Deutsch (German)
  question         = "Was üben?"
  t_multiplication = "multiplizieren"
  t_division       = "dividieren"
  correct          = "richtig"
  wrong            = "falsch"
  continue         = "Willst du weiter machen?"
else -- default: English
  question         = "what to exercise?"
  t_multiplication = "multiplication"
  t_division       = "division"
  correct          = "correct"
  wrong            = "wrong"
  continue         = "do you want to continue?"
end

------------------------------------------------------------------

-- symbols for the exercise
local multiplication = 1
local division = 2

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
  avt.set_balloon_size (3, 40)
  avt.clear()
  avt.say(question)
  avt.newline()

  avt.say("1) ", t_multiplication, "\n2) ", t_division)

  local c = avt.choice(2, 2, "1")

  if c == 1 then exercise = multiplication
    elseif c == 2 then exercise = division
    else endRequest = true
  end

  avt.clear()
end

function sayCorrect()
  avt.set_text_color("dark green")
  avt.say(correct)
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function sayWrong()
  avt.bell () -- make a sound
  avt.set_text_color("dark red")
  avt.say(wrong)
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function query()
  local counter = 0
  local a, b, r, e

  avt.set_balloon_size(4, 40)

  while not endRequest do
    counter = counter + 1
    a = math.random(a_minimum, a_maximum)
    b = math.random(b_minimum, b_maximum)
    r = a * b;

    -- repeat asking the same question until correct
    repeat
      avt.say(string.format("%2d) ", counter))

      if exercise == multiplication then
        avt.say(a, multiplicationSign, b, "=")
        e = askResult()
        isCorrect = (e == r)
      elseif exercise == division then
        avt.say(r, divisionSign, a, "=")
        e = askResult()
        isCorrect = (e == b)
      end

      if not endRequest then
        answerposition ()
        if isCorrect then sayCorrect() else sayWrong() end
        end
  until isCorrect or endRequest
  end -- while
end

function WantToContinue()
  avt.clear()
  avt.say(continue)

  return avt.decide()
end

function initialize()
  avt.set_background_color("tan")
  avt.set_balloon_color("floral white")
  avt.initialize {
    title = "AKFAvatar: multiply",
    shortname = "multiply",
    avatar = require "akfavatar.teacher",
    audio = true,
    encoding = "UTF-8"
    }
  avt.move_in()

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

