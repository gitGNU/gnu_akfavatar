/*
 * multibyte encoding support for AKFAvatar
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013
 * Andreas K. Foerster <info@akfoerster.de>
 *
 * required standards: C99, POSIX.1-2001
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

#include "akfavatar.h"
#include "avtinternals.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <iconv.h>
#include <errno.h>
#include <iso646.h>

/*
 * Most iconv implementations support "" for the systems encoding.
 * You should redefine this macro if and only if it is not supported.
 */
#ifndef SYSTEMENCODING
#  define SYSTEMENCODING  ""
#endif

#ifndef WCHAR_ENCODING

#  ifndef WCHAR_MAX
#    error "please define WCHAR_ENCODING (no autodetection possible)"
#  endif

#  if (WCHAR_MAX <= 255)
#    define WCHAR_ENCODING "ISO-8859-1"
#  elif (WCHAR_MAX <= 65535U)
#    if (AVT_BIG_ENDIAN == AVT_BYTE_ORDER)
#      define WCHAR_ENCODING "UTF-16BE"
#    else // little endian
#      define WCHAR_ENCODING "UTF-16LE"
#    endif // little endian
#  else	// (WCHAR_MAX > 65535U)
#    if (AVT_BIG_ENDIAN == AVT_BYTE_ORDER)
#      define WCHAR_ENCODING "UTF-32BE"
#    else // little endian
#      define WCHAR_ENCODING "UTF-32LE"
#    endif // little endian
#  endif // (WCHAR_MAX > 65535U)

#endif // not WCHAR_ENCODING

#define ICONV_UNINITIALIZED   (iconv_t)(-1)

static char avt_encoding[100];

// conversion descriptors for text input and output
static iconv_t output_cd = ICONV_UNINITIALIZED;
static iconv_t input_cd = ICONV_UNINITIALIZED;

