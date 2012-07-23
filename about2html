#!/usr/bin/awk -f

# Convert about-file into HTML-code
#
# Example-Usage:
# ./about2html audioplayer.en.about > audioplayer.en.html
#
# Copyright (c) 2011,2012 Andreas K. Foerster
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

NR == 1 {
  name = FILENAME
  sub(/^.*[\\\/]/, "", name) # strip path

  print "<!DOCTYPE html>\n"
  print "<html>\n<head>"
  print "<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>"
  print "<title>" name "</title>"
  print "</head>\n\n<body>\n"
  print "<pre style='width:80ex; margin-left:auto; margin-right:auto;'>"
}

{
  gsub(/&/, "\&amp;")
  gsub(/</, "\&lt;")
  gsub(/>/, "\&gt;")

  # Overstrike with UTF-8 is complicated :-(
  gsub(/_\b[^\x80-\xBF][\x80-\xBF]*/, "<i>&</i>")
  gsub(/_\b/, "")
  gsub(/<\/i><i>/, "")
  gsub(/<\/i> <i>/, " ")

  gsub(/[^\x80-\xBF][\x80-\xBF]*\b[^\x80-\xBF][\x80-\xBF]*/, "<b>&</b>")
  gsub(/[^\x80-\xBF][\x80-\xBF]*\b/, "")
  gsub(/<\/b><b>/, "")
  gsub(/<\/b> <b>/, " ")

  # page breaks
  gsub(/[\f\x1C-\x1F]/, "<hr>")

  print
}

END {
  print "</pre>\n"
  print "</body>\n</html>"
}
