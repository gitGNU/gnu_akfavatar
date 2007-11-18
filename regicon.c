/* This file is in the Public Domain - AKFoerster */

static void
avt_register_icon (void)
{
  SDL_Surface *icon;
  SDL_Color color;

  static char icondata[] = {
    "                                "
    "                                "
    "                                "
    "     ......................     "
    "    ........................    "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "   ..++++++++++++++++++++++..   "
    "    .............+++++++....    "
    "     .............+++++....     "
    "                ..++++..        "
    "               ..++++..         "
    "              ..++++..          "
    "             ..++++..           "
    "            ..++++..            "
    "           ..++++..             "
    "          ..++++..              "
    "         ..+++...               "
    "        ..+....                 "
    "       .....                    "
    "      ...                       "
    "                                "
    "                                "
  };

  icon = SDL_CreateRGBSurfaceFrom (&icondata, 32, 32, 8, 32, 0, 0, 0, 0);

  /* silently fail on error */
  if (!icon)
    return;

  /* dark gray */
  color.r = 0x55;
  color.g = 0x55;
  color.b = 0x55;
  SDL_SetColors (icon, &color, '.', 1);

  /* white */
  color.r = 0xFF;
  color.g = 0xFF;
  color.b = 0xFF;
  SDL_SetColors (icon, &color, '+', 1);

  /* set space-char as being transparent */
  SDL_SetColorKey (icon, SDL_SRCCOLORKEY, 32);

  SDL_WM_SetIcon (icon, NULL);
  SDL_FreeSurface (icon);
}
