/*
 * avtmsg - message output for AKFAvatar
 * Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
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
#include <stdio.h>

extern void
avta_graphic_error (const char *msg1, const char *msg2)
{
  char msg[4096 + 1];

  if (msg2)
    snprintf (msg, sizeof (msg), "%s:\n%s", msg1, msg2);
  else
    snprintf (msg, sizeof (msg), "%s", msg1);

  avt_set_status (AVT_NORMAL);
  avt_change_avatar_image (NULL);
  avt_set_balloon_color (0xFF, 0xAA, 0xAA);
  avt_normal_text ();
  avt_set_auto_margin (AVT_TRUE);
  avt_set_scroll_mode (-1);
  avt_set_text_delay (0);
  avt_lock_updates (AVT_FALSE);
  avt_bell ();
  avt_tell_mb (msg);
  avt_wait_button ();
  avt_quit ();
}
