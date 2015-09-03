/*
 * avttermsys - system specific functions for terminal emulation
 * Copyright (c) 2009,2010,2011,2012,2013,2014
 + Andreas K. Foerster <akf@akfoerster.de>
 *
 * These functions are only for internal use, they are not part of the API!
 *
 * This file is part of AKFAvatar
 * This file is not part of the official API
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

/*
 * execute a subprocess, visible in the balloon
 * if prg_argv is NULL, start a shell
 * sets input_fd to a file-descriptor for the input of the process
 * returns file-descriptor for the output of the process or -1 on error
 * both file-descriptors can be the same
 */
extern int avt_term_initialize (int *input_fd, int width, int height,
                                bool monochrome,
                                const char *working_dir,
                                char *prg_argv[]);

// tell the terminal the new size of the balloon
// (this may be a dummy function)
extern void avt_term_size (int fd, int height, int width);

// close terminal
extern void avt_closeterm (int fd);
