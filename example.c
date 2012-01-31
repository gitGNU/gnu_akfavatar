/*
 * example to program AKFAvatar in C (can be used as a starting point)
 * Copyright (c) 2012 ... (enter your name)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* include the akfavatar library functions */
#include "akfavatar.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


#define PRGNAME  "AKFAvatar example program"
#define PRGSHORTNAME  "AKFAvatar"


void
say (wchar_t * msg)
{
  /*
   * tell the message
   * and if there is quit-request or a fatal error, stop the program
   * (note: fatal errors are unlikely after initialization)
   */
  if (avt_say (msg))
    exit (EXIT_SUCCESS);
}


void
run_plot (void)
{
  wchar_t name[AVT_LINELENGTH + 1];
  /* AVT_LINELENGTH is the maximum length of one line in a balloon */

  /* do slow printing */
  avt_set_text_delay (AVT_DEFAULT_TEXT_DELAY);

  /* set the balloon size: height, width (use 0 for maximum) */
  avt_set_balloon_size (6, 50);

  if (avt_move_in ())
    exit (EXIT_SUCCESS);

  /* ask for a name */
  say (L"What's your name? ");
  if (avt_ask (name, sizeof (name)))
    exit (EXIT_SUCCESS);

  /* if no name was given, call him "Mister unknown" ;-) */
  if (!name[0])
    wcscpy (name, L"Mister unknown");

  /* clear the balloon */
  avt_clear ();
  say (L"Hello ");
  say (name);
  say (L",\n\n");
  say (L"I am the avatar (the incarnation) of your program.\n"
       L"It is sooo easy to program me...\n\n"
       L"I am longing for being programmed by you!");

  /* wait for a key, move out and wait some time */
  avt_wait_button ();
  avt_move_out ();
  avt_wait (AVT_SECONDS (1));
}


/*
 * For the SDL on the windows platform the main function must have
 * exactly this form!  It will be replaced with a macro.
 *
 * (Windows normally uses a non-standard entry function for graphical
 * programs, which is not portable at all. This macro makes it portable)
 */
int
main (int argc, char *argv[])
{
  /* initialize it */
  if (avt_initialize (PRGNAME, PRGSHORTNAME, avt_default (), AVT_AUTOMODE))
    {
      fprintf (stderr, "cannot initialize graphics: %s\n", avt_get_error ());
      exit (EXIT_FAILURE);
    }

  /* clean up when the program exits */
  atexit (avt_quit);

  run_plot ();

  return EXIT_SUCCESS;
}
