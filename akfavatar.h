/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * needed: 
 *  SDL1.2 (recommended: SDL1.2.11)
 * recommended: 
 *  SDL_image1.2
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

/* $Id: akfavatar.h,v 2.37 2008-01-15 08:35:06 akf Exp $ */

#ifndef _akfavatar_h
#define _akfavatar_h

/* SDL redefines main on some systems */
#if defined(__WIN32__) ||  defined(__MACOS__) || defined(__MACOSX__)
#  include "SDL.h"
#endif

/* to get the systems definition of wchar_t */
#include <stddef.h>

/* maximum linelength */
#define AVT_LINELENGTH 80

/* for avt_initialize */
#define AVT_AUTOMODE -1
#define AVT_WINDOW 0
#define AVT_FULLSCREENNOSWITCH 2

#ifdef AVT_NOSWITCH
#  define AVT_FULLSCREEN AVT_FULLSCREENNOSWITCH
#else
#  define AVT_FULLSCREEN 1
#endif

/* for _avt_STATUS */
#define AVT_NORMAL 0
#define AVT_QUIT 1
#define AVT_ERROR -1

/* for boolean expressions */
#define AVT_TRUE 1
#define AVT_FALSE 0

/* for avt_set_delays */
#define AVT_DEFAULT_TEXT_DELAY 75
#define AVT_DEFAULT_FLIP_PAGE_DELAY 2700

/* for avt_text_direction */
#define AVT_LEFT_TO_RIGHT 0
#define AVT_RIGHT_TO_LEFT 1

/* 
 * example: avt_wait(AVT_SECONDS(2.5)) waits 2.5 seconds 
 */
#define AVT_SECONDS(x) ((x)*1000)

#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif /* __cplusplus */

/*
 * boolean are integers for this library
 * to make language bindings more easy
 * (you can use stdbool.h in your program, it's compatible)
 */
typedef int avt_bool_t;

/* 
 * general type for avatar images
 * may change in the future!
 */
typedef void avt_image_t;

/* 
 * general type for audio data
 * may change in the future!
 */
typedef void avt_audio_t;

/*
 * type for keyhandler
 * see avt_register_keyhandler
 */
typedef void (*avt_keyhandler) (int sym, int mod, int unicode);

/* base fnctions */

/* 
 * initialize the avatar system
 * mode is either WINDOW or FULLSCREEN or FULLSCREENNOSWITCH
 * the original image is freed in this function!
 * the image may be NULL if no avatar should be shown
 */
int avt_initialize (const char *title,
       		    const char *icontitle,
		    avt_image_t * image, int mode);

/* 
 * quit the avatar system
 */
void avt_quit (void);

/* which version */
const char *avt_version (void);

/* copyright information */
const char *avt_copyright (void);

/* license information */
const char *avt_license (void);

/* is it initialized? */
avt_bool_t avt_initialized (void);

/* 0 = normal; 1 = quit-request; -1 = error */
int avt_get_status (void);

/* set status */
void avt_set_status (int status);

/* get error message */
char *avt_get_error (void);

/* 
 * set text direction
 * the cursor is moved to start of the line
 * in a text, you might want to call avt_newline after that
 */
void avt_text_direction (int direction);

/* stop, when Esc is pressed? (default: yes) */
void avt_stop_on_esc (avt_bool_t stop);

/*
 * get the default avatar image
 * use this with avt_initialize
 */
avt_image_t *avt_default (void);

/*
 * RGB gimp_image
 * for importing an avatar
 * also uses avt_make_transparent
 */
avt_image_t *avt_import_gimp_image (void *gimp_image);

/* 
 * import avatar from image data
 */
avt_image_t *avt_import_image_data (void *img, int imgsize);

/* 
 * import avatar from file
 */
avt_image_t *avt_import_image_file (const char *file);

/* 
 * free avt_image_t images
 * allocated with avt_import_gimp_image or avt_import_imagefile
 * (automatically called in avt_initialize)
 */
void avt_free_image (avt_image_t * image);

