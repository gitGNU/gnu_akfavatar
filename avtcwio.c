/*
 * C-specific functions for AKFAvatar (wide characters)
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * the calling program must have used avt_initialize before calling 
 * any of these functions.
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

#ifndef _ISOC99_SOURCE
#  define _ISOC99_SOURCE
#endif

#include "avtcio.h"
#include <stdio.h>

#ifndef AVT_PRINTF_MAXLEN
#  define AVT_PRINTF_MAXLEN 1024
#endif

/* msvcrt has this function in a non-standard way */
#if defined(__MINGW32__) && !defined(vswprintf)
#  define vswprintf _vswprintf
#endif

extern int
avtcwio_vwprintf (const wchar_t * format, va_list ap)
{
  wchar_t str[AVT_PRINTF_MAXLEN];
  int n;

  /* this is conforming to the C99 standard */
  n = vswprintf (str, AVT_PRINTF_MAXLEN, format, ap);

  if (n > -1)
    avt_say_len (str, n);

  return n;
}

extern int
avtcwio_wprintf (const wchar_t * format, ...)
{
  va_list ap;
  int n;

  va_start (ap, format);
  n = avtcwio_vwprintf (format, ap);
  va_end (ap);

  return n;
}

extern wint_t
avtcwio_putwchar (wchar_t c)
{
  avt_put_character (c);
  return c;
}

extern int 
avtcwio_putws (const wchar_t *s)
{
  avt_say (s);
  avt_new_line ();

  /* return a non-negative number on success */
  return 1;
}

extern int
avtcwio_vwscanf (const wchar_t *format, va_list ap)
{
  wchar_t str[AVT_LINELENGTH];

  avt_ask (str, sizeof (str));
  return vswscanf (str, format, ap);
}

extern int 
avtcwio_wscanf (const wchar_t *format, ...)
{
  va_list ap;
  int n;

  va_start (ap, format);
  n = avtcwio_vwscanf (format, ap);
  va_end (ap);

  return n;
}
