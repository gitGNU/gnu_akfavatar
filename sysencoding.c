/*
 * systems encoding (locale LC_CTYPE) support for AKFAvatar
 * Copyright (c) 2013 Andreas K. Foerster <info@akfoerster.de>
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
#include "avtinternals.h"
#include <wchar.h>
#include <stdlib.h>
#include <limits.h>


static size_t
system_to_unicode (const struct avt_charenc *self, avt_char * dest,
		   const char *src)
{
  (void) self;

  wchar_t ch;
  size_t r = mbrtowc (&ch, src, MB_LEN_MAX, NULL);

  if (r == 0)
    {
      ch = L'\0';
      r = 1;
    }
  else if (r == (size_t) (-1) or r == (size_t) (-2))
    {
      ch = AVT_INVALID_WCHAR;
      r = 1;
    }

  *dest = (avt_char) ch;

  return (size_t) r;
}


static size_t
system_from_unicode (const struct avt_charenc *self, char *dest, size_t size,
		     avt_char src)
{
  (void) self;

  // enough space?
  // note MB_CUR_MAX may be a function
  if (size < MB_LEN_MAX and size < MB_CUR_MAX)
    return 0;

  // wchar_t is too small? - limit to BMP
  if (sizeof (wchar_t) <= 2 and src > 0xFFFF)
    src = AVT_INVALID_WCHAR;

  size_t r = wcrtomb (dest, (wchar_t) src, NULL);

  if (r == (size_t) (-1))
    {
      r = wcrtomb (dest, AVT_INVALID_WCHAR, NULL);

      if (r == (size_t) (-1))
	r = wcrtomb (dest, L'\x1A', NULL);

      if (r == (size_t) (-1))
	r = 0;
    }

  return (size_t) r;
}


static const struct avt_charenc converter = {
  .data = NULL,
  .decode = system_to_unicode,
  .encode = system_from_unicode
};


extern const struct avt_charenc *
avt_systemencoding (void)
{
  return &converter;
}