/*
 * make background transparent
 * pixel in the upper left corner is supposed to be the background color
 */
avt_image_t *avt_make_transparent (avt_image_t * image);

/*
 * define the backgroud color
 * values in the range 0x00 .. 0xFF
 * can and should be called before avt_initialize
 * if the balloon is visible, it is cleared
 */
void avt_set_background_color (int red, int green, int blue);

/*
 * change the text color
 * values in the range 0x00 .. 0xFF
 */
void avt_set_text_color (int red, int green, int blue);
void avt_set_text_background_color (int red, int green, int blue);

/*
 * delay time for text-writing
 * default: AVT_DEFAULT_TEXT_DELAY
 */
void avt_set_text_delay (int delay);

/*
 * delay time for page flipping
 * default: AVT_DEFAULT_FLIP_PAGE_DELAY
 */
void avt_set_flip_page_delay (int delay);

/* don't use this anymore, it is about to be removed */
void avt_set_delays (int text, int flip_page);

/* register an external keyhandler */
void avt_register_keyhandler (avt_keyhandler handler);

/*
 * switch to fullscreen or window mode 
 * (experimental!)
 */
void avt_switch_mode (int mode);

/* 
 * prints a L'\0' terminated string in the balloon
 * if there is no balloon, it is drawn
 * if there is no avatar, it is shown (not moved in)
 * interprets control chars
 */
int avt_say (const wchar_t * txt);

/* 
 * writes string with given length in the balloon
 * the string needn't be terminated and can contain binary zeros
 * if there is no balloon, it is drawn
 * if there is no avatar, it is shown (not moved in)
 * interprets control characters
 */
int avt_say_len (const wchar_t * txt, const int len);

/*
 * writes a single character in the balloon
 * if there is no balloon, it is drawn
 * if there is no avatar, it is shown (not moved in)
 * interprets control characters
 */ 
int avt_put_character (const wchar_t ch);

/* set encoding for mb functions */
int avt_mb_encoding (const char *encoding);

/* 
 * decode a string into wchar_t
 * size in bytes
 * returns number of characters in dest (without the termination zero)
 * dest must be freed by caller with avt_free
 */
int avt_mb_decode (wchar_t ** dest, const char *src, const int size);

/* 
 * encode a string from wchar_t
 * len is the length
 * returns number of characters in dest (without the termination zero)
 * dest must be freed by caller with avt_free
 * (the size of dest may be much more than needed)
 */
int avt_mb_encode (char ** dest, const wchar_t *src, const int len);

/* free memory allocated by this library */
void avt_free (void *ptr);

/*
 * like avt_say,
 * but converts from a given charset encoding
 * (see avt_mb_encoding)
 * the text is a 0 terminated C-String
 */
int avt_say_mb (const char *txt);

/*
 * the same with a given length
 * the string needn't be terminated then 
 * and can contain binary zeros
 */
int avt_say_mb_len (const char *txt, int len);

/*
 * get a character from the keyboard
 * only for printable characters, not for function keys
 * (ch is just one character, not a string)
 */
int avt_get_key (wchar_t * ch);

/*
 * get string (just one line)
 * the maximum length is LINELENGTH-1
 * size is the size of s in bytes (not the length)
 *
 * (I don't use size_t for better compatiblity with other languages)
 */
int avt_ask (wchar_t * s, const int size);

/*
 * like avt_ask,
 * but converts to a given encoding
 *
 * for UTF-8 encoding it should have a capacity of 
 * 4 * LINELENGTH Bytes
 */
int avt_ask_mb (char *s, const int size);

/*
 * new line
 * same as \n in avt_say
 */
int avt_new_line (void);

/*
 * wait a while and then clear the textfield
 * same as \f in avt_say
 */
int avt_flip_page (void);

/* update, ie handle events and give some time to other processes */
/* use this in a longer loop in your program */
int avt_update (void);

/* wait a while */
int avt_wait (int milliseconds);

/* wait for a keypress while displaying a button */
int avt_wait_button (void);

/* wait for a keypress (deprecated) */
int avt_wait_key (const wchar_t * message);

