#!/usr/bin/env lua-akfavatar

-- Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
-- License: GPL version 3 or later

require "base64"

local i, o, infile, outfile

if arg[1]
  then infile = arg[1]
  else 
    require "lua-akfavatar"
    avt.initialize{title="Base 64 encoder", shortname="base64"}
    infile = avt.file_selection()
  end

outfile = infile .. ".b64"

i = io.open(infile, "rb") --> binary mode!
if not i then error('cannot read file "'..infile..'".', 0) end

o = io.open(outfile, "w") --> text mode
if not o then error('cannot write file "'..outfile..'".', 0) end

o:write(base64.encode(i:read("*all")))

i:close()
o:close()

if not arg[1] then
  avt.tell('created file "', outfile, '".')
  avt.wait_button()
end
