/*
 * AKFAvatar library - for giving your programs a graphical Avatar
 * Copyright (c) 2007, 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
 *
 * needed:
 *  SDL1.2 (recommended: SDL1.2.11 or later (but not 1.3!))
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

#ifndef _akfavatar_h
#define _akfavatar_h

/* SDL redefines main on some systems */
#if defined(__WIN32__) ||  defined(__MACOS__) || defined(__MACOSX__)
#  include "SDL.h"
#endif

/* to get the systems definition of wchar_t */
#include <stddef.h>

#define AKFAVATAR 1

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
#define AVT_MAKE_BOOL(x) ((x) != 0)

/* for avt_set_text_delay and avt_set_flip_page_delay */
#define AVT_DEFAULT_TEXT_DELAY 75
#define AVT_DEFAULT_FLIP_PAGE_DELAY 2700

/* for avt_text_direction */
#define AVT_LEFT_TO_RIGHT 0
#define AVT_RIGHT_TO_LEFT 1

/*
 * example: avt_wait(AVT_SECONDS(2.5)) waits 2.5 seconds 
 */
#define AVT_SECONDS(x) ((x)*1000)

/*
 * makros for marking deprecated functions in this header,
 * or possibly unused parameters
 */
#ifdef __GNUC__
#  define AVT_DEPRECATED __attribute__ ((__deprecated__))
#  define AVT_UNUSED __attribute__ ((__unused__))
#else
#  define AVT_DEPRECATED
#  define AVT_UNUSED
#endif /* __GNUC__ */

#ifdef __cplusplus
#  define AVT_BEGIN_DECLS  extern "C" {
#  define AVT_END_DECLS    }
#else
#  define AVT_BEGIN_DECLS
#  define AVT_END_DECLS
#endif /* __cplusplus */

/* for later use */
#define AVT_API  extern

AVT_BEGIN_DECLS

/*
 * boolean are chars for this library
 * to make language bindings more easy
 * (you can use stdbool.h in your program, it's compatible)
 */
typedef char avt_bool_t;

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

/*
 * type for mousehandler
 * see avt_register_mousehandler
 */
typedef void (*avt_mousehandler) (int button, avt_bool_t pressed,
				  int x, int y);

/* base fnctions */

/* 
 * initialize the avatar system
 * mode is either WINDOW or FULLSCREEN or FULLSCREENNOSWITCH
 * the original image is freed in this function!
 * the image may be NULL if no avatar should be shown
 * title and icontitle may also be NULL
 */
AVT_API int avt_initialize (const char *title,
			    const char *icontitle,
			    avt_image_t * image,
			    int mode);

/*
 * quit the avatar system
 * can be used with atexit
 */
AVT_API void avt_quit (void);

/*
 * call avt_wait_button (); avt_move_out (); avt_quit ();
 * can be used with atexit
 */
AVT_API void avt_button_quit (void);

/* which version */
AVT_API const char *avt_version (void);

/* copyright information */
AVT_API const char *avt_copyright (void);

/* license information */
AVT_API const char *avt_license (void);

/* is it initialized? */
AVT_API avt_bool_t avt_initialized (void);

/* 0 = normal; 1 = quit-request; -1 = error */
AVT_API int avt_get_status (void);

/* set status */
AVT_API void avt_set_status (int status);

/* get error message */
AVT_API char *avt_get_error (void);

/*
 * change the title and/or the icontitle
 * use NULL for the unchanged part
 * in newer SDL-versions it is to be encoded in UTF-8
 * if possible stick to ASCII for compatibility
 */
AVT_API void avt_set_title (const char *title, const char *icontitle);

/*
 * set text direction
 * the cursor is moved to start of the line
 * in a text, you might want to call avt_newline after that
 */
AVT_API void avt_text_direction (int direction);

/*
 * set the baloon width and height in number of characters
 * 0 or less for maximum width
 * if it's actually changed, the balloon is redrawn and emptied
 * see also avt_get_max_x () and avt_get_max_y ()
 */
AVT_API void avt_set_balloon_size (int height, int width);
AVT_API void avt_set_balloon_width (int width);
AVT_API void avt_set_balloon_height (int height);

/* activate the text cursor? (default: no) */
AVT_API void avt_activate_cursor (avt_bool_t on);

/*
 * get the default avatar image
 * use this with avt_initialize
 */
AVT_API avt_image_t *avt_default (void);

/*
 * import an avatar from XPM data
 */
AVT_API avt_image_t *avt_import_XPM (char **xpm);

/*
 * RGB gimp_image
 * for importing an avatar
 * also uses avt_make_transparent
 */