/*
 * like avt_waitkey,
 * but converts from a given charset encoding
 * (deprecated) 
 */
int avt_wait_key_mb (char *message);


/* functions for extended use */

/* 
 * set a viewport (sub-area of the textarea)
 * upper left corner is 1, 1
 */
void avt_viewport (int x, int y, int width, int height);

/* show an empty screen with the background color */
void avt_clear_screen (void);

/* show just the avatar without the balloon */
void avt_show_avatar (void);

/* 
 * load image and show it
 * if SDL_image isn't available then uncompressed BMP is still supported
 * if it succeeds call avt_wait or avt_waitkey 
 */
int avt_show_image_file (const char *file);

/*
 * show image from image data
 * if SDL_image isn't available then uncompressed BMP is still supported
 * after that call avt_wait or avt_waitkey
 */
int avt_show_image_data (void *img, int imgsize);

/*
 * show gimp image
 */
int avt_show_gimp_image (void *gimp_image);

/*
 * like avt_show_avatar, but the avatar is moved in
 */
int avt_move_in (void);

/*
 * move the avatar out => empty screen
 */
int avt_move_out (void);

/*
 * make a short sound, when audio is initialized
 * else it is the same as avt_flash
 * same as with \a in avt_say
 * the sound is actually not a bell ;-)
 */
void avt_bell (void);

/*
 * visual flash of the screen
 */
void avt_flash (void);

/* 
 * clears the viewport
 * if there is no balloon yet, it is drawn
 */
void avt_clear (void);

/* 
 * clear end of line
 * depending on text direction
 */
void avt_clear_eol (void);

/* 
 * clear beginning of line
 * depending on text direction
 */
void avt_clear_bol (void);

/* clear line */
void avt_clear_line (void);

/* forward one character position
 * ie. print a space
 */
int avt_forward (void);

/*
 * delete last caracter
 */
void avt_backspace (void);

/*
 * set scroll mode
 * 0 = off (page-flipping), 1 = normal
 * (further modes are planned, it is not meant as a boolean)
 */
void avt_set_scroll_mode (int mode);
int avt_get_scroll_mode (void);

/* 
 * handle coordinates
 *
 * the coordinates start with 1, 1 
 * in the upper left corner
 * and are independent from the text direction
 */

/*
 * get position in the viewport
 */
int avt_where_x (void);
int avt_where_y (void);

/* maximum positions (whole text-field) */
int avt_get_max_x (void);
int avt_get_max_y (void);

/*
 * put cusor to specified coordinates
 */
void avt_move_x (int x);
void avt_move_y (int y);

/* 
 * delete num lines, starting from line
 * the rest ist scrolled up
 */
void avt_delete_lines (int line, int num);

/* 
 * insert num lines, starting at line
 * the rest ist scrolled down
 */
void avt_insert_lines (int line, int num);

/***********************************************************************/
/* audio stuff */

/* must be called AFTER avt_initialize! */
int avt_initialize_audio (void);

/* 
 * no longer needed, 
 * this is executed automatically by avt_quit() 
 * this function is only there for backward compatiblity
 */
void avt_quit_audio (void);

/* 
 * loads a wave file 
 * supported: PCM, MS-ADPCM, IMA-ADPCM
 */
avt_audio_t *avt_load_wave_file (const char *file);

/*
 * loads wave data from memory
 * must still be freed with avt_free_audio!
 */
avt_audio_t *avt_load_wave_data (void *data, int datasize);

/*
 * frees memory of a loaded sound
 */
void avt_free_audio (avt_audio_t * snd);

/*
 * plays a sound
 */
int avt_play_audio (avt_audio_t * snd, avt_bool_t doloop);

/*
 * wait until the sound ends
 * this stops a loop, but still plays to the end of the sound
 */
int avt_wait_audio_end (void);

/* stops audio */
void avt_stop_audio (void);


#ifdef __cplusplus
/* *INDENT-OFF* */
}
#endif /* __cplusplus */
#endif /* _akfavatar_h */
