#!/usr/bin/awk -f

# data2c - Convert binary data file into C-code
#
# Copyright (c) 2009,2012 Andreas K. Foerster
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

function help(code)
{
  if (code != 0) print "* ERROR\n"

  print "data2c - by Andreas K. Foerster\n"
  print "Example-Usages:"
  print "  ./data2c audio.au > audio1.c"
  print "  ./data2c -v name=audio1 audio.au > audio1.c"
  print "  cat audio.au | ./data2c -v name=audio1 - > audio1.c"

  exit code
}

BEGIN \
{
  OD = "od"

  if (ARGC != 2) help(0)

  data = ARGV[1]
  cmd = OD " -t x1 -v " data

  # if no name is given, deduce it from the filename
  if (name == "")
    {
      if (data == "-") help(1)

      name = data
      gsub(/[-\.:&~]/, "_", name) # replace some chars
    }

  size = 0

  while ((cmd | getline) > 0)
    {
      if (NF > 1)
      {
        if (size == 0) print "const char " name "[] ="

        printf "\t\""

        for (i = 2; i <= NF; i++)
          {
            printf "\\x%s", $i
            size++
          }

        print "\""
      }
    }

  close(cmd)

  if (size > 0)
    print "\t;\n\nconst int " name "_size = " size ";"

  exit
}
