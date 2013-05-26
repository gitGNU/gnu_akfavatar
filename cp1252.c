#include "avtaddons.h"

static const struct avt_char_map map = {
  .start = 128,.end = 159,
  .table = {
	    0x20AC,		// Euro
	    INVALID_WCHAR,
	    0x201A,		// single low 9 quotation mark
	    0x0192,		// function symbol
	    0x201E,		// double low 9 quotation mark
	    0x2026,		// ellipsis
	    0x2020,		// dagger
	    0x2021,		// double dagger
	    0x02C6,		// circumflex
	    0x2030,		// promille
	    0x0160,		// S caron
	    0x2039,		// single left-pointing angle quotation mark
	    0x0152,		// Oe ligature
	    INVALID_WCHAR,
	    0x017D,		// Z caron
	    INVALID_WCHAR,
	    INVALID_WCHAR,
	    0x2018,		// left single quotation mark
	    0x2019,		// right single quotation mark
	    0x201C,		// left double quotation mark
	    0x201D,		// right double quotation mark
	    0x2022,		// bullet
	    0x2013,		// en dash
	    0x2014,		// em dash
	    0x02DC,		// small tilde
	    0x2122,		// trade mark
	    0x0161,		// s caron
	    0x203A,		// singe right-pointing angle quotation mark
	    0x0153,		// oe ligature
	    INVALID_WCHAR,
	    0x017E,		// z caron (?)
	    0x0178		// Y diaresis
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &map,
  .to_unicode = map_to_unicode,
  .from_unicode = map_from_unicode
};


extern const struct avt_charenc *
avt_cp1252 (void)
{
  return &converter;
}
