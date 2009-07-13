/*
 * different addons for AKFAvatar
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

#ifndef _AVTADDONS_H
#define _AVTADDONS_H

#include "akfavatar.h"
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

AVT_BEGIN_DECLS

/**********************************************************************
 * Section: avtccio
 * C-specific functions for input/output
 *
 * the calling program must have used avt_initialize before calling 
 * any of these functions.
 * Some of these functions also require avt_mb_encoding to be used 
 * before calling them.
 **********************************************************************/

int avt_vprintf (const char *format, va_list ap);
int avt_printf (const char *format, ...);
int avt_putchar (int c);
int avt_puts (const char *s);
int avt_vscanf (const char *format, va_list ap);
int avt_scanf (const char *format, ...);

/**********************************************************************
 * Section: avtcwio
 * C-specific functions for wide characters input/output
 *
 * the calling program must have used avt_initialize before calling 
 * any of these functions.
 * Some of these functions also require avt_mb_encoding to be used 
 * before calling them.
 **********************************************************************/

int avt_vwprintf (const wchar_t * format, va_list ap);
int avt_wprintf (const wchar_t * format, ...);
wint_t avt_putwchar (wchar_t c);
int avt_putws (const wchar_t * s);
int avt_vwscanf (const wchar_t * format, va_list ap);
int avt_wscanf (const wchar_t * format, ...);


/**********************************************************************
 * Section: avtmsg
 * message output for avatarsay
 **********************************************************************/

#define PRGNAME "avatarsay"

/*
 * "warning_msg", "notice_msg" and "error_msg" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

void info_msg(const char *msg);
void warning_msg (const char *msg1, const char *msg2);
void notice_msg (const char *msg1, const char *msg2);
void error_msg (const char *msg1, const char *msg2);

/**********************************************************************
 * Section: avtfilechooser
 * file-chooser for AKFAvatar
 **********************************************************************/

/* starts in working directory; return -1 on error or 0 on success */
int get_file (char *filename);

/**********************************************************************
 * Section: arch
 * functions for handling ar archives
 **********************************************************************/

/*
 * return file descriptor, if it's an archive
 * or -1 on error
 */
int arch_open (const char *archive);

/* 
 * finds a member in the archive 
 * and leaves the fileposition at its start
 * the member name may not be longer than 15 characters 
 * returns size of the member, or 0 if not found 
 */
size_t arch_find_member (int fd, const char *member);

/* 
 * finds first archive member
 * and leaves the fileposition at its start
 * if member is not NULL it will get the name of the member
 * member must have at least 16 bytes
 * returns size of first member
 */
size_t arch_first_member (int fd, char *member);

/* 
 * read in whole member of a named archive
 * the member name may not be longer than 15 characters 
 * the buffer is allocated with malloc and must be freed by the caller
 * the buffer gets some binary zeros added, so it can be used as string
 * returns size or 0 on error 
 */
size_t arch_get_data (const char *archive, const char *member,
		      void **buf, size_t * size);


/**********************************************************************
 * Section: avtterm
 * terminal emulator for AKFAvatar
 **********************************************************************/

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

#endif /* AVTADDONS_H */
