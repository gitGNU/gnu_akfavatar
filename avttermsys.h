/*
 * avttermsys - system specific functions for terminal emulation
 * Copyright (c) 2007, 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
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


#include "akfavatar.h"		/* for avt_bool_t */

/* execute a subprocess, visible in the balloon */
/* if fname == NULL, start a shell */
/* sets input_fd to a file-descriptor for the input of the process */
/* returns file-descriptor for the output of the process or -1 on error */
extern int avta_term_initialize (int *input_fd, int width, int height,
				 avt_bool_t monochrome,
				 const char *working_dir,
				 char *const prg_argv[]);

/* may be defined externally when EXT_AVTTERM_SIZE is defined */
extern void avta_term_size (int fd, int height, int width);
