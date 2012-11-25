/*
 * example to program AKFAvatar in C (can be used as a starting point)
 * Copyright (c) 2012 AKFoerster
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

/*
 * If you write your own program based on this,
 * you are allowed to remove my name from the
 * Copyright notice.
 * AKFoerster
 */

/* SDL redefines main on some systems */
#if defined(_WIN32) || defined(__APPLE__) || defined(macintosh)
#  include "SDL.h"
#endif

// include the akfavatar library functions
#include "akfavatar.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// XPM files are valid C-Code!
#include "data/female_user.xpm"


#define PRGNAME  "AKFAvatar example program"
#define PRGSHORTNAME  "AKFAvatar"

// Prototypes
static void say (char *msg);
static void plot (void);


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
  // make the compiler not complain, that they are not used
  (void) argc;
  (void) argv;

  avt_mb_encoding ("UTF-8");

  // initialize it
  if (avt_start (PRGNAME, PRGSHORTNAME, AVT_AUTOMODE))
    {
      fprintf (stderr, "cannot initialize graphics: %s\n", avt_get_error ());
      exit (EXIT_FAILURE);
    }

  // clean up when the program exits
  atexit (avt_quit);

  plot ();

  return EXIT_SUCCESS;
}


static void
plot (void)
{
  char name[AVT_LINELENGTH + 1];
  // AVT_LINELENGTH is the maximum length of one line in a balloon

  // set the avatar
  avt_avatar_image_xpm (female_user_xpm);

  // for static linking it is beneficial to avoid avt_colorname
  avt_set_background_color (0xFFC0CB);

  if (avt_move_in ())
    exit (EXIT_SUCCESS);

  // do slow printing
  avt_set_text_delay (AVT_DEFAULT_TEXT_DELAY);

  // set the balloon size: height, width (use 0 for maximum)
  avt_set_balloon_size (1, 50);

  // ask for a name
  say ("What's your name? ");
  if (avt_ask_mb (name, sizeof (name)))
    exit (EXIT_SUCCESS);

  // if no name was given, call him "stranger" ;-)
  if (!name[0])
    strcpy (name, "stranger");

  avt_set_balloon_size (6, 50);
  // clear the balloon
  avt_clear ();
  say ("Hello ");
  say (name);
  say (",\n\n");
  say ("I am the avatar (the incarnation) of your program.\n"
       "It is sooo easy to program me...\n\n"
       "I am longing for being programmed by you!");

  // wait for a key, move out and wait some time
  avt_wait_button ();
  avt_move_out ();
  avt_wait (AVT_SECONDS (0.75));
}


static void
say (char *msg)
{
  /*
   * tell the message
   * and if there is quit-request or a fatal error, stop the program
   * (note: fatal errors are unlikely after initialization)
   */
  if (avt_say_mb (msg))
    exit (EXIT_SUCCESS);
}
