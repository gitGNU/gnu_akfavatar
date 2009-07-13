/* 
 * avtmsg - message output for avatarsay
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is part of AKFAvatar
 *
 * AKFAvatar is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AKFAvatar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "avtaddons.h"
#include <stdio.h>
#include <stdlib.h>		/* exit */

void
msg_info (const char *msg)
{
  puts (msg);
}

/*
 * "msg_warning", "msg_notice" and "msg_error" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

void
msg_warning (const char *msg1, const char *msg2)
{
  if (msg2)
    fprintf (stderr, PRGNAME ": %s: %s\n", msg1, msg2);
  else
    fprintf (stderr, PRGNAME ": %s\n", msg1);
}

void
msg_notice (const char *msg1, const char *msg2)
{
  msg_warning (msg1, msg2);
}

void
msg_error (const char *msg1, const char *msg2)
{
  msg_warning (msg1, msg2);
  exit (EXIT_FAILURE);
}