AVT_API avt_image_t *avt_import_gimp_image (void *gimp_image);

/*
 * import avatar from image data
 */
AVT_API avt_image_t *avt_import_image_data (void *img, int imgsize);

/*
 * import avatar from file
 */
AVT_API avt_image_t *avt_import_image_file (const char *file);

/*
 * import avatar from stream
 * the stream is internally casted to (FILE *)
 */
AVT_API avt_image_t *avt_import_image_stream (void *stream);

/*
 * change avatar image while running
 * if the avatar is visible, the screen gets cleared
 * the original image is freed in this function!
 * the image may be NULL if no avatar should be shown
 * on error AVT_ERROR is set and returned 
 */
AVT_API int avt_change_avatar_image (avt_image_t * image);

/*
 * free avt_image_t images
 * allocated with avt_import_gimp_image or avt_import_imagefile
 * (automatically called in avt_initialize)
 */
AVT_API void avt_free_image (avt_image_t * image);

/*
 * make background transparent
 * pixel in the upper left corner is supposed to be the background color
 */
AVT_API avt_image_t *avt_make_transparent (avt_image_t * image);

/*
 * get color values for a given color-name
 * returns 0 on success or -1 on error
 */
AVT_API int avt_name_to_color (const char *name,
			       int *red, int *green, int *blue);

/* get color name of given number, or NULL on error */
AVT_API char *avt_get_color_name (int nr);

/*
 * define the background color
 * values in the range 0x00 .. 0xFF
 * can and should be called before avt_initialize
 * if the balloon is visible, it is cleared
 */
AVT_API void avt_set_background_color (int red, int green, int blue);
AVT_API void avt_set_background_color_name (const char *name);

/*
 * define the balloon color
 * values in the range 0x00 .. 0xFF
 * can be called before avt_initialize
 * the text-background-color is set to the balloon-color too
 * if the balloon is visible, it is cleared
 */
AVT_API void avt_set_balloon_color (int red, int green, int blue);
AVT_API void avt_set_balloon_color_name (const char *name);

/*
 * change the text color
 * values in the range 0x00 .. 0xFF
 */
AVT_API void avt_set_text_color (int red, int green, int blue);
AVT_API void avt_set_text_color_name (const char *name);
AVT_API void avt_set_text_background_color (int red, int green, int blue);
AVT_API void avt_set_text_background_color_name (const char *name);

/* set text background to balloon color */
AVT_API void avt_set_text_background_ballooncolor (void);

/* set underlined mode on or off */
AVT_API void avt_underlined (avt_bool_t onoff);

/* get underlined mode */
AVT_API avt_bool_t avt_get_underlined (void);

/* set bold mode on or off (not recommended) */
AVT_API void avt_bold (avt_bool_t onoff);

/* get bold mode */
AVT_API avt_bool_t avt_get_bold (void);

/* set inverse mode on or off */
AVT_API void avt_inverse (avt_bool_t onoff);

/* get inverse mode */
AVT_API avt_bool_t avt_get_inverse (void);

/* set default color and switch off bold, underlined, inverse */
AVT_API void avt_normal_text (void);

/*
 * delay time for text-writing
 * default: AVT_DEFAULT_TEXT_DELAY
 */
AVT_API void avt_set_text_delay (int delay);

/*
 * delay time for page flipping
 * default: AVT_DEFAULT_FLIP_PAGE_DELAY
 */
AVT_API void avt_set_flip_page_delay (int delay);

/* don't use this anymore, it is about to be removed */
AVT_API void avt_set_delays (int text, int flip_page) AVT_DEPRECATED;

/*
 * reserve single keys (Esc, F11)
 * use this with avt_register_keyhandler
 */
AVT_API void avt_reserve_single_keys (avt_bool_t onoff);

/* just for backward compatiblity, don't use it */
AVT_API void avt_stop_on_esc (avt_bool_t on) AVT_DEPRECATED;

/* register an external keyhandler */
AVT_API void avt_register_keyhandler (avt_keyhandler handler);

/* register an external mousehandler
 *
 * it is only called, when a mouse-button is pressed or released inside
 * of the balloon. The coordinates are the character positions.
 */
AVT_API void avt_register_mousehandler (avt_mousehandler handler);

/*
 * switch to fullscreen or window mode
 */
AVT_API void avt_switch_mode (int mode);

/*
 * toggle fullscrenn mode
 */
AVT_API void avt_toggle_fullscreen (void);

/*
 * prints a L'\0' terminated string in the balloon
 * if there is no balloon, it is drawn
 * if there is no avatar, it is shown (not moved in)
 * interprets control chars
 */
