#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2009,2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- Note: Since this is a trivial example, you don't have to keep my
-- Copyright notice, but you may replace it with your own.


local avt = require "lua-akfavatar" --> gives access to the avt. comands

avt.encoding("UTF-8")
avt.title("Hello World", "Hello")
avt.set_background_color("tan")
avt.start()
avt.avatar_image("default")
avt.move_in()
avt.set_text_delay() --> activate the slowprint mode (optional)

avt.set_balloon_size(10, 20) --> set the size of the balloon (optional)

-- say something:
avt.say [[
Hello world!
¡Hola mundo!
Bonjour le monde!
Hallo Welt!
Hej Världen!
Καλημέρα κόσμε!
Xin chào thế giới
Здравствуй мир!
]]

avt.newline()
avt.say("π≈", math.pi) --> avt.say accepts strings and numbers
avt.wait_button() --> wait for a button to be pressed
avt.move_out()
