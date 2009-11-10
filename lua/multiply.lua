#!/usr/bin/env lua
-- simple multiplication

--[[
simple multiplication
Copyright (c) 2009 Andreas K. Foerster <info@akfoerster.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
]]

require "lua-avt"

-- edit to your needs:
a_minimum = 2
a_maximum = 9
b_minimum = 2
b_maximum = 9

-- English
--[[
question         = "what to exercise?"
t_multiplication = "multiplication"
t_division       = "division"
correct          = "correct"
wrong            = "wrong"
continue         = "do you want to continue?"
]]

-- Deutsch
question         = "Was üben?"
t_multiplication = "multiplizieren"
t_division       = "dividiren"
correct          = "richtig"
wrong            = "falsch"
continue         = "Willst du weiter machen?"

multiplicationSign = "·"
divisionSign       = "÷"

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
  local c

  avt.set_balloon_size (4, 40)
  avt.clear()
  avt.say(question)
  avt.newline()
  avt.newline()

  avt.say(string.format("1) %s\n2) %s", t_multiplication, t_division))

  c = avt.choice(3, 2, "1")

  if c == 1 then exercise = multiplication
    elseif c == 2 then exercise = division
    else endRequest = true
  end

  avt.clear()
end

function sayCorrect()
  avt.play_audio(correctsnd, false)
  avt.set_text_color ("dark green")
  avt.say(correct)
  avt.newline()
  avt.normal_text()
end

function sayWrong()
  avt.play_audio(wrongsnd, false);
  avt.set_text_color ("dark red")
  avt.say(wrong)
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
        avt.say(string.format("%d %s %d = ", a, multiplicationSign, b))
        e = askResult()
        isCorrect = (e == r)
      elseif exercise == division then
        avt.say(string.format("%d %s %d = ", r, divisionSign, a))
        e = askResult()
        isCorrect = (e == b)
      end

      if not endRequest and (e ~= -1) then
        answerposition ()
        if isCorrect then sayCorrect() else sayWrong() end
        end
  until isCorrect or endRequest
  end -- while
end

function WantToContinue()
  if avt.get_status() ~= 0 then return false end
  
  avt.clear()
  avt.say(continue)

  -- an end-request is a negative decision, 
  -- so we dont' have to check avt.get_status here again
  return avt.decide()
end

-- main program

avt.set_background_color("tan")
avt.set_balloon_color("floral white")

avt.initialize("AKFAvatar: multiply", "multiply", 
               avt.import_image_file("teacher.xpm"), 0)
avt.initialize_audio()
avt.encoding("UTF-8") -- UTF-8 is the default

avt.move_in()

correctsnd = avt.load_audio_file ("positive.au")
wrongsnd = avt.load_audio_file ("negative.au")

math.randomseed(os.time())

repeat
  endRequest = false
  AskWhatToExercise()
  query()
until not WantToContinue()

avt.free_audio (correctsnd)
avt.free_audio (wrongsnd)

-- Avoid waiting for a keypress
avt.move_out()
avt.quit()
