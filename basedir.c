/*
 * get base directory of the executable, if possible
 * written 2012 by Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is dedicated to the Public Domain (CC0)
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L

#include "avtaddons.h"
#include <string.h>
#include <iso646.h>

#if defined(__linux__)		// Linux 2.2 or later

#include <unistd.h>

extern int
avta_base_directory (char *name, size_t size)
{
  char *p;

  // readlink conforms to POSIX.1-2001
  // "/proc/self/exe" is linux specific
  if (readlink ("/proc/self/exe", name, size) == -1
      or not (p = strrchr (name, '/')))
    {
      name[0] = '\0';
      return -1;
    }

  // cut filename off
  *p = '\0';

  // eventually also strip subdirectory /bin
  if (memcmp ("/bin", p - 4, 5) == 0)
    *(p - 4) = '\0';

  return 0;
}

#elif defined(_WIN32)

#include <windows.h>

extern int
avta_base_directory (char *name, size_t size)
{
  char *p;
  DWORD len;

  len = GetModuleFileNameA (NULL, name, size);

  if (not len or len == size or not (p = strrchr (name, '\\')))
    {
      name[0] = '\0';
      return -1;
    }
  else
    *p = '\0';			// cut filename off

  return 0;
}

#else // not __linux__ and not _WIN32

extern int
avta_base_directory (char *name, size_t size)
{
  // no general known way to find a base directory
  // well, I could search the PATH for argv[0], but...
  if (size > 0)
    name[0] = '\0';

  return -1;
}

#endif // not __linux__ and not _WIN32
