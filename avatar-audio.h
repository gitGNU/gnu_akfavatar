/*
 * AKFAvatar - library for showing an avatar who says things in a balloon
 * this part is for the audio-output
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * needed: 
 *  SDL1.2
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

/* $Id: avatar-audio.h,v 2.1 2007-08-20 17:55:15 akf Exp $ */

#ifndef _avatar_audio_h
#define _avatar_audio_h

#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C"
{
 /* *INDENT-ON* */
#endif /* __cplusplus */


/* must be called AFTER avt_initialize! */
extern int avt_initialize_audio (void);

/* should be called BEFORE avt_quit */
extern void avt_quit_audio (void);

/* 
 * loads a wave file 
 * supported: PCM, MS-ADPCM, IMA-ADPCM
 */
extern int avt_load_wave_file (const char *file);
extern int avt_load_wave_data (void *data, int datasize);

/* stops audio and frees the audio memory */
extern void avt_free_wave (void);

extern int avt_play_audio (void);

extern int avt_wait_audio_end (void);

/* stops audio, but leaves the file loaded */
/* the next call to avt_play_audio will start from the beginning */
extern void avt_stop_audio (void);

#ifdef __cplusplus
/* *INDENT-OFF* */
}
#endif /* __cplusplus */
#endif /* _avatar_audio_h */
