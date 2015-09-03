#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010,2011,2012,2013 Andreas K. Foerster <akf@akfoerster.de>
-- License: GPL version 3 or later

local avt = require "lua-akfavatar"

local image_file = "teacher.xpm"

-- colors can be names or numbers
local background_color = "tan"
local board_color = 0x005500
local text_color = "white"
local correct_color = 0xCCFFCC
local wrong_color = 0xFFCCCC

local positive = avt.load_audio_file(avt.search("positive.au")) or avt.silent()
local negative = avt.load_audio_file(avt.search("negative.au")) or avt.alert()
local question = avt.load_audio_file(avt.search("question.au")) or avt.alert()

-- edit to your needs:
local random_minimum = 1
local random_maximum = 10
local maximum_tries = 2

avt.translations = {

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
local L = avt.translate  -- abbreviation
local specific_table = nil
local endRequest = false
local board_bottom

local exercise = "multiplication"

local function answerposition()
  -- previous line, column 30
  avt.move_xy(30, avt.where_y()-1)
end

local function AskWhatToExercise()
  avt.clear()
  avt.set_text_delay(0)
  avt.say(L"What to exercise?", "\n\n",
          "1) ", L"Multiplication",
          " (", random_minimum, "-", random_maximum, ")\n",
          "2) ", L"Multiples of " , "...\n",
          "3) ", L"Division",
          " (", random_minimum, "-", random_maximum, ")\n",
          "4) ", L"Division by ", "...")

  local c = avt.choice(3, 4, "1")

  if c == 1 then
    exercise = "multiplication"
    specific_table = nil
  elseif c == 2 then
    exercise = "multiplication"

    repeat
      avt.clear ()
      specific_table = tonumber(avt.ask(L"Multiples of "))
    until specific_table
  elseif c == 3 then
    exercise = "division"
    specific_table = nil
  elseif c == 4 then
    exercise = "division"

    repeat
      avt.clear ()
      specific_table = tonumber(avt.ask(L"Division by "))
    until specific_table
  else endRequest = true
  end

  avt.clear()
  avt.set_text_delay()
end

local function sayCorrect()
  positive()
  answerposition()
  avt.set_text_color(correct_color)
  avt.say(L"correct")
  avt.clear_eol()
  avt.set_text_color(text_color)
  avt.newline()
end

local function sayWrong()
  negative()
  answerposition()
  avt.set_text_color(wrong_color)
  avt.say(L"wrong")
  avt.clear_eol()
  avt.set_text_color(text_color)
  avt.newline()
end

local function sayUnknown()
  question()
  answerposition()
  avt.say(L"???")
  avt.clear_eol()
  avt.newline()
end

local function eventually_clear_board()
  if avt.where_y() > board_bottom then
    avt.wait(2.5)
    avt.clear()
  end
end

local function askResult(task)
  local result

  repeat
    local line = avt.ask(task)
    if line == "" then endRequest = true end
    result = tonumber(line)
    if not result and not endRequest then sayUnknown() end
  until result or endRequest

  return result
end

local function query()
  local counter = 0
  local a, b, c, e
  local tries
  local isCorrect

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
      eventually_clear_board()

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
        eventually_clear_board()
        avt.set_text_color(wrong_color)
        avt.inverse(true)
        avt.bold(true)
        if exercise == "multiplication" then
          avt.say(string.format("%2d) %d%s%d=%d ", counter, a, L"×", b, c))
        elseif exercise == "division" then
          avt.say(string.format("%2d) %d%s%d=%d ", counter, c, L"÷", a, b))
        end --> if exercise
        avt.inverse(false)
        avt.bold(false)
        avt.set_text_color(text_color)
        avt.newline()
        isCorrect = true  --> the teacher is always right ;-)
      end --> if tries >= maximum_tries
    until isCorrect or endRequest
  end --> while not endRequest
end

local function WantToContinue()
  avt.clear()
  avt.say(L"Do you want to take another exercise?")

  return avt.decide()
end

local function initialize()
  avt.encoding("UTF-8")
  avt.title("AKFAvatar: " .. L"Multiply", L"Multiply")
  avt.set_background_color(background_color)
  avt.start()
  avt.start_audio()

  avt.avatar_image_file(avt.search(image_file))
  avt.set_avatar_mode("footer")
  avt.move_in()

  avt.set_balloon_color(board_color)
  avt.set_text_color(text_color)
  avt.set_balloon_size (0, 80)
  board_bottom = avt.get_max_y()
  avt.viewport(20, 1, 40, board_bottom)
  avt.set_scroll_mode(-1) --> board gets cleared manually
  avt.set_text_delay()

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