AVT_API int avt_say (const wchar_t * txt);

/*
 * writes string with given length in the balloon
 * the string needn't be terminated and can contain binary zeros
 * if there is no balloon, it is drawn
 * if there is no avatar, it is shown (not moved in)
 * interprets control characters
 */
AVT_API int avt_say_len (const wchar_t * txt, const int len);

/*
 * writes a single character in the balloon
 * if there is no balloon, it is drawn
 * if there is no avatar, it is shown (not moved in)
 * interprets control characters
 */
AVT_API int avt_put_character (const wchar_t ch);

/* set encoding for mb functions */
AVT_API int avt_mb_encoding (const char *encoding);

/*
 * decode a string into wchar_t
 * size in bytes
 * returns number of characters in dest (without the termination zero)
 * dest must be freed by caller with avt_free
 */
AVT_API int avt_mb_decode (wchar_t ** dest, const char *src, const int size);

/*
 * encode a string from wchar_t
 * len is the length
 * returns number of characters in dest (without the termination zero)
 * dest must be freed by caller with avt_free
 * (the size of dest may be much more than needed)
 */
AVT_API int avt_mb_encode (char **dest, const wchar_t * src, const int len);

/* free memory allocated by this library */
AVT_API void avt_free (void *ptr);

/*
 * like avt_say,
 * but converts from a given charset encoding
 * (see avt_mb_encoding)
 * the text is a 0 terminated C-String
 */
AVT_API int avt_say_mb (const char *txt);

/*
 * the same with a given length
 * the string needn't be terminated then 
 * and can contain binary zeros
 */
AVT_API int avt_say_mb_len (const char *txt, int len);

/*
 * get a character from the keyboard
 * only for printable characters, not for function keys
 * (ch is just one character, not a string)
 */
AVT_API int avt_get_key (wchar_t * ch);

/*
 * avt_choice
 * result:        result code, first item is 1
 * start_line:    line, where choice begins
 * items:         number of items/lines
 * key:           first key, like '1' or 'a', 0 for no keys
 * back, forward: whether first/last entry is a back/forward function
 *
 * returns AVT_ERROR and sets _avt_STATUS when it cannot get enough memory
 */
AVT_API int
avt_choice (int *result, int start_line, int items, int key,
	    avt_bool_t back, avt_bool_t forward);

/* deprecated, use avt_choice */
AVT_API int avt_menu (wchar_t * ch, int menu_start, int menu_end, 
                      wchar_t start_code, avt_bool_t back, 
                      avt_bool_t forward) AVT_DEPRECATED;

/* deprecated, use avt_choice */
AVT_API int
avt_get_menu (wchar_t * ch, int menu_start, int menu_end, wchar_t start_code)
AVT_DEPRECATED;

/*
 * get string (just one line)
 * the maximum length is LINELENGTH-1
 * size is the size of s in bytes (not the length)
 *
 * (I don't use size_t for better compatiblity with other languages)
 */
AVT_API int avt_ask (wchar_t * s, const int size);

/*
 * like avt_ask,
 * but converts to a given encoding
 *
 * for UTF-8 encoding it should have a capacity of 
 * 4 * LINELENGTH Bytes
 */
AVT_API int avt_ask_mb (char *s, const int size);

/*
 * new line
 * same as \n in avt_say
 */
AVT_API int avt_new_line (void);

/*
 * wait a while and then clear the textfield
 * same as \f in avt_say
 */
AVT_API int avt_flip_page (void);

/* update, ie handle events and give some time to other processes */
/* use this in a longer loop in your program */
AVT_API int avt_update (void);

/* wait a while */
AVT_API int avt_wait (int milliseconds);

/* wait for a keypress while displaying a button */
AVT_API int avt_wait_button (void);

/*
 * show positive or negative buttons
 * keys for positive: + 1 Enter
 * keys for negative: - 0 Backspace
 *
 * returns the result as boolean
 * on error or quit request AVT_FALSE is returned and the status is set
 * you should check the status with avt_get_status()
 */
AVT_API avt_bool_t avt_decide (void);

/* wait for a keypress  (deprecated: use avt_wait_button) */
AVT_API int avt_wait_key (const wchar_t * message) AVT_DEPRECATED;

/*
 * like avt_waitkey,
 * but converts from a given charset encoding
 * (deprecated: use avt_wait_button) 
 */
AVT_API int avt_wait_key_mb (char *message) AVT_DEPRECATED;


/* functions for extended use */

/*
 * set a viewport (sub-area of the textarea)
 * upper left corner is 1, 1
 */
