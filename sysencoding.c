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

/*
 * Latin-1 is just a small subset of the supported characters,
 * but it's still widely used and easy to implement
 */

#include "akfavatar.h"
#include "avtinternals.h"
#include <stdlib.h>
#include <limits.h>

static size_t
system_to_unicode (avt_char * dest, const char *src)
{
  int r;
  wchar_t ch;

  r = mbtowc (&ch, src, MB_LEN_MAX);

  if (r == 0)
    {
      ch = L'\0';
      r = 1;
    }
  else if (r < 0)
    {
      ch = BROKEN_WCHAR;
      r = 1;
    }

  *dest = ch;

  return (size_t) r;
}


static size_t
system_from_unicode (char *dest, size_t size, avt_char src)
{
  int r;

  // enough space?
  if (size < MB_CUR_MAX)
    return 0;

  // wchar_t is too small?
  if (sizeof (wchar_t) <= 2 and src > 0xFFFF)
    src = BROKEN_WCHAR;

  r = wctomb (dest, (wchar_t) src);

  if (r <= 0)
    {
      r = wctomb (dest, BROKEN_WCHAR);

      if (r <= 0)
	r = wctomb (dest, L'\x1A');

      if (r < 0)
	r = 0;
    }

  return (size_t) r;
}


extern struct avt_charenc *
avt_systemencoding (void)
{
  static struct avt_charenc converter;

  converter.data = NULL;
  converter.to_unicode = system_to_unicode;
  converter.from_unicode = system_from_unicode;

  return &converter;
}
