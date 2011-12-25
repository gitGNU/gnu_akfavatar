#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010,2011 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

local avt = require "lua-akfavatar"
local lang = require "akfavatar.lang"

local positive = avt.load_audio_file(avt.search("positive.au"))
local negative = avt.load_audio_file(avt.search("negative.au"))
local neutral = avt.load_audio_file(avt.search("neutral.au"))

-- edit to your needs:
local random_minimum = 1
local random_maximum = 10
local maximum_tries = 3

lang.translations {

  ["Multiply"] = {
    de="Multiplizieren" },

  ["What to exercise?"] = {
    de="Was üben?" },

  ["Multiplication"] = {
    de="Multiplizieren" },

  ["Multiples of "] = {
    de="Vielfache von " },

  ["Division"] = {
    de="Teilen" },

  ["Division by "] = {
    de="Teilen durch " },

  ["correct"] = {
    de="richtig" },

  ["wrong"] = {
    de="falsch" },

  ["???"] = {
    en="pardon?",
    de="Wie bitte?" },

  ["Do you want to take another exercise?"] = {
    de="Willst du eine andere Übung machen?" },

  ["×"] = {
    de="·" },

  ["÷"] = {
    de=":" }
}

------------------------------------------------------------------
local L = lang.translate  -- abbreviation
local specific_table = nil
local endRequest = false

local exercise = "multiplication"

function answerposition()
  -- previous line, column 30
  avt.move_xy(30, avt.where_y()-1)
end

function AskWhatToExercise()
  avt.tell(L"What to exercise?",
          "\n1) ", L"Multiplication",
          " (", random_minimum, "-", random_maximum, ")",
          "\n2) ", L"Multiples of " , "...",
          "\n3) ", L"Division",
          " (", random_minimum, "-", random_maximum, ")",
          "\n4) ", L"Division by ", "...")

  local c = avt.choice(2, 4, "1")

  if c == 1 then
    exercise = "multiplication"
    specific_table = nil
  elseif c == 2 then
    exercise = "multiplication"
    avt.set_balloon_size(1, 20)
    repeat
      avt.clear ()
      specific_table = tonumber(avt.ask(L"Multiples of "))
    until specific_table
  elseif c == 3 then
    exercise = "division"
    specific_table = nil
  elseif c == 4 then
    exercise = "division"
    avt.set_balloon_size(1, 20)
    repeat
      avt.clear ()
      specific_table = tonumber(avt.ask(L"Division by "))
    until specific_table
  else endRequest = true
  end

  avt.clear()
end

function sayCorrect()
  positive()
  avt.set_text_color("dark green")
  answerposition()
  avt.say(L"correct")
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function sayWrong()
  negative()
  answerposition()
  avt.set_text_color("dark red")
  avt.say(L"wrong")
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function sayUnknown()
  neutral()
  answerposition()
  avt.set_text_color("gray30")
  avt.say(L"???")
  avt.clear_eol()
  avt.newline()
  avt.normal_text()
end

function askResult(task)
  local result

  repeat
    local line = avt.ask(task)
    if line == "" then endRequest = true end
    result = tonumber(line)
    if not result and not endRequest then sayUnknown() end
  until result or endRequest

  return result
end

function query()
  local counter = 0
  local a, b, c, e
  local tries
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

    c = a * b  --> this is the secret formula ;-)

    tries = 0

    -- repeat asking the same question until correct
    repeat
      if exercise == "multiplication" then
        e = askResult(string.format("%2d) %d%s%d=", counter, a, L"×", b))
        isCorrect = (e == c)
      elseif exercise == "division" then
        e = askResult(string.format("%2d) %d%s%d=", counter, c, L"÷", a))
        isCorrect = (e == b)
      end

      if not endRequest then
        if isCorrect then sayCorrect() else sayWrong() end
      end

      tries = tries + 1
      if tries >= maximum_tries and not isCorrect then -- help
        avt.set_text_color("dark red")
        avt.inverse(true)
        avt.bold(true)
        if exercise == "multiplication" then
          avt.say(string.format("%2d) %d%s%d=%d", counter, a, L"×", b, c))
        elseif exercise == "division" then
          avt.say(string.format("%2d) %d%s%d=%d ", counter, c, L"÷", a, b))
        end --> if exercise
        avt.normal_text()
        avt.newline()
        isCorrect = true  --> the teacher is always right ;-)
      end --> if tries >= maximum_tries
    until isCorrect or endRequest
  end --> while not endRequest
end

function WantToContinue()
  avt.tell(L"Do you want to take another exercise?")

  return avt.decide()
end

function initialize()
  avt.set_background_color("tan")
  avt.set_balloon_color("floral white")
  avt.initialize {
    title = "AKFAvatar: " .. L"Multiply",
    shortname = L"Multiply",
    avatar = avt.search("teacher.xpm"),
    audio = true,
    encoding = "UTF-8"
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