AVT_API void avt_viewport (int x, int y, int width, int height);

/* show an empty screen with the background color */
AVT_API void avt_clear_screen (void);

/* show just the avatar without the balloon */
AVT_API void avt_show_avatar (void);

/*
 * load image and show it
 * if SDL_image isn't available then
 * XPM and uncompressed BMP are still supported
 * on error it returns AVT_ERROR without changing the status
 * if it succeeds call avt_wait or avt_waitkey 
 */
AVT_API int avt_show_image_file (const char *file);

/*
 * load image from stream and show it
 * the stream is internally casted to (FILE *)
 * if SDL_image isn't available then
 * XPM and uncompressed BMP are still supported
 * on error it returns AVT_ERROR without changing the status
 * if it succeeds call avt_wait or avt_waitkey 
 */
AVT_API int avt_show_image_stream (void *stream);

/*
 * show image from image data
 * if SDL_image isn't available then
 * XPM and uncompressed BMP are still supported
 * on error it returns AVT_ERROR without changing the status
 * if it succeeds call avt_wait or avt_waitkey 
 */
AVT_API int avt_show_image_data (void *img, int imgsize);

/*
 * show image from XPM data
 * on error it returns AVT_ERROR without changing the status
 * if it succeeds call avt_wait or avt_waitkey 
 */
AVT_API int avt_show_image_XPM (char **xpm);

/*
 * show gimp image
 * on error it returns AVT_ERROR without changing the status
 */
AVT_API int avt_show_gimp_image (void *gimp_image);

/*
 * like avt_show_avatar, but the avatar is moved in
 */
AVT_API int avt_move_in (void);

/*
 * move the avatar out => empty screen
 */
AVT_API int avt_move_out (void);

/*
 * show final credits
 */
AVT_API int avt_credits (const wchar_t *text, avt_bool_t centered);
AVT_API int avt_credits_mb (const char *text, avt_bool_t centered);

/*
 * make a short sound, when audio is initialized
 * else it is the same as avt_flash
 * same as with \a in avt_say
 * the sound is actually not a bell ;-)
 */
AVT_API void avt_bell (void);

/*
 * visual flash of the screen
 */
AVT_API void avt_flash (void);

/* 
 * clears the viewport
 * if there is no balloon yet, it is drawn
 */
AVT_API void avt_clear (void);

/*
 * clears from cursor position down the viewport
 * if there is no balloon yet, it is drawn
 */
AVT_API void avt_clear_down (void);

/*
 * clears from cursor position up the viewport
 * if there is no balloon yet, it is drawn
 */
AVT_API void avt_clear_up (void);

/*
 * clear end of line
 * depending on text direction
 */
AVT_API void avt_clear_eol (void);

/*
 * clear beginning of line
 * depending on text direction
 */
AVT_API void avt_clear_bol (void);

/* clear line */
AVT_API void avt_clear_line (void);

/*
 * forward one character position
 * ie. print a space
 */
AVT_API int avt_forward (void);

/* delete last caracter */
AVT_API void avt_backspace (void);

/* insert spaces at current position (move rest of line) */
AVT_API void avt_insert_spaces (int num);

/* delete num characters at current position (move rest of line) */
AVT_API void avt_delete_characters (int num);

/*
 * erase num characters from current position
 * don't move the cursor or the rest of the line
 */
AVT_API void avt_erase_characters (int num);

/*
 * set scroll mode
 * -1 = off, 0 = page-flipping, 1 = normal
 */
AVT_API void avt_set_scroll_mode (int mode);
AVT_API int avt_get_scroll_mode (void);

/* set newline mode (default: on) */
AVT_API void avt_newline_mode (avt_bool_t mode);

/* set auto-margin mode (default: on) */
AVT_API void avt_auto_margin (avt_bool_t mode);

/*
 * origin mode
 * AVT_FALSE: origin (1,1) is always the top of the textarea
 * AVT_TRUE:  origin (1,1) is the top of the viewport
 */
AVT_API void avt_set_origin_mode (avt_bool_t mode);
AVT_API avt_bool_t avt_get_origin_mode (void);

/*
 * handle coordinates
 *
 * the coordinates start with 1, 1 
 * in the upper left corner
 * and are independent from the text direction
 */

/* get position in the viewport */
AVT_API int avt_where_x (void);
AVT_API int avt_where_y (void);

/*
 * is the cursor in the home position? 
 * (also works for right-to-left writing)
 */
AVT_API avt_bool_t avt_home_position (void);

