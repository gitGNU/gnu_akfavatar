-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.positive"
require "akfavatar.negative"

local count = { questions = 0, right = 0 }

-- These messages can be changed with the function querymessages{}
local msg = {
  correct = "That's correct.",
  wrong = "Wrong!",
  again = "Try again?",
  correction = "The correct answer:",
  questions = "Questions",
  correctly_answered = "correct"
}

function questionarymessages(m)
  for key, message in pairs(m) do
    msg[key] = message
  end
end

local function normalize(s)
  -- make lowercase and remove leading and trailing spaces
  return string.lower(string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

local function correct()
  positive()
  count.right = count.right + 1
  avt.show_avatar()
  avt.set_balloon_color("#CFC")
  avt.tell(msg.correct)
  avt.wait_button()

  return false --> correct answer
end

local function wrong(q)
  negative()
  avt.show_avatar()
  avt.set_balloon_color("#FCC")
  avt.tell(msg.wrong, "\n\n", msg.again)
  if avt.decide()
  then return true --> wrong, try again
  else
    avt.set_balloon_color("floral white")
    avt.tell(msg.correction, "\n- ", table.concat(q, "\n- ", 2))
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
      avt.tell(msg.wrong)
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
  avt.tell(string.format("%s: %d, %s: %d (%d%%)",
    msg.questions, count.questions,
    msg.correctly_answered, count.right,
    count.right * 100 / count.questions))
  avt.wait_button()
end

function questionary(qa)
  count.questions, count.right = 0, 0

  if not avt.initialized()
  then
    avt.initialize{title="AKFAvatar: query", audio=true}
  else
    avt.initialize_audio()
  end

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

--for yes/no questions
yes, no = true, false

return questionary