extern int
avt_mb_encoding (const char *encoding)
{
  if (not encoding)
    encoding = "";

  /*
   * check if it is the result of avt_get_mb_encoding()
   * or the same encoding
   */
  if (encoding == avt_encoding or strcmp (encoding, avt_encoding) == 0)
    return _avt_STATUS;

  strncpy (avt_encoding, encoding, sizeof (avt_encoding));
  avt_encoding[sizeof (avt_encoding) - 1] = '\0';

  // if encoding is "" and SYSTEMENCODING is not ""
  if (encoding[0] == '\0' and SYSTEMENCODING[0] != '\0')
    encoding = SYSTEMENCODING;

  // output

  //  if it is already open, close it first
  if (output_cd != ICONV_UNINITIALIZED)
    iconv_close (output_cd);

  // initialize the conversion framework
  output_cd = iconv_open (WCHAR_ENCODING, encoding);

  // check if it was successfully initialized
  if (output_cd == ICONV_UNINITIALIZED)
    {
      avt_set_error ("encoding not supported for output");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  // input

  //  if it is already open, close it first
  if (input_cd != ICONV_UNINITIALIZED)
    iconv_close (input_cd);

  // initialize the conversion framework
  input_cd = iconv_open (encoding, WCHAR_ENCODING);

  // check if it was successfully initialized
  if (input_cd == ICONV_UNINITIALIZED)
    {
      iconv_close (output_cd);
      output_cd = ICONV_UNINITIALIZED;
      avt_set_error ("encoding not supported for input");
      _avt_STATUS = AVT_ERROR;
      return _avt_STATUS;
    }

  return _avt_STATUS;
}

extern char *
avt_get_mb_encoding (void)
{
  return avt_encoding;
}

// size in bytes
// returns length (number of characters)
extern size_t
avt_mb_decode_buffer (wchar_t * dest, size_t dest_size,
		      const char *src, size_t src_size)
{
  static char rest_buffer[10];
  static size_t rest_bytes = 0;
  char *outbuf;
  char *inbuf, *restbuf;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  // check if sizes are useful
  if (not dest or not dest_size or not src or not src_size)
    return (size_t) (-1);

  // check if encoding was set
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = src_size;
  inbuf = (char *) src;

  // leave room for terminator
  outbytesleft = dest_size - sizeof (wchar_t);
  outbuf = (char *) dest;

  restbuf = (char *) rest_buffer;

  // if there is a rest from last call, try to complete it
  while (rest_bytes > 0 and inbytesleft > 0)
    {
      rest_buffer[rest_bytes++] = *inbuf;
      inbuf++;
      inbytesleft--;
      returncode =
	iconv (output_cd, &restbuf, &rest_bytes, &outbuf, &outbytesleft);

      if (returncode != (size_t) (-1))
	rest_bytes = 0;
      else if (errno != EINVAL)
	{
	  *((wchar_t *) outbuf) = L'\xFFFD';

	  outbuf += sizeof (wchar_t);
	  outbytesleft -= sizeof (wchar_t);
	  rest_bytes = 0;
	}
    }

  // do the conversion
  returncode =
    iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  // handle invalid characters
  while (returncode == (size_t) (-1) and errno == EILSEQ)
    {
      inbuf++;
      inbytesleft--;

      *((wchar_t *) outbuf) = L'\xFFFD';

      outbuf += sizeof (wchar_t);
      outbytesleft -= sizeof (wchar_t);
      returncode =
	iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  // check for incomplete sequences and put them into the rest_buffer
  if (returncode == (size_t) (-1) and errno == EINVAL
      and inbytesleft <= sizeof (rest_buffer))
    {
      rest_bytes = inbytesleft;
      memcpy ((void *) &rest_buffer, inbuf, rest_bytes);
    }

  // ignore E2BIG - just put in as much as fits

  // terminate outbuf
  *((wchar_t *) outbuf) = L'\0';

  return ((dest_size - sizeof (wchar_t) - outbytesleft) / sizeof (wchar_t));
}

// size in bytes
// dest must be freed by caller
extern size_t
avt_mb_decode (wchar_t ** dest, const char *src, size_t src_size)
{
  size_t dest_size;
  size_t length;

  if (not dest)
    return (size_t) (-1);

  *dest = NULL;

  if (not src or not src_size)
    return (size_t) (-1);

  // get enough space
  // a character may be 4 Bytes, also in UTF-16
  // plus the terminator
  dest_size = src_size * 4 + sizeof (wchar_t);

  // minimal string size
  if (dest_size < 8)
    dest_size = 8;

  *dest = (wchar_t *) malloc (dest_size);

  if (not * dest)
    return (size_t) (-1);

  length = avt_mb_decode_buffer (*dest, dest_size, src, src_size);

  if (length == (size_t) (-1) or length == 0)
    {
      free (*dest);
      *dest = NULL;
      return (size_t) (-1);
    }

  return length;
}

extern size_t
avt_mb_encode_buffer (char *dest, size_t dest_size, const wchar_t * src,
		      size_t len)
{
  char *outbuf;
  char *inbuf;
  size_t inbytesleft, outbytesleft;

  // check if sizes are useful
  if (not dest or not dest_size or not src or not len)
    return (size_t) (-1);

  // check if encoding was set
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  inbytesleft = len * sizeof (wchar_t);
  inbuf = (char *) src;

  // leave room for terminator
  outbytesleft = dest_size - sizeof (char);
  outbuf = dest;

  // do the conversion
  (void) iconv (input_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
  // ignore errors

  // terminate outbuf
  *outbuf = '\0';

  return (dest_size - sizeof (char) - outbytesleft);
}

extern size_t
avt_mb_encode (char **dest, const wchar_t * src, size_t len)
{
  size_t dest_size, size;

  if (not dest)
    return (size_t) (-1);

  *dest = NULL;

  // check if len is useful
  if (not src or not len)
    return (size_t) (-1);

  // get enough space
  // UTF-8 may need 4 bytes per character
  // +1 for the terminator
  dest_size = len * 4 + 1;
  *dest = (char *) malloc (dest_size);

  if (not * dest)
    return (size_t) (-1);

  size = avt_mb_encode_buffer (*dest, dest_size, src, len);

  if (size == (size_t) (-1) or size == 0)
    {
      free (*dest);
      *dest = NULL;
      return (size_t) (-1);
    }

  return size;
}

extern size_t
avt_recode_buffer (const char *tocode, const char *fromcode,
		   char *dest, size_t dest_size, const char *src,
		   size_t src_size)
{
  iconv_t cd;
  char *outbuf, *inbuf;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  // check if sizes are useful
  if (not dest or not dest_size or not src or not src_size)
    return (size_t) (-1);

  // NULL as code means the encoding, which was set

  if (not tocode)
    tocode = avt_encoding;

  if (not fromcode)
    fromcode = avt_encoding;

  cd = iconv_open (tocode, fromcode);
  if (cd == (iconv_t) (-1))
    return (size_t) (-1);

  inbuf = (char *) src;
  inbytesleft = src_size;

  /*
   * I reserve 4 Bytes for the terminator,
   * in case of using UTF-32
   */
  outbytesleft = dest_size - 4;
  outbuf = dest;

  // do the conversion
  returncode = iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  // jump over invalid characters
  while (returncode == (size_t) (-1) and errno == EILSEQ)
    {
      inbuf++;
      inbytesleft--;
      returncode = iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }

  // ignore E2BIG - just put in as much as fits

  iconv_close (cd);

  // terminate outbuf (4 Bytes were reserved)
  memset (outbuf, 0, 4);

  return (dest_size - 4 - outbytesleft);
}

// dest must be freed by the caller
extern size_t
avt_recode (const char *tocode, const char *fromcode,
	    char **dest, const char *src, size_t src_size)
{
  iconv_t cd;
  char *outbuf, *inbuf;
  size_t dest_size;
  size_t inbytesleft, outbytesleft;
  size_t returncode;

  if (not dest)
    return (size_t) (-1);

  *dest = NULL;

  // check if size is useful
  if (not src or not src_size)
    return (size_t) (-1);

  // NULL as code means the encoding, which was set

  if (not tocode)
    tocode = avt_encoding;

  if (not fromcode)
    fromcode = avt_encoding;

  cd = iconv_open (tocode, fromcode);
  if (cd == (iconv_t) (-1))
    return (size_t) (-1);

  inbuf = (char *) src;
  inbytesleft = src_size;

  /*
   * I reserve 4 Bytes for the terminator,
   * in case of using UTF-32
   */

  // guess it's the same size
  dest_size = src_size + 4;
  *dest = (char *) malloc (dest_size);

  if (not * dest)
    {
      iconv_close (cd);
      return -1;
    }

  outbuf = *dest;
  outbytesleft = dest_size - 4;	// reserve 4 Bytes for terminator

  // do the conversion
  while (inbytesleft > 0)
    {
      returncode = iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

      // check for fatal errors
      if (returncode == (size_t) (-1))
	switch (errno)
	  {
	  case E2BIG:		// needs more memory
	    {
	      ptrdiff_t old_size = outbuf - *dest;

	      dest_size *= 2;
	      *dest = (char *) realloc (*dest, dest_size);
	      if (not * dest)
		{
		  iconv_close (cd);
		  return (size_t) (-1);
		}

	      outbuf = *dest + old_size;
	      outbytesleft = dest_size - old_size - 4;
	      // reserve 4 bytes for terminator again
	    }
	    break;

	  case EILSEQ:
	    inbuf++;		// jump over invalid chars bytewise
	    inbytesleft--;
	    break;

	  case EINVAL:		// incomplete sequence
	  default:
	    inbytesleft = 0;	// cannot continue
	    break;
	  }
    }

  iconv_close (cd);

  // terminate outbuf (4 Bytes were reserved)
  memset (outbuf, 0, 4);

  return (dest_size - 4 - outbytesleft);
}

extern int
avt_say_mb (const char *txt)
{
  if (_avt_STATUS == AVT_NORMAL and avt_initialized ())
    avt_say_mb_len (txt, strlen (txt));

  return _avt_STATUS;
}

extern int
avt_say_mb_len (const char *txt, size_t len)
{
  wchar_t wctext[AVT_LINELENGTH];
  char *inbuf, *outbuf;
  size_t inbytesleft, outbytesleft, nconv;
  int err;

  static char rest_buffer[10];
  static size_t rest_bytes = 0;

  if (_avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return _avt_STATUS;

  // check if encoding was set
  if (output_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  if (len)
    inbytesleft = len;
  else
    inbytesleft = strlen (txt);

  inbuf = (char *) txt;

  // if there is a rest from last call, try to complete it
  while (rest_bytes > 0 and rest_bytes < sizeof (rest_buffer)
	 and inbytesleft > 0)
    {
      char *rest_buf;
      size_t rest_bytes_left;

      outbuf = (char *) wctext;
      outbytesleft = sizeof (wctext);

      rest_buffer[rest_bytes] = *inbuf;
      rest_bytes++;
      inbuf++;
      inbytesleft--;

      rest_buf = (char *) rest_buffer;
      rest_bytes_left = rest_bytes;

      nconv =
	iconv (output_cd, &rest_buf, &rest_bytes_left, &outbuf,
	       &outbytesleft);
      err = errno;

      if (nconv != (size_t) (-1))	// no error
	{
	  avt_say_len (wctext,
		       (sizeof (wctext) - outbytesleft) / sizeof (wchar_t));
	  rest_bytes = 0;
	}
      else if (err != EINVAL)	// any error, but incomplete sequence
	{
	  avt_put_char (0xFFFD);	// broken character
	  rest_bytes = 0;
	}
    }

  // convert and display the text
  while (inbytesleft > 0)
    {
      outbuf = (char *) wctext;
      outbytesleft = sizeof (wctext);

      nconv = iconv (output_cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
      err = errno;

      avt_say_len (wctext,
		   (sizeof (wctext) - outbytesleft) / sizeof (wchar_t));

      if (nconv == (size_t) (-1))
	{
	  if (err == EILSEQ)	// broken character
	    {
	      avt_put_char (0xFFFD);
	      inbuf++;
	      inbytesleft--;
	    }
	  else if (err == EINVAL)	// incomplete sequence
	    {
	      rest_bytes = inbytesleft;
	      memcpy (&rest_buffer, inbuf, rest_bytes);
	      inbytesleft = 0;
	    }
	}
    }

  return _avt_STATUS;
}

/*
 * for avt_tell_mb_len  we must convert the whole text
 * or else analyzing it would be too complicated
 */
extern int
avt_tell_mb_len (const char *txt, size_t len)
{
  if (_avt_STATUS == AVT_NORMAL and avt_initialized ())
    {
      if (not len)
	len = strlen (txt);

      wchar_t *wctext;
      int wclen = avt_mb_decode (&wctext, txt, len);

      if (wctext)
	{
	  avt_tell_len (wctext, wclen);
	  free (wctext);
	}
    }

  return _avt_STATUS;
}

extern int
avt_tell_mb (const char *txt)
{
  if (_avt_STATUS == AVT_NORMAL and avt_initialized ())
    avt_tell_mb_len (txt, strlen (txt));

  return _avt_STATUS;
}

extern avt_char
avt_input_mb (char *s, size_t size, const char *default_text,
	      int position, int mode)
{
  avt_char ch;
  wchar_t ws[size];
  wchar_t ws_default_text[size];

  if (_avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return _avt_STATUS;

  // check if encoding was set
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  ws_default_text[0] = L'\0';

  if (default_text and default_text[0] != L'\0')
    avt_mb_decode_buffer (ws_default_text, sizeof (ws_default_text),
			  default_text, strlen (default_text) + 1);

  ch = avt_input (ws, sizeof (ws), ws_default_text, position, mode);

  s[0] = '\0';

  // if a halt is requested, don't bother with the conversion
  if (_avt_STATUS != AVT_NORMAL)
    return AVT_KEY_NONE;

  avt_mb_encode_buffer (s, size, ws, wcslen (ws));

  return ch;
}

extern int
avt_ask_mb (char *s, size_t size)
{
  wchar_t ws[AVT_LINELENGTH + 1];
  char *inbuf;
  size_t inbytesleft, outbytesleft;

  if (_avt_STATUS != AVT_NORMAL or not avt_initialized ())
    return _avt_STATUS;

  // check if encoding was set
  if (input_cd == ICONV_UNINITIALIZED)
    avt_mb_encoding (MB_DEFAULT_ENCODING);

  avt_input (ws, sizeof (ws), NULL, -1, 0);

  s[0] = '\0';

  // if a halt is requested, don't bother with the conversion
  if (_avt_STATUS)
    return _avt_STATUS;

  // prepare the buffer
  inbuf = (char *) &ws;
  inbytesleft = (wcslen (ws) + 1) * sizeof (wchar_t);
  outbytesleft = size;

  // do the conversion
  iconv (input_cd, &inbuf, &inbytesleft, &s, &outbytesleft);

  return _avt_STATUS;
}

extern int
avt_set_avatar_name_mb (const char *name)
{
  wchar_t *wcname;

  if (not name or not * name)
    avt_set_avatar_name (NULL);
  else
    {
      avt_mb_decode (&wcname, name, strlen (name) + 1);

      if (wcname)
	{
	  avt_set_avatar_name (wcname);
	  free (wcname);
	}
    }

  return _avt_STATUS;
}

extern int
avt_credits_mb (const char *txt, bool centered)
{
  wchar_t *wctext;

  if (_avt_STATUS == AVT_NORMAL and txt and * txt and avt_initialized ())
    {
      avt_mb_decode (&wctext, txt, strlen (txt) + 1);

      if (wctext)
	{
	  avt_credits (wctext, centered);
	  free (wctext);
	}
    }

  return _avt_STATUS;
}

extern void
avt_mb_close (void)
{
  // close conversion descriptors
  if (output_cd != ICONV_UNINITIALIZED)
    iconv_close (output_cd);

  if (input_cd != ICONV_UNINITIALIZED)
    iconv_close (input_cd);

  output_cd = input_cd = ICONV_UNINITIALIZED;

  memset (avt_encoding, 0, sizeof (avt_encoding));
}
