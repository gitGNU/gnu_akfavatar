/*
 * get language code for Windows 2000 or newer
 * Copyright (c) 2012 Andreas K. Foerster <info@akfoerster.de>
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

#include <windows.h>

// returns 2-letter language code according ISO 639-1
// or NULL if unknown
extern char *
avta_get_language (void)
{
  int nr;
  char *lang;

  // lower byte is the main language
  // but we need all
  nr = GetUserDefaultLangID ();

  // transfer to language code according ISO 639-1
  switch (nr & 0xFF)
    {
    case 0x36:
      lang = "af";		// Afrikaans
      break;

    case 0x5E:
      lang = "am";		// Amharic
      break;

    case 0x01:
      lang = "ar";		// Arabic
      break;

    case 0x2B:
      lang = "hy";		// Armenian
      break;

    case 0x4D:
      lang = "as";		// Assamese
      break;

    case 0x2C:
      lang = "az";		// Azerbaijani
      break;

    case 0x6D:
      lang = "ba";		// Bashkir
      break;

    case 0x2D:
      lang = "eu";		// Basque
      break;

    case 0x23:
      lang = "be";		// Belarusian
      break;

    case 0x45:
      lang = "bn";		// Bengali
      break;

    case 0x1A:
      if (nr == 0x781A || nr == 0x201A || nr == 0x141A)
	lang = "bs";		// Bosnian
      else if (nr == 0x001A || nr == 0x101A || nr == 0x041A)
	lang = "hr";		// Croatian
      else if (nr == 0x1C1A || nr == 0x181A || nr == 0x0C1A || nr == 0x081A)
	lang = "sr";		// Serbian
      else
	lang = NULL;
      break;

    case 0x7E:
      lang = "br";		// Breton
      break;

    case 0x02:
      lang = "bg";		// Bulgarian
      break;

    case 0x92:
      lang = "ku";		// Kurdish
      break;

    case 0x03:
      lang = "ca";		// Catalan
      break;

    case 0x04:
      lang = "zh";		// Chinese
      break;

    case 0x83:
      lang = "co";		// Corsican
      break;

    case 0x05:
      lang = "cs";		// Czech
      break;

    case 0x06:
      lang = "da";		// Danish
      break;

    case 0x65:
      lang = "dv";		// Divehi
      break;

    case 0x13:
      lang = "nl";		// Dutch
      break;

    case 0x09:
      lang = "en";		// English
      break;

    case 0x25:
      lang = "et";		// Estonian
      break;

    case 0x38:
      lang = "fo";		// Faroese
      break;

    case 0x0B:
      lang = "fi";		// Finnish
      break;

    case 0x0C:
      lang = "fr";		// French
      break;

    case 0x62:
      lang = "fy";		// Western Frisian
      break;

    case 0x56:
      lang = "gl";		// Galician
      break;

    case 0x37:
      lang = "ka";		// Georgian
      break;

    case 0x07:
      lang = "de";		// German (Deutsch)
      break;

    case 0x08:
      lang = "gr";		// Greek
      break;

    case 0x6F:
      lang = "kl";		// Greenlandic (Kalaallisut)
      break;

    case 0x47:
      lang = "gu";		// Gujarati
      break;

    case 0x68:
      lang = "ha";		// Hausa
      break;

    case 0x0D:
      lang = "he";		// Hebrew
      break;

    case 0x39:
      lang = "hi";		// Hindi
      break;

    case 0x0E:
      lang = "hu";		// Hungarian
      break;

    case 0x0F:
      lang = "is";		// Icelandic
      break;

    case 0x70:
      lang = "ig";		// Igbo
      break;

    case 0x21:
      lang = "id";		// Indonesian
      break;

    case 0x5D:
      lang = "iu";		// Inuktitut
      break;

    case 0x3C:
      lang = "ga";		// Irish
      break;

    case 0x34:
      lang = "xh";		// Xhosa
      break;

    case 0x35:
      lang = "zu";		// Zulu
      break;

    case 0x10:
      lang = "it";		// Italian
      break;

    case 0x11:
      lang = "ja";		// Japanese
      break;

    case 0x4B:
      lang = "kn";		// Kannada
      break;

    case 0x60:
      lang = "ks";		// Kashmiri
      break;

    case 0x3F:
      lang = "kk";		// Kazakh
      break;

    case 0x87:
      lang = "rw";		// Kinyarwanda
      break;

    case 0x12:
      lang = "ko";		// Korean
      break;

    case 0x40:
      lang = "ky";		// Kirghiz
      break;

    case 0x54:
      lang = "lo";		// Lao
      break;

    case 0x26:
      lang = "lv";		// Latvian
      break;

    case 0x27:
      lang = "lt";		// Lithuanian
      break;

    case 0x6E:
      lang = "lb";		// Luxembourgish
      break;

    case 0x2F:
      lang = "mk";		// Macedonian
      break;

    case 0x3E:
      lang = "ms";		// Malay
      break;

    case 0x4C:
      lang = "ml";		// Malayalam
      break;

    case 0x3A:
      lang = "mt";		// Maltese
      break;

    case 0x81:
      lang = "mi";		// Maori
      break;

    case 0x4E:
      lang = "mr";		// Marathi
      break;

    case 0x50:
      lang = "mn";		// Mongolian
      break;

    case 0x61:
      lang = "ne";		// Nepali
      break;

    case 0x14:
      lang = "no";		// Norwegian
      break;

    case 0x82:
      lang = "oc";		// Occitan
      break;

    case 0x48:
      lang = "or";		// Oriya
      break;

    case 0x63:
      lang = "ps";		// Pashto
      break;

    case 0x29:
      lang = "fa";		// Persian
      break;

    case 0x15:
      lang = "pl";		// Polish
      break;

    case 0x16:
      lang = "pt";		// Portuguese
      break;

    case 0x67:
      lang = "ff";		// Pular
      break;

    case 0x46:
      lang = "pa";		// Panjabi
      break;

    case 0x6B:
      lang = "qu";		// Quechua
      break;

    case 0x18:
      lang = "ro";		// Romanian
      break;

    case 0x17:
      lang = "rm";		// Romansh
      break;

    case 0x19:
      lang = "ru";		// Russian
      break;

    case 0x3B:
      lang = "se";		// Sami
      break;

    case 0x4F:
      lang = "sa";		// Sanskrit
      break;

    case 0x32:
      lang = "tn";		// Tswana
      break;

    case 0x59:
      lang = "sd";		// Sindhi
      break;

    case 0x5B:
      lang = "si";		// Sinhala
      break;

    case 0x1B:
      lang = "sk";		// Slovak
      break;

    case 0x24:
      lang = "sl";		// Slovenian
      break;

    case 0x0A:
      lang = "es";		// Spanish
      break;

    case 0x41:
      lang = "sw";		// Swahili
      break;

    case 0x1D:
      lang = "sv";		// Swedish
      break;

    case 0x28:
      lang = "tg";		// Tajik
      break;

    case 0x49:
      lang = "ta";		// Tamil
      break;

    case 0x44:
      lang = "tt";		// Tatar
      break;

    case 0x4A:
      lang = "te";		// Telugu
      break;

    case 0x1E:
      lang = "th";		// Thai
      break;

    case 0x51:
      lang = "bo";		// Tibetan
      break;

    case 0x73:
      lang = "ti";		// Tigrinya
      break;

    case 0x1F:
      lang = "tr";		// Turkish
      break;

    case 0x42:
      lang = "tk";		// Turkmen
      break;

    case 0x22:
      lang = "uk";		// Ukrainian
      break;

    case 0x20:
      lang = "ur";		// Urdu
      break;

    case 0x80:
      lang = "ug";		// Uighur
      break;

    case 0x43:
      lang = "uz";		// Uzbek
      break;

    case 0x2A:
      lang = "vi";		// Vietnamese
      break;

    case 0x52:
      lang = "cy";		// Welsh
      break;

    case 0x88:
      lang = "wo";		// Wolof
      break;

    case 0x78:
      lang = "ii";		// Sichuan Yi
      break;

    case 0x6A:
      lang = "yo";		// Yoruba
      break;

    default:
      lang = NULL;		// unknown
      break;
    }

  return lang;
}
