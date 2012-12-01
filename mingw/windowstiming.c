/*
 * Windows timing functions for AKFAvatar
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
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
#include <windows.h>
#include <iso646.h>

static DWORD start_ticks;

extern size_t
avt_ticks (void)
{
  DWORD now;

  now = GetTickCount ();

  if (not start_ticks)
    start_ticks = now;

  if (now >= start_ticks)
    return (now - start_ticks);
  else				// overrun
    return (now + (0xFFFFFFFFu - start_ticks));
}

extern void
avt_delay (int milliseconds)
{
  Sleep (milliseconds);
}
