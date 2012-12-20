/*
 * get base directory of the executable, if possible
 * written 2012 by Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is dedicated to the Public Domain (CC0)
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE		// for memrchr

#include "avtaddons.h"
#include <string.h>
#include <iso646.h>

#if defined(__linux__)		// Linux 2.2 or later

#include <unistd.h>

extern int
avta_base_directory (char *name, size_t size)
{
  ssize_t nchars;

  // readlink conforms to POSIX.1-2001
  // "/proc/self/exe" is Linux specific
  // note: name does not get terminated here
  nchars = readlink ("/proc/self/exe", name, size);

  // find last '/'
  while (nchars > 0 and name[nchars] != '/')
    --nchars;

  if (nchars <= 0)
    {
      name[0] = '\0';
      return -1;
    }

  // cut off the file name - finally terminating the string
  name[nchars] = '\0';

  // eventually also strip last subdirectory "/bin"
  if (memcmp ("/bin", &name[nchars - 4], 5) == 0)
    name[nchars - 4] = '\0';

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

  *p = '\0';			// cut filename off

  return 0;
}

#else // no way to find the base directory

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
