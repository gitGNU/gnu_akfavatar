/*
 * avttermstub - dummy stub replacing avtterm
 * Copyright (c) 2009 Andreas K. Foerster <info@akfoerster.de>
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

extern void
avtterm_register_APC (avtterm_APC_command command AVT_UNUSED)
{
}

extern void
avtterm_nocolor (avt_bool_t nocolor AVT_UNUSED)
{
}

extern void
avtterm_slowprint (avt_bool_t on AVT_UNUSED)
{
}

extern void
avtterm_update_size (void)
{
}

extern int
avtterm_start (const char *system_encoding AVT_UNUSED,
	       const char *working_dir AVT_UNUSED,
	       char *const prg_argv[] AVT_UNUSED)
{
  return -1;
}

extern void
avtterm_run (int fd AVT_UNUSED)
{
}
