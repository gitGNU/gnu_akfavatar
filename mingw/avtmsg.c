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

#include "akfavatar.h"
#include "avtaddons.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>

void
msg_info (const char *msg)
{
  MessageBox (GetActiveWindow (), msg, PRGNAME,
	      MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
}

void
msg_warning (const char *msg1, const char *msg2)
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
msg_notice (const char *msg1 AVT_UNUSED, const char *msg2 AVT_UNUSED)
{
}

void
msg_error (const char *msg1, const char *msg2)
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
