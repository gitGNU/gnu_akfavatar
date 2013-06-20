-- Copyright (c) 2010,2011,2012,2013 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

local avt = require "lua-akfavatar"

avt.translations["That's correct."] = {
    de = "Das ist richtig."}

avt.translations["Wrong!"] = {
    de = "Falsch!"}

avt.translations["Try again?"] = {
    de = "Nochmal versuchen?"}

avt.translations["The correct answer:"] = {
    de = "Die richtige Antwort lautet:"}

avt.translations["Questions: %d, correct: %d (%d%%)"] = {
    de = "Fragen: %d, davon richtig: %d (%d%%)"}

local positive = avt.load_audio_file(avt.search("positive.au")) or avt.silent()
local negative = avt.load_audio_file(avt.search("negative.au")) or avt.alert()
local count = { questions = 0, right = 0 }
local L = avt.translate

local function normalize(s)
  -- make lowercase and remove leading and trailing spaces
  return string.lower(string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

local function correct()
  positive()
  count.right = count.right + 1
  avt.show_avatar()
  avt.set_balloon_color("#CFC")
  avt.tell(L"That's correct.")
  avt.wait_button()

  return false --> correct answer
end

local function wrong(q)
  negative()
  avt.show_avatar()
  avt.set_balloon_color("#FCC")
  avt.tell(L"Wrong!", "\n\n", L"Try again?")
  if avt.decide()
  then return true --> wrong, try again
  else
    avt.set_balloon_color("floral white")
    avt.tell(L"The correct answer:", "\n- ", table.concat(q, "\n- ", 2))
    avt.wait_button()
    return false --> correct answer shown
  end
end

local function ask_boolean(b)
  if b == avt.decide()
    then correct()
    else
      avt.show_avatar()
      avt.set_balloon_color("#FCC")
      negative()
      avt.tell(L"Wrong!")
      avt.wait_button()
  end

  return false --> the correct answer is implicitly clear
end

local function ask_input(q)
  local again = true
  local answer = normalize(avt.ask())

  for a=2, #q do --> look through all answers
    if answer==normalize(q[a]) then
      again = correct()
      break
    end -- if answer==
  end -- for

  if again then again = wrong(q) end
  return again
end

local function show_results()
  avt.set_balloon_color("floral white")
  avt.tell(string.format(L"Questions: %d, correct: %d (%d%%)",
    count.questions, count.right,
    count.right * 100 / count.questions))
  avt.wait_button()
end

local function questionary(qa)
  local myavatar

  count.questions, count.right = 0, 0

  if qa.avatar=="default" or qa.avatar=="none" then
    myavatar = qa.avatar
  elseif qa.avatar then
    myavatar = avt.search(qa.avatar)
  end

  if not avt.started()
  then
    avt.title(qa.title)
    avt.start()
    avt.start_audio()
    if not avt.avatar_image(myavatar) then
      avt.avatar_image_file(myavatar)
      end
  else
    avt.start_audio()
    if myavatar then
      if not avt.avatar_image(myavatar) then
        avt.avatar_image_file(myavatar)
      end
    end
  end

  avt.set_avatar_mode "say"
  avt.encoding "UTF-8"

  if qa.lang then avt.language = qa.lang end

  for i, q in ipairs(qa) do
    local again
    count.questions = count.questions + 1

    repeat
      avt.show_avatar()
      avt.set_balloon_color("floral white")
      avt.set_balloon_size(4, 80)
      avt.say(i, ") ", q[1])
      avt.move_xy(1, 4)

      if type(q[2]) == "boolean"
        then again = ask_boolean(q[2])
        else again = ask_input(q)
      end
    until not again
  end -- for

  show_results()
end -- function

return questionary