/* maximum positions (whole text-field) */
AVT_API int avt_get_max_x (void);
AVT_API int avt_get_max_y (void);

/* put cusor to specified coordinates */
AVT_API void avt_move_x (int x);
AVT_API void avt_move_y (int y);
AVT_API void avt_move_xy (int x, int y);

/* save and restore current cursor position */
AVT_API void avt_save_position (void);
AVT_API void avt_restore_position (void);

/* go to next or last tab stop */
AVT_API void avt_next_tab (void);
AVT_API void avt_last_tab (void);

/* reset tab stops to every eigth column */
AVT_API void avt_reset_tab_stops (void);

/* clear all tab stops */
AVT_API void avt_clear_tab_stops (void);

/* set or clear a tab in position x */
AVT_API void avt_set_tab (int x, avt_bool_t onoff);

/*
 * delete num lines, starting from line
 * the rest ist scrolled up
 */
AVT_API void avt_delete_lines (int line, int num);

/*
 * insert num lines, starting at line
 * the rest ist scrolled down
 */
AVT_API void avt_insert_lines (int line, int num);

/***********************************************************************/
/* audio stuff */

/* must be called AFTER avt_initialize! */
AVT_API int avt_initialize_audio (void);

/*
 * no longer needed,
 * this is executed automatically by avt_quit()
 * this function is only there for backward compatiblity
 */
AVT_API void avt_quit_audio (void) AVT_DEPRECATED;

/*
 * supported audio formats:
 * AU:  linear PCM with up to 32Bit, mu-law, A-law
 * WAV: linear PCM with up to 16Bit, MS-ADPCM, IMA-ADPCM
 * Both: mono or stereo
 */

/*
 * loads an audio file in AU or Wave format
 * not for headerless formats
 */
AVT_API avt_audio_t *avt_load_audio_file (const char *file);

/*
 * loads audio in AU or Wave format from a stream
 * the stream is internally casted to (FILE *)
 * not for headerless formats
 */
AVT_API avt_audio_t *avt_load_audio_stream (void *stream);

/*
 * loads audio in AU or Wave format from memory
 * must still be freed with avt_free_audio!
 */
AVT_API avt_audio_t *avt_load_audio_data (void *data, int datasize);

/*
 * loads a wave file 
 * deprecated: use avt_load_audio_file
 */
AVT_API avt_audio_t *avt_load_wave_file (const char *file) AVT_DEPRECATED;

/*
 * loads wave data from memory
 * deprecated: use avt_load_audio_file
 */
AVT_API avt_audio_t *avt_load_wave_data (void *data, int datasize) 
        AVT_DEPRECATED;

/* values for audio_type */
#define AVT_AUDIO_UNKNOWN   0  /* doesn't play */
#define AVT_AUDIO_U8        1  /* unsigned 8 Bit */
#define AVT_AUDIO_S8        2  /* signed 8 Bit */
#define AVT_AUDIO_U16LE     3  /* unsigned 16 Bit little endian */
#define AVT_AUDIO_U16BE     4  /* unsigned 16 Bit big endian */
#define AVT_AUDIO_U16SYS    5  /* unsigned 16 Bit system's endianess */
#define AVT_AUDIO_S16LE     6  /* signed 16 Bit little endian */
#define AVT_AUDIO_S16BE     7  /* signed 16 Bit big endian */
#define AVT_AUDIO_S16SYS    8  /* signed 16 Bit system's endianess */
#define AVT_AUDIO_MULAW   100  /* 8 Bit mu-law (u-law) */
#define AVT_AUDIO_ALAW    101  /* 8 Bit A-Law */

/* for channels */
#define AVT_AUDIO_MONO      1
#define AVT_AUDIO_STEREO    2

/*
 * loads raw audio data from memory
 * the data buffer is copied and can be freed immediately
 *
 * audio_type is one of the AVT_AUDIO_* constants
 * channels is AVT_MONO or AVT_STEREO
 *
 * must be freed with avt_free_audio!
 */
AVT_API avt_audio_t *avt_load_raw_audio_data (void *data, int data_size,
			int samplingrate, int audio_type, int channels);

/*
 * frees memory of a loaded sound
 */
AVT_API void avt_free_audio (avt_audio_t * snd);

/*
 * plays a sound
 */
AVT_API int avt_play_audio (avt_audio_t * snd, avt_bool_t doloop);

/*
 * wait until the sound ends
 * this stops a loop, but still plays to the end of the sound
 */
AVT_API int avt_wait_audio_end (void);

/* stops audio */
AVT_API void avt_stop_audio (void);


AVT_END_DECLS

#endif /* _akfavatar_h */
