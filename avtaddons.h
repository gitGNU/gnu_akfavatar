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

/*
 * This library is not so strictly handled as the main library.
 * This library uses the libc directly and can use system-specific
 * functions.
 * Not all parts of this library are available on all platforms!
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
 * the calling program must have used avt_initialize and
 * avt_mb_encoding  before calling any of these functions.
 **********************************************************************/

int avtccio_vprintf (const char *format, va_list ap);
int avtccio_printf (const char *format, ...);
int avtccio_putchar (int c);
int avtccio_puts (const char *s);
int avtccio_vscanf (const char *format, va_list ap);
int avtccio_scanf (const char *format, ...);

/**********************************************************************
 * Section: avtcwio
 * C-specific functions for wide characters input/output
 *
 * the calling program must have used avt_initialize before calling 
 * any of these functions.
 **********************************************************************/

int avtcwio_vwprintf (const wchar_t * format, va_list ap);
int avtcwio_wprintf (const wchar_t * format, ...);
wint_t avtcwio_putwchar (wchar_t c);
int avtcwio_putws (const wchar_t * s);
int avtcwio_vwscanf (const wchar_t * format, va_list ap);
int avtcwio_wscanf (const wchar_t * format, ...);


/**********************************************************************
 * Section: avtmsg
 * message output for avatarsay
 **********************************************************************/

/*
 * the output takes place on stderr when this exists
 * or in message boxes on some other systems
 */

/*
 * these functions are also used by other parts of avtaddons,
 * so they are most likely included
 */

/*
 * "warning_msg", "notice_msg" and "error_msg" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

/* set the name of the programm, default: "AKFAvatar" */
void msg_prgname (const char *name);

/* info on stdout */
void msg_info(const char *msg);

/* warning on stderr */
void msg_warning (const char *msg1, const char *msg2);

/* unimportant notice (might be ignored) */
void msg_notice (const char *msg1, const char *msg2);

/* error that quits the program */
void msg_error (const char *msg1, const char *msg2);

/**********************************************************************
 * Section: avtfilechooser
 * file-chooser for AKFAvatar
 **********************************************************************/

/* starts in working directory; returns 0 on success or -1 on error */
int avtfc_get_file (char *filename);

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
 * (not available for MinGW)
 **********************************************************************/

typedef void (*avtterm_APC_command) (wchar_t*);

/* register handler for APC commands (optional) */
void avtterm_register_APC (avtterm_APC_command command);

void avtterm_nocolor (avt_bool_t nocolor);

/*
 * execute a subprocess, visible in the balloon
 * if prg_argv == NULL, start a shell
 * returns file-descriptor for output of the process
 * or -1 on error
 */
int avtterm_start (const char *system_encoding, const char *working_dir,
		   char *const prg_argv[]);

void avtterm_run (int fd);

/*
 * update size of textarea
 * call this after you have changed the size of the balloon
 */
void avtterm_update_size (void);

#endif /* AVTADDONS_H */
