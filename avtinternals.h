/*
 * internal prototypes for AKFAvatar
 * Copyright (c) 2010 Andreas K. Foerster <info@akfoerster.de>
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

#ifndef _AVTINTERNALS_H
#define _AVTINTERNALS_H

#include "SDL.h"
#include <stdio.h>		/* FILE */

/* avatar.c */
extern int _avt_STATUS;
extern void avt_analyze_event (SDL_Event * event);
extern int avt_checkevent (void);
extern void (*avt_alert_func) (void);
extern void (*avt_quit_audio_func) (void);

/* avtgrmsg.c */
extern void avta_graphic_error (const char *msg1, const char *msg2);

/* avtposix.c / avtwindows.c */
extern void get_user_home (char *home_dir, size_t size);
extern void edit_file (const char *name, const char *encoding);
extern FILE *open_config_file (const char *name, avt_bool_t writing);

/* mingw/askdrive.c */
extern int avta_ask_drive (int max_idx);

/* font.c */
extern const unsigned char *get_font_char (int ch);
extern const unsigned short *get_font_char2 (int ch);

#endif /* AVTINTERNALS_H */
