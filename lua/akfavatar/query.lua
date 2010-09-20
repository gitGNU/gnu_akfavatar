-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.positive"
require "akfavatar.negative"

local msg = {
  correct = "That's correct.",
  wrong = "Wrong!",
  again = "Try again?",
  correction = "The correct answer:"
}

function querymessages(m)
  if m.correct then msg.correct = m.correct end
  if m.wrong then msg.wrong = m.wrong end
  if m.again then msg.again = m.again end
  if m.correction then msg.correction = m.correction end
end

local function correct()
  avt.show_avatar()
  avt.set_balloon_color("#CFC")
  positive()
  avt.tell(msg.correct)
  avt.wait_button()

  return true --> correct answer
end

local function wrong(q)
  avt.show_avatar()
  avt.set_balloon_color("#FCC")
  negative()
  avt.tell(msg.wrong, "\n\n", msg.again)

  if avt.decide()
  then return false --> wrong, try again
  else
    avt.set_balloon_color("floral white")
    avt.tell(msg.correction, "\n- ", table.concat(q, "\n- ", 2))
    avt.wait_button()
    return true --> correct answer shown
  end
end

function query(qa)
  if not avt.initialized()
  then
    avt.initialize{title="query", audio=true}
  else
    avt.initialize_audio()
  end

  for i, q in ipairs(qa) do
    local is_correct = false
    repeat
      avt.show_avatar()
      avt.set_balloon_color("floral white")
      avt.set_balloon_size(4, 80)
      avt.say(string.format("%d) %s", i, q[1]))
      avt.move_xy(1, 4)
      local answer = string.lower(avt.ask())

      for a=2,#q do --> look through all answers
        if answer==string.lower(q[a]) then
          is_correct = correct()
          break
        end -- if answer==
      end -- for

      if not is_correct then is_correct = wrong(q) end

  until is_correct --> either correcly answerd, or the answer was shown
  end -- for
end -- function

return query
