/*
program to convert the file bdf files into C code
Attention: This is only for fixed width fonts!

Author: Andreas K. Foerster <info@akfoerster.de>
This file is in the public domain
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MaxCode         0x10ffff
#define DefaultChar     0

typedef int boolean;
#define true  1
#define false 0

static unsigned int FontHeight, FontWidth;
static FILE *inp;
static unsigned int MaxUsedCode;
static boolean fontstarted;	/* started definition of the font */
static int charoffset;		/* Offset of current char in the font-table */
static unsigned transtable[MaxCode + 1];

static void
help (void)
{
  printf (" Usage: bdfread 9x18.bdf\n\n");
  printf ("program to convert the BDF files into C code\n");
  printf ("Attention: this is only for fixed width fonts.\n");
  exit (EXIT_SUCCESS);
}


static void
error (char *l)
{
  fprintf (stderr, "%s\n", l);
  exit (EXIT_FAILURE);
}

static void
initializetranstable (void)
{
  int i;

  for (i = 0; i <= MaxCode; i++)
    transtable[i] = 0;
}

static void
strip_nl (char *s)
{
  char *p;

  p = strchr(s, '\n');
  if (p)
    *p = '\0';
}

static void
writetranstable (void)
{
  unsigned int i;
  unsigned int blockstart = 0;
  unsigned int FORLIM;

  putchar ('\n');
  if (FontWidth <= 8)
    printf ("const unsigned char *\n");
  else
    printf ("const unsigned short *\n");
  printf ("get_font_char (wchar_t ch)\n");
  printf ("{\n");
  printf ("  ");

  FORLIM = MaxUsedCode;
  for (i = 0; i <= FORLIM; i++)
    {
      if (transtable[i] != DefaultChar)
	{
	  if (transtable[i] + 1 == transtable[i + 1])
	    {
	      if (blockstart == 0)
		blockstart = i;
	    }
	  else
	    {
	      if (blockstart == 0)
		printf ("if (ch == %u) return &font[%u];\n",
			i, transtable[i] * FontHeight);
	      else
		printf
		  ("if (ch <= %u && ch >= %u) return &font[(ch - %u) * %u];\n",
		   i, blockstart, blockstart - transtable[blockstart],
		   FontHeight);
	      blockstart = 0;
	      printf ("  else ");
	    }
	}
    }

  printf ("return &font[DEFAULT_CHAR];\n");
  printf ("}\n");
}


static void
processchar (char *charname)
{
  char zl[256];
  unsigned int codepoint;

  if (fontstarted)
    printf (",\n");
  else
    {
      putchar ('\n');
      if (FontWidth <= 8)
	printf ("static const unsigned char font[] = {\n");
      else
	printf ("static const unsigned short font[] = {\n");
      fontstarted = true;
    }

  strip_nl(charname);
  printf ("/* %s */\n", charname);

  while (fgets (zl, sizeof (zl), inp) && strncmp (zl, "BITMAP", 6) != 0)
    {
      if (sscanf (zl, "ENCODING %u", &codepoint) == 1)
	{
	  transtable[codepoint] = charoffset;
	  if (codepoint > MaxUsedCode)
	    MaxUsedCode = codepoint;
	}
    }

  fgets (zl, sizeof (zl), inp);
  strip_nl(zl);
  printf ("0x%s", zl);
  fgets (zl, sizeof (zl), inp);
  while (strncmp (zl, "ENDCHAR", 7) != 0)
    {
      strip_nl(zl);
      printf (",0x%s", zl);
      fgets (zl, sizeof (zl), inp);
    }
  charoffset++;
}


static void
GetFontSize (char *s)
{
  if (sscanf (s, "FONTBOUNDINGBOX %u %u", &FontWidth, &FontHeight) < 2)
    error ("FONTBOUNDINGBOX: error in input data");
}


static void
processdata (void)
{
  char zl[256], defchar[256];
  boolean initialized;

  *defchar = '\0';
  initialized = false;
  fgets (zl, 256, inp);
  if (strncmp (zl, "STARTFONT ", 10) != 0)
    error ("input is not in bdf format");

  while (!initialized && fgets (zl, sizeof (zl), inp) != NULL)
    {
      strip_nl (zl);

      if (strncmp (zl, "FONTBOUNDINGBOX ", 16) == 0)
	GetFontSize (zl);

      if (strncmp (zl, "FONT ", 5) == 0)
	printf ("/* %s */\n", zl);

      if (strncmp (zl, "COPYRIGHT ", 10) == 0)
	printf ("/* %s */\n", zl);

      if (strncmp (zl, "CHARS ", 6) == 0)
	printf ("/* %s */\n", zl);

      if (strncmp (zl, "DEFAULT_CHAR ", 13) == 0)
	strcpy (defchar, zl);

      if (strncmp (zl, "STARTCHAR ", 10) == 0)
	{
	  if (FontWidth == 0 || FontHeight == 0)
	    error ("error in input data");
	  printf ("\n#include <stddef.h>\n\n");
	  if (*defchar != '\0')
	    printf ("#define %s\n", defchar);
	  initialized = true;
	  processchar (&zl[10]);
	}
    }

  while (fgets (zl, sizeof (zl), inp) != NULL)
    {
      if (strncmp (zl, "STARTCHAR ", 10) == 0)
	processchar (&zl[10]);
    }

  printf ("};\n\n");
}

int
main (int argc, char *argv[])
{
  inp = NULL;
  fontstarted = false;
  charoffset = 0;
  MaxUsedCode = 0;
  FontHeight = 0;
  FontWidth = 0;
  initializetranstable ();

  if (argc != 2 || argv[1][0] == '-')
    help ();

  inp = fopen (argv[1], "r");
  if (inp == NULL)
    error ("cannot open file");

  printf ("/* derived from the font %s */\n", argv[1]);
  printf
    ("/* fetched from http://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html */\n");

  processdata ();
  writetranstable ();

  fclose (inp);

  return EXIT_SUCCESS;
}
