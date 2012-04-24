#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- this gives an impression over which characters are available
-- note: the version for lower resolutions has less characters!

local avt = require "lua-akfavatar"

avt.encoding("UTF-8")
avt.title("characters overview")
avt.start()

local items = {
  "End",
  "All characters in a slowprint demo",
  "Basic Latin (U+0000-U+007F)",
  "Latin-1 Supplement (U+0080-U+00FF)",
  "Latin Extended-A (U+0100-U+017F)",
  "Latin Extended-B (U+0180-U+024F)",
  "IPA Extensions (U+0250-U+02AF)",
  "Spacing Modifier Letters (U+02B0-U+02FF)",
  "Combining Diacritical Marks (U+0300-U+036F)",
  "Greek and Coptic (U+0370-U+03FF)",
  "Cyrillic (U+0400-U+04FF)",
  "Cyrillic Supplementary (U+0500-U+052F)",
  "Armenian (U+0530-U+058F)",
  "Hebrew (U+0590-U+05FF)",
  "Thai (U+0E00-U+0E7F)",
  "Georgian (U+10A0-U+10FF)",
  "Ethiopic (U+1200-U+137F)",
  "Cherokee (U+13A0-U+13FF)",
  "Unified Canadian Aboriginal Syllabics (U+1400-U+167F)",
  "Runic (U+16A0-U+16FF)",
  "Latin Extended Additional (U+1E00-U+1EFF)",
  "Greek Extended (U+1F00-U+1FFF)",
  "General Punctuation (U+2000-U+206F)",
  "Superscripts and Subscripts (U+2070-U+209F)",
  "Currency Symbols (U+20A0-U+20CF)",
  "Combining Diacritical Marks for Symbols (U+20D0-U+20FF)",
  "Letterlike Symbols (U+2100-U+214F)",
  "Number Forms (U+2150-U+218F)",
  "Arrows (U+2190-U+21FF)",
  "Mathematical Operators (U+2200-U+22FF)",
  "Miscellaneous Technical (U+2300-U+23FF)",
  "Control Pictures (U+2400-U+243F)",
  "Optical Character Recognition (U+2440-U+245F)",
  "Enclosed Alphanumerics (U+2460-U+24FF)",
  "Box Drawing (U+2500-U+257F)",
  "Block Elements (U+2580-U+259F)",
  "Geometric Shapes (U+25A0-U+25FF)",
  "Miscellaneous Symbols (U+2600-U+26FF)",
  "Dingbats (U+2700-U+27BF)",
  "Miscellaneous Mathematical Symbols-A (U+27C0-U+27EF)",
  "Supplemental Arrows-A (U+27F0-U+27FF)",
  "Braille Patterns (U+2800-U+28FF)",
  "Supplemental Mathematical Operators (U+2A00-U+2AFF)",
  "Free block (U+2C00-U+2E7F)",
  "CJK Symbols and Punctuation (U+3000-U+303F)",
  "Alphabetic Presentation Forms (U+FB00-U+FB4F)",
  "Combining Half Marks (U+FE20-U+FE2F)",
  "Halfwidth and Fullwidth Forms (U+FF00-U+FFEF)",
  "Specials (U+FFF0-U+FFFF)"
}


local function block(f, t)
  for i = f, t do
    if avt.printable(i) then
      avt.say_unicode(i, 0x0020)
    end
  end

  avt.wait_button()
end

local function all_slowprint()
  avt.set_text_delay()
  block(0x0000, 0xFFFF)
  -- note: Unicode copdepoints can be up to 0x10FFFF,
  -- but the font in AKFAvatar covers "just" the BMP
end

local function show_item(item)
  local f, t

  -- get the range from the item string
  f, t = string.match(item, "%(U%+(%x+)%-U%+(%x+)%)$")
  f, t = tonumber(f, 16), tonumber(t, 16)

  avt.clear()
  avt.underlined(true)
  avt.say(item)
  avt.underlined(false)
  avt.newline()
  avt.newline()
  block(f, t)
end


local menu_item

repeat
  avt.set_text_delay(0)
  avt.clear()
  menu_item = avt.long_menu (items)

  if menu_item == 1 then
    break
  elseif menu_item == 2 then
    avt.subprogram(all_slowprint)
  else
    show_item(items[menu_item])
  end

until menu_item == 1
