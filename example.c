/*
 * example how to program AKFAvatar in C
 * this file is in the public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* the avatar library: */
#include "avatar.h"


void
say (wchar_t *msg)
{
  if (avt_say (msg))
    exit (0);
}

void
init (int argc, char *argv[])
{
  int mode = WINDOW;
  int i;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "--fullscreen") || !strcmp (argv[i], "-f"))
	mode = FULLSCREEN;
      if (!strcmp (argv[i], "--window") || !strcmp (argv[i], "-w"))
	mode = WINDOW;
    }

  if (avt_initialize ("Avatar", "Avatar", avt_default (), mode))
    {
      fprintf (stderr, "cannot initialize graphics: %s\n",
	       avt_get_error ());
      exit (1);
    }

  atexit (avt_quit);
}

/* 
 * for the SDL on the windows platform the main function head 
 * must have exactly this form, see SDL FAQ for windows 
 */

int
main (int argc, char *argv[])
{
  wchar_t s[255];

  init (argc, argv);

  if (avt_move_in ())
    exit (0);

  say (L"What's your name? ");
  if (avt_ask (s, sizeof (s)))
    exit (0);

  if (s[0] == L'\0')
    wcscpy (s, L"Mister unknown");

  avt_clear ();
  say (L"Hello ");
  say (s);
  say (L",\n\n");
  say (L"I am the avatar (the incarnation) of your program.\n"
       L"It is sooo easy to program me...\n\n" 
       L"I am longing for being programmed by you!");

  if (avt_wait_key (L"press any key..."))
    exit (0);

  if (!avt_move_out ())
    avt_wait (seconds (1));

  return 0;
}
