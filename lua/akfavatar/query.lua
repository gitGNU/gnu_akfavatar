-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "lua-akfavatar"
require "akfavatar.positive"
require "akfavatar.negative"

local msg = {}

local function correct()
  avt.show_avatar()
  avt.set_balloon_color("#CFC")
  positive()
  avt.tell(msg.correct)
  avt.wait_button()
  avt.set_balloon_color("floral white")
end

local function wrong(answer)
  if type(answer) == "table" then
    answer = table.concat(answer, "\n")
  end

  avt.show_avatar()
  avt.set_balloon_color("#FCC")
  negative()
  avt.tell(msg.wrong, "\n\n", msg.correction, "\n", answer)
  avt.wait_button()
  avt.set_balloon_color("floral white")
end

function query (qa)
  msg.correct = qa.correct or "That's correct."
  msg.wrong = qa.wrong or "Wrong!"
  msg.correction = qa.correction or "The correct answer:"

  if not avt.initialized()
  then
    avt.initialize{title="query", audio=true}
  else
    avt.initialize_audio()
  end

  for i, q in ipairs(qa) do

    avt.set_balloon_size (4, 80)
    local answer = string.lower(avt.ask(string.format("%2d) %s\n", i, q[1])))

    if type(q[2]) == "table" then
      local is_correct = false
      for i2, a in ipairs(q[2]) do
        if answer==string.lower(a) then 
          correct()
          is_correct = true
          break
        end -- if answer==
      end -- for
      if not is_correct then wrong(q[2]) end
    else -- not a table (string or number allowed
      if answer==string.lower(q[2]) then correct() else wrong(q[2]) end
    end -- if type

  end -- for
end

return query
