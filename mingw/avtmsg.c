/* 
 * avtmsg - message output for avatarsay for windows
 * Copyright (c) 2007, 2008, 2009, 2010 Andreas K. Foerster <info@akfoerster.de>
 *
 * stdout/stderr are broken in Windows GUI programs,
 * that is the only reason why I fall back to the Windows API
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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static const char *prgname = "AKFAvatar";

extern void
avta_prgname (const char *name)
{
  prgname = name;
}

static void
avta_message_box (const char *msg1, const char *msg2, unsigned int MB)
{
  char msg[4096 + 1];

  if (msg2)
    snprintf (msg, sizeof (msg), "%s:\n%s", msg1, msg2);
  else
    snprintf (msg, sizeof (msg), "%s", msg1);

  MessageBox (GetActiveWindow (), msg, prgname,
	      MB | MB_OK | MB_SETFOREGROUND);
}

extern void
avta_info (const char *msg)
{
  MessageBox (GetActiveWindow (), msg, prgname,
	      MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
}

extern void
avta_warning (const char *msg1, const char *msg2)
{
  avta_message_box (msg1, msg2, MB_ICONWARNING);
}

/* ignore unimportant notices on Windows */
extern void
avta_notice (const char *msg1 AVT_UNUSED, const char *msg2 AVT_UNUSED)
{
}

extern void
avta_error (const char *msg1, const char *msg2)
{
  extern void avta_graphic_error (const char *msg1, const char *msg2);

  if (avt_initialized ())
    avta_graphic_error (msg1, msg2);
  else
    avta_message_box (msg1, msg2, MB_ICONERROR);

  exit (EXIT_FAILURE);
}
