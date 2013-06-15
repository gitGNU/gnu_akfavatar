/*
 * avttermstub - dummy stub replacing avtterm
 * Copyright (c) 2009, 2010, 2013 Andreas K. Foerster <info@akfoerster.de>
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

extern int
avt_term_start (const char *working_dir, char *prg_argv[])
{
  (void) working_dir;
  (void) prg_argv;

  return -1;
}

extern void
avt_term_run (int fd)
{
  (void) fd;
}

extern void
avt_term_register_apc (avt_term_apc_cmd command)
{
  (void) command;
}

extern void
avt_term_nocolor (bool nocolor)
{
  (void) nocolor;
}

extern void
avt_term_slowprint (bool on)
{
  (void) on;
}

extern void
avt_term_update_size (void)
{
}

extern void
avt_term_send (const char *buf, size_t count)
{
  (void) buf;
  (void) count;
}
