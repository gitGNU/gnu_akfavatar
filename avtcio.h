/*
 * C-specific functions for AKFAvatar
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * the calling program must have used avt_initialize before calling 
 * any of these functions.
 * Some of these functions also require avt_mb_encoding to be used 
 * before calling them.
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

#ifndef _AVTCIO_H
#define _AVTCIO_H

#include "akfavatar.h"
#include <stdarg.h>
#include <wchar.h>

AVT_BEGIN_DECLS

int avt_vprintf (const char *format, va_list ap);
int avt_printf (const char *format, ...);
int avt_putchar (int c);
int avt_puts (const char *s);
int avt_vscanf (const char *format, va_list ap);
int avt_scanf (const char *format, ...);

int avt_vwprintf (const wchar_t * format, va_list ap);
int avt_wprintf (const wchar_t * format, ...);
wint_t avt_putwchar (wchar_t c);
int avt_putws (const wchar_t * s);
int avt_vwscanf (const wchar_t * format, va_list ap);
int avt_wscanf (const wchar_t * format, ...);

AVT_END_DECLS

#endif /* _AVTCIO_H */
