{Language=Turbo} {Integer16=0} {AnsiC=1} {MainType=int}

{
program to convert the file bdf files into C code
Attention: This is only for fixed width fonts!

Author: Andreas K. Foerster <info@akfoerster.de>
This file is in the public domain

Compilable wit gpc, fpc and p2c
}

program bdfread;

const
  MaxCode = $1FFFFF;

const 
  DefaultChar = 0;

var 
  FontHeight, FontWidth: Cardinal;
  inp: text;
  MaxUsedCode: LongInt;
  fontstarted: boolean; { started definition of the font }
  charoffset: LongInt; { Offset of current char in the font-table }
  transtable : array [ 0 .. MaxCode ] of Cardinal;

procedure help;
begin
WriteLn(' Usage: bdfread 9x18.bdf');
WriteLn;
WriteLn('program to convert the BDF files into C code');
WriteLn('Attention: this is only for fixed width fonts.');
Halt
end;

procedure error (l: string);
begin
WriteLn (stderr, l);
halt(1);
end;

procedure initializetranstable;
var i: LongInt;
begin
for i := 0 to MaxCode do
  transtable [i] := 0
end;

procedure writetranstable;
var i, blockstart: Cardinal;
begin
WriteLn;
if FontWidth <= 8
  then WriteLn('const unsigned char *')
  else WriteLn('const unsigned short *');
WriteLn('get_font_char (wchar_t ch)');
WriteLn('{');
Write('  ');

blockstart := 0;
for i := 0 to MaxUsedCode do
  if transtable [i] <> DefaultChar then
    begin
    if transtable [i] + 1 = transtable[i+1]
      then begin
           if blockstart = 0 then blockstart := i;
	   end
      else begin
           if blockstart = 0
	     then WriteLn ('if (ch == ', i, ') return &font[', 
	                   transtable[i] * FontHeight, '];')
             else WriteLn ('if (ch >= ',blockstart, ' && ch <= ', i, 
	                   ') return &font[(ch - ', 
			   blockstart - transtable[blockstart], 
	                   ') * ', FontHeight, '];');
	   blockstart := 0;
	   Write('  else ')
	   end
    end;

WriteLn('return &font[DEFAULT_CHAR];');
WriteLn('}')
end;


procedure processchar(var charname: string);
var
  zl: string;
  codepoint: LongInt;
  code: word;
begin
if fontstarted 
  then WriteLn(',')
  else
    begin
    WriteLn;
    if FontWidth <= 8
      then WriteLn('const unsigned char font[] = {')
      else WriteLn('const unsigned short font[] = {');
    fontstarted := true;
    end;

WriteLn ('/* ', charname, ' */');

repeat
  ReadLn(inp, zl);
  if pos('ENCODING ', zl)=1 then
    begin
    Delete(zl, 1, Length('ENCODING '));
    val(zl, codepoint, code);
    if code <> 0 then error('error in input data');
    transtable[codepoint] := charoffset;
    if codepoint > MaxUsedCode then 
      MaxUsedCode := codepoint
    end;
until (zl = 'BITMAP') or EOF(inp);

ReadLn(inp, zl);
Write('0x', zl);
ReadLn(inp, zl);
while zl <> 'ENDCHAR' do
  begin
  Write(',0x', zl);
  ReadLn(inp, zl)
  end;
inc(charoffset)
end;

procedure GetFontSize(var s: string);
var tmp: string;
var err, p: integer;
begin
tmp := s;
Delete(tmp, 1, Length('FONTBOUNDINGBOX '));
p := pos(' ', tmp);
tmp := copy(tmp, 1, p-1);
val(tmp, FontWidth, err);
tmp := s;
Delete(tmp, 1, Length('FONTBOUNDINGBOX '));
Delete (tmp, 1, pos(' ', tmp));
p := pos(' ', tmp);
tmp := copy(tmp, 1, p-1);
val(tmp, FontHeight, err);
end;

procedure processdata;
var zl, defchar : string;
var initialized : boolean;
begin
defchar := '';
initialized := false;
ReadLn(inp, zl);
if pos('STARTFONT ', zl)<>1 then
  error ('input is not in bdf format');

While (not EOF(inp)) and (not initialized) do
  begin
    ReadLn(inp, zl);

    if pos('FONTBOUNDINGBOX ', zl) = 1 then
      GetFontSize(zl);

    if pos('FONT ', zl) = 1 then
      WriteLn('/* ', zl, ' */');

    if pos('COPYRIGHT ', zl) = 1 then
      WriteLn('/* ', zl, ' */');

    if pos('CHARS ', zl) = 1 then
      WriteLn('/* ', zl, ' */');

    if pos('DEFAULT_CHAR ', zl) = 1 then
      defchar := zl;

    if pos('STARTCHAR ', zl) = 1 then
      begin
      if (fontWidth=0) or (FontHeight=0) then error('error in input data');
      WriteLn;
      WriteLn('#include <stddef.h>');
      WriteLn;
      if defchar <>'' then WriteLn ('#define ', defchar);
      initialized := true;
      delete (zl, 1, Length('STARTCHAR '));
      processchar(zl)
      end
  end; { while not EOF(inp) }

While not EOF(inp) do
  begin
  ReadLn(inp, zl);
  if pos('STARTCHAR ', zl) = 1 then
    begin
    delete (zl, 1, Length('STARTCHAR '));
    processchar(zl)
    end
  end; { while not EOF(inp) }

WriteLn('};');
WriteLn;
end;

begin
fontstarted := false;
charoffset := 0;
MaxUsedCode := 0;
FontHeight := 0;
FontWidth := 0;
initializetranstable;

if ParamCount<>1 then help;
if ParamStr(1)[1] = '-' then help;

assign(inp, ParamStr(1));
Reset(inp);

WriteLn('/* derived from the font ', ParamStr(1), ' */');
WriteLn('/* fetched from http://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html */');

processdata;
writetranstable;
close(inp)
end.
