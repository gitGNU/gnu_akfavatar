#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010,2011,2012,2013 Andreas K. Foerster <akf@akfoerster.de>
-- License: GPL version 3 or later

-- this gives an impression over which characters are available
-- note: the version for lower resolutions has less characters!

local avt = require "lua-akfavatar"

avt.encoding("UTF-8")
avt.title("characters tables")
avt.start()

local function block_list(f, t)
  local list = "╔═══╤════════╤══════════╤══════════════╗\n" ..
               "║ c │ number │ XML/HTML │    UTF-8     ║\n" ..
               "╠═══╪════════╪══════════╪══════════════╣\n"

  for unicode = f, t do
    if avt.printable(unicode) then
      local u8 = avt.toutf8(unicode)
      local hex = string.format("0x%X", unicode)
      local xml = string.format("&#x%X;", unicode)

      local u8_c = ""
      for i=1, string.len(u8) do
        u8_c = u8_c .. string.format("\\x%02X", string.byte(u8, i))
      end

      -- add a base character for combining
      if avt.combining(unicode) then u8 = "\xE2\x97\x8C"..u8; end

      -- u8 would break in string.format when it is \0
      list = list .. "║ " .. u8 ..
             string.format(" │ %-6s │ %-8s │ %-12s ║\n", hex, xml, u8_c)
    end
  end

  list = list .. "╚═══╧════════╧══════════╧══════════════╝"

  avt.set_balloon_width(40)
  avt.pager(list)
end

repeat
  avt.set_text_delay(0)
  avt.set_balloon_size()
  avt.clear()

  local v1, v2 = avt.menu {
    {"End", "end"},
    {"Basic Latin (U+0000-U+007F)", 0x0000, 0x007F},
    {"Latin-1 Supplement (U+0080-U+00FF)", 0x0080, 0x00FF},
    {"Latin Extended-A (U+0100-U+017F)", 0x0100, 0x017F},
    {"Latin Extended-B (U+0180-U+024F)", 0x0180, 0x024F},
    {"IPA Extensions (U+0250-U+02AF)", 0x0250, 0x02AF},
    {"Spacing Modifier Letters (U+02B0-U+02FF)", 0x02B0, 0x02FF},
    {"Combining Diacritical Marks (U+0300-U+036F)", 0x0300, 0x036F},
    {"Greek and Coptic (U+0370-U+03FF)", 0x0370, 0x03FF},
    {"Cyrillic (U+0400-U+04FF)", 0x0400, 0x04FF},
    {"Cyrillic Supplementary (U+0500-U+052F)", 0x0500, 0x052F},
    {"Armenian (U+0530-U+058F)", 0x0530, 0x058F},
    {"Hebrew (U+0590-U+05FF)", 0x0590, 0x05FF},
    {"Thai (U+0E00-U+0E7F)", 0x0E00, 0x0E7F},
    {"Georgian (U+10A0-U+10FF)", 0x10A0, 0x10FF},
    {"Ethiopic (U+1200-U+137F)", 0x1200, 0x137F},
    {"Cherokee (U+13A0-U+13FF)", 0x13A0, 0x13FF},
    {"Unified Canadian Aboriginal Syllabics (U+1400-U+167F)", 0x1400, 0x167F},
    {"Runic (U+16A0-U+16FF)", 0x16A0, 0x16FF},
    {"Latin Extended Additional (U+1E00-U+1EFF)", 0x1E00, 0x1EFF},
    {"Greek Extended (U+1F00-U+1FFF)", 0x1F00, 0x1FFF},
    {"General Punctuation (U+2000-U+206F)", 0x2000, 0x206F},
    {"Superscripts and Subscripts (U+2070-U+209F)", 0x2070, 0x209F},
    {"Currency Symbols (U+20A0-U+20CF)", 0x20A0, 0x20CF},
    {"Combining Diacritical Marks for Symbols (U+20D0-U+20FF)", 0x20D0, 0x20FF},
    {"Letterlike Symbols (U+2100-U+214F)", 0x2100, 0x214F},
    {"Number Forms (U+2150-U+218F)", 0x2150, 0x218F},
    {"Arrows (U+2190-U+21FF)", 0x2190, 0x21FF},
    {"Mathematical Operators (U+2200-U+22FF)", 0x2200, 0x22FF},
    {"Miscellaneous Technical (U+2300-U+23FF)", 0x2300, 0x23FF},
    {"Control Pictures (U+2400-U+243F)", 0x2400, 0x243F},
    {"Optical Character Recognition (U+2440-U+245F)", 0x2440, 0x245F},
    {"Enclosed Alphanumerics (U+2460-U+24FF)", 0x2460, 0x24FF},
    {"Box Drawing (U+2500-U+257F)", 0x2500, 0x257F},
    {"Block Elements (U+2580-U+259F)", 0x2580, 0x259F},
    {"Geometric Shapes (U+25A0-U+25FF)", 0x25A0, 0x25FF},
    {"Miscellaneous Symbols (U+2600-U+26FF)", 0x2600, 0x26FF},
    {"Dingbats (U+2700-U+27BF)", 0x2700, 0x27BF},
    {"Miscellaneous Mathematical Symbols-A (U+27C0-U+27EF)", 0x27C0, 0x27EF},
    {"Supplemental Arrows-A (U+27F0-U+27FF)", 0x27F0, 0x27FF},
    {"Braille Patterns (U+2800-U+28FF)", 0x2800, 0x28FF},
    {"Supplemental Mathematical Operators (U+2A00-U+2AFF)", 0x2A00, 0x2AFF},
    {"Free block (U+2C00-U+2E7F)", 0x2C00, 0x2E7F},
    {"CJK Symbols and Punctuation (U+3000-U+303F)", 0x3000, 0x303F},
    {"Alphabetic Presentation Forms (U+FB00-U+FB4F)", 0xFB00, 0xFB4F},
    {"Combining Half Marks (U+FE20-U+FE2F)", 0xFE20, 0xFE2F},
    {"Halfwidth and Fullwidth Forms (U+FF00-U+FFEF)", 0xFF00, 0xFFEF},
    {"Specials (U+FFF0-U+FFFF)", 0xFFF0, 0xFFFF}
  }

  if v1 ~= "end" then
    block_list(v1, v2)
  end

until v1 == "end"

