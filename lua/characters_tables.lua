#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

-- this gives an impression over which characters are available
-- note: the version for lower resolutions has less characters!

require "lua-akfavatar"

avt.initialize{}

local items = {
  "End",
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


local function block_list(f, t)
  local list = "╔═══╤════════╤══════════════╗\n" ..
               "║ c │ number │ UTF-8 (Lua)  ║\n" ..
               "╠═══╪════════╪══════════════╣\n"
  
  for unicode = f, t do
    if avt.printable(unicode) then
      local u8 = avt.unicode_to_utf8(unicode)
      local hex = string.format("0x%X", unicode)
      local u8_esc = ""
      for i=1, string.len(u8) do
        u8_esc = u8_esc .. string.format("\\%03d", string.byte(u8, i))
      end

      -- u8 would break in string.format when it is \0
      list = list .. "║ " .. u8 ..
             string.format(" │ %-6s │ %-12s ║\n", hex, u8_esc)
    end
  end

  list = list .. "╚═══╧════════╧══════════════╝\n"

  avt.set_balloon_width(29)
  avt.pager(list)
end

local function show_item(item)
  local f, t

  -- get the range from the item string
  f, t = string.match(item, "%(U%+(%x+)%-U%+(%x+)%)$")
  f, t = tonumber(f, 16), tonumber(t, 16)

  avt.show_avatar()
  block_list(f, t)
end


local menu_item

repeat
  avt.set_text_delay(0)
  avt.set_balloon_size()
  avt.clear()
  menu_item = avt.long_menu (items)

  if menu_item ~= 1 then
    show_item(items[menu_item])
  end

until menu_item == 1

