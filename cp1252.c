#include "avtaddons.h"

static const struct avt_char_map cp1252 = {
  .start = 128,.end = 159,
  .table = {
	    L'\u20AC',		// Euro
	    INVALID_WCHAR,
	    L'\u201A',		// single low 9 quotation mark
	    L'\u0192',		// function symbol
	    L'\u201E',		// double low 9 quotation mark
	    L'\u2026',		// ellipsis
	    L'\u2020',		// dagger
	    L'\u2021',		// double dagger
	    L'\u02C6',		// circumflex
	    L'\u2030',		// promille
	    L'\u0160',		// S caron
	    L'\u2039',		// single left-pointing angle quotation mark
	    L'\u0152',		// Oe ligature
	    INVALID_WCHAR,
	    L'\u017D',		// Z caron
	    INVALID_WCHAR,
	    INVALID_WCHAR,
	    L'\u2018',		// left single quotation mark
	    L'\u2019',		// right single quotation mark
	    L'\u201C',		// left double quotation mark
	    L'\u201D',		// right double quotation mark
	    L'\u2022',		// bullet
	    L'\u2013',		// en dash
	    L'\u2014',		// em dash
	    L'\u02DC',		// small tilde
	    L'\u2122',		// trade mark
	    L'\u0161',		// s caron
	    L'\u203A',		// singe right-pointing angle quotation mark
	    L'\u0153',		// oe ligature
	    INVALID_WCHAR,
	    L'\u017E',		// z caron (?)
	    L'\u0178'		// Y diaresis
	    }
};


static const struct avt_charenc converter = {
  .data = (void *) &cp1252,
  .to_unicode = map_to_unicode,
  .from_unicode = map_from_unicode
};


extern const struct avt_charenc *
avt_cp1252 (void)
{
  return &converter;
}
