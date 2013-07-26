/*
 * timing functions for AKFAvatar
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

#define _ISOC99_SOURCE
#define _XOPEN_SOURCE 600

#include "akfavatar.h"
#include <iso646.h>
#include <time.h>
#include <sys/time.h>


static struct timeval start_ticks;

extern size_t
avt_ticks (void)
{
  struct timeval now;

  // conforming to POSIX.1-2001
  gettimeofday (&now, NULL);

  if (not start_ticks.tv_sec)
    start_ticks = now;

  return ((now.tv_sec - start_ticks.tv_sec) * 1000)
    + ((now.tv_usec - start_ticks.tv_usec) / 1000);
}


// conforming to POSIX.1-2001
extern void
avt_delay (int milliseconds)
{
  struct timespec time, remaining;
  int r;

  if (milliseconds <= 0)
    return;

  remaining.tv_sec = milliseconds / 1000;
  remaining.tv_nsec = (milliseconds % 1000) * 1000000;

  do
    {
      time = remaining;
      r = nanosleep (&time, &remaining);
    }
  while (r == -1);
}
