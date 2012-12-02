#!/usr/bin/env lua-akfavatar

-- This file is dedicated to the Public Domain (CC0)
-- http://creativecommons.org/publicdomain/zero/1.0/

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
