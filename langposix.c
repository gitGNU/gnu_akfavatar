/*
 * get language code for POSIX compatible systems
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
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

#include <stdlib.h>
#include <locale.h>

// the macros assume that latin letters are in a continuous ordered block
// like in ASCII based charsets, but not in EBCDIC

#define checkalpha(c) \
  (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))

#define lowercase(c) \
  (((c) >= 'a' && (c) <= 'z') ? (c) : (c) + ('a' - 'A'))

// returns 2-letter language code according ISO 639-1
// or NULL if unknown
extern char *
avta_get_language (void)
{
  static char language[3];
  char *l;

  // don't depend on setlocale being used (only as last resort)
  if (!(l = getenv ("LC_ALL"))
      || !(l = getenv ("LC_MESSAGES")) || !(l = getenv ("LANG")))
    l = setlocale (LC_MESSAGES, NULL);

  // check if it starts with two letters, followed by a non-letter
  if (!l || !checkalpha (l[0]) || !checkalpha (l[1]) || checkalpha (l[2]))
    return NULL;

  language[0] = lowercase (l[0]);
  language[1] = lowercase (l[1]);
  language[2] = '\0';

  return language;
}