/*
 * avtmsg - message output for AKFAvatar
 * Copyright (c) 2007, 2008, 2009, 2010 Andreas K. Foerster <info@akfoerster.de>
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

#include "akfavatar.h"
#include "avtaddons.h"
#include "avtinternals.h"
#include <stdio.h>
#include <stdlib.h>		/* exit */

static const char *prgname = "AKFAvatar";

extern void
avta_prgname (const char *name)
{
  prgname = name;
}

extern void
avta_info (const char *msg)
{
  puts (msg);
}

/*
 * "msg_warning", "msg_notice" and "msg_error" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

extern void
avta_warning (const char *msg1, const char *msg2)
{
  if (msg2)
    fprintf (stderr, "%s: %s: %s\n", prgname, msg1, msg2);
  else
    fprintf (stderr, "%s: %s\n", prgname, msg1);
}

extern void
avta_notice (const char *msg1, const char *msg2)
{
  avta_warning (msg1, msg2);
}

extern void
avta_error (const char *msg1, const char *msg2)
{
  if (avt_initialized ())
    avta_graphic_error (msg1, msg2);
  else
    avta_warning (msg1, msg2);

  exit (EXIT_FAILURE);
}
