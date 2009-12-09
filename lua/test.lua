#!/usr/bin/lua

require "lua-akfavatar"

avt.initialize("Lua-Test", "Test")
avt.set_avatar_name("Theora Tester äöü€")
avt.move_in()
file = avt.file_selection ()
avt.set_text_delay()
avt.say("Dies ist ein Test. äöüß€\n")
avt.say("Datei: " .. file)
avt.wait_button()
avt.move_out()
avt.quit()
