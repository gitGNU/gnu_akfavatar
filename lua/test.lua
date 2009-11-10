#!/usr/bin/lua5.1

-- test for lakfavatar

require "lua-avt";

avt.initialize ("Lua-Test", "Lua", avt.import_image_file("teacher.xpm"), 0);
avt.set_avatar_name ("Theora Tester äöü€");
avt.move_in();
avt.set_text_delay();
avt.say("Dies ist ein Test. äöüß€\n");
avt.wait_button();
avt.set_avatar_name (nil);
avt.wait_button();
avt.move_out();
avt.quit();
