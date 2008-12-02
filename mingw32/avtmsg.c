/* 
 * avtmsg - message output for avatarsay for windows
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
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

/* $Id: avtmsg.c,v 2.2 2008-12-02 11:31:29 akf Exp $ */

#include "avtmsg.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>

void
info_msg (const char *msg)
{
  MessageBox (GetActiveWindow (), msg, PRGNAME,
	      MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
}

void
warning_msg (const char *msg1, const char *msg2)
{
  char msg[1024];

  strcpy (msg, msg1);

  if (msg2)
    {
      strcat (msg, ": ");
      strcat (msg, msg2);
    }

  MessageBox (GetActiveWindow (), msg, PRGNAME,
	      MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
}

/* ignore unimportant notices on Windows */
void
notice_msg (const char *msg1, const char *msg2)
{
}

void
error_msg (const char *msg1, const char *msg2)
{
  char msg[1024];

  strcpy (msg, msg1);

  if (msg2)
    {
      strcat (msg, ": ");
      strcat (msg, msg2);
    }

  MessageBox (GetActiveWindow (), msg, PRGNAME,
	      MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
  exit (EXIT_FAILURE);
}
