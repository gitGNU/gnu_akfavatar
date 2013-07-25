/*
 * example to program AKFAvatar in C
 * This can be used as a starting point for your own programs.
 *
 * This example is dedicated to the public domain (CC0)
 * http://creativecommons.org/publicdomain/zero/1.0/
 *
 * You may relisense it to GPLv3 or a compatible license
 * under your own copyright
 */

/* SDL redefines main on some systems */
#if defined(_WIN32) || defined(__APPLE__) || defined(macintosh)
#  include "SDL.h"
#endif

// include the akfavatar library functions
#include "akfavatar.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// XPM files are valid C-Code!
#include "data/akfoerster.xpm"


#define PRGNAME  "AKFAvatar example program"
#define PRGSHORTNAME  "AKFAvatar"

// Prototypes
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
  // make the compiler not complain about unused parameters
  (void) argc;
  (void) argv;

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
  // set the avatar
  avt_avatar_image_xpm (akfoerster_xpm);
  avt_set_avatar_name (L"Andreas K. F\u00F6rster");

  // for static linking it is beneficial to avoid avt_colorname
  avt_set_background_color (0xBEBEBE);

  if (avt_move_in ())
    exit (EXIT_SUCCESS);

  // activate slow printing - set to 0 to deactivate
  avt_set_text_delay (AVT_DEFAULT_TEXT_DELAY);

  // set the balloon size: height, width (use 0 for maximum)
  avt_set_balloon_size (5, 50);

  avt_say (L"Hello,\n"
	   L"My name is Andreas K. F\u00F6rster.\n"
	   L"I am the author of AKFAvatar - this userinterface.\n\n");

  // ask for a name
  wchar_t name[AVT_LINELENGTH + 1];
  avt_say (L"What's your name? ");
  if (avt_ask (name, sizeof (name)))
    exit (EXIT_SUCCESS);

  // if no name was given, call him "stranger" ;-)
  if (!name[0])
    wcscpy (name, L"stranger");

  avt_set_balloon_size (2, 50);

  // clear the balloon
  avt_clear ();
  avt_say (L"Hello ");
  avt_say (name);
  avt_say (L",\nnice to meet you!");

  // wait for a key, move out and wait some time
  // checking the return code here also catches earlier quit-requests
  if (avt_wait_button ())
    exit (EXIT_SUCCESS);

  avt_move_out ();
  avt_wait (AVT_SECONDS (0.75));
}
