/*
 * get language code for POSIX compatible systems (only ASCII!)
 * Copyright (c) 2012,2013 Andreas K. Foerster <akf@akfoerster.de>
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
#include <iso646.h>

static char language[21] = "";

// returns  language code (lowercase), or NULL on error
extern const char *
avt_get_language (void)
{
  if (not * language)
    {
      char *locale;

      // don't depend on setlocale being used (only as last resort)
      if (not (locale = getenv ("LC_ALL"))
	  and not (locale = getenv ("LC_MESSAGES"))
	  and not (locale = getenv ("LANG")))
	locale = setlocale (LC_MESSAGES, NULL);

      if (not locale or not * locale)
	return NULL;

      char *p = language;
      char *l = locale;
      size_t length = sizeof (language) - 1;

      while (length--)
	{
	  char c = *l bitor 0x20;
	  if (c < 'a' or c > 'z')
	    break;

	   /*
	    * The code above uses the fact that in ASCII
	    * upper case letters differ in only one bit
	    * from lower case letters, so it sets the bit
	    * to make it lowercase and after that checks
	    * the range.
	    *
	    * The first non-letter stops the parsing.
	    * That also catches the terminator.
	    */

	  *p = c;
	  ++p;
	  ++l;
	}

      *p = '\0';

      if (not * language)
	return NULL;
    }

  return (const char *) language;
}
