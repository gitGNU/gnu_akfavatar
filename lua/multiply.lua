#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"

-- edit to your needs:
local a_minimum = 1
local a_maximum = 10
local b_minimum = 1
local b_maximum = 10

local multiplicationSign = "·"
local divisionSign       = ":"

-- get the main language
local locale = os.setlocale ("", "ctype")

-- the language is at the beginning of the locale
if string.find(locale, "^de") or string.find(locale, "^[Gg]erman")
then -- Deutsch (German)
  question         = "Was üben?"
  t_multiplication = "Multiplizieren (1-10)"
  t_multiples_of   = "Vielfache von "
  t_division       = "Teilen (1-10)"
  t_division_by    = "Teilen durch "
  correct          = "richtig"
  wrong            = "falsch"
  continue         = "Willst du weiter machen?"
else -- default: English
  question         = "What to exercise?"
  t_multiplication = "Multiplication (1-10)"
  t_multiples_of   = "Multiples of "
  t_division       = "Division (1-10)"
  t_division_by    = "Division by "
  correct          = "correct"
  wrong            = "wrong"
  continue         = "Do you want to continue?"
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
  avt.tell(question,
          "\n1) ", t_multiplication,
          "\n2) ", t_multiples_of, "...",
          "\n3) ", t_division,
          "\n4) ", t_division_by, "...")

  local c = avt.choice(2, 4, "1")

  if c == 1 then exercise = multiplication
    elseif c == 2 then
      exercise = multiplication
      avt.set_balloon_size(1, 20)
      repeat
        avt.clear ()
        a_minimum = tonumber(avt.ask(t_multiples_of))
      until a_minimum
    a_maximum = a_minimum
    elseif c == 3 then exercise = division
    elseif c == 4 then
      exercise = division
      avt.set_balloon_size(1, 20)
      repeat
        avt.clear ()
        a_minimum = tonumber(avt.ask(t_division_by))
      until a_minimum
    a_maximum = a_minimum
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

    if a_minimum == a_maximum then
      a = a_minimum
    else
      a = math.random(a_minimum, a_maximum)
    end

    if b_minimum == b_maximum then
      b = b_minimum
    else
      b = math.random(b_minimum, b_maximum)
    end

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
  avt.tell(continue)

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

