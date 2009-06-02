/* 
 * avtterm - terminal emulation for AKFAAvatar
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
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

#ifndef AVTTERM_H
#define AVTTERM_H 1

#include "akfavatar.h"

/*
 * execute a subprocess, visible in the balloon
 * if prg_argv == NULL, start a shell
 * returns file-descriptor for output of the process
 * or -1 on error 
 */
int avtterm_start (const char *system_encoding, const char *working_dir,
		   char *const prg_argv[]);

void avtterm_run (int fd);
void avtterm_nocolor (avt_bool_t nocolor);

/* update size of textarea */
void avtterm_update_size (void);

#endif /* AVTTERM_H */
