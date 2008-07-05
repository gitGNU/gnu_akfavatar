{
program to convert the file 7x14.bdf or 7x14B.bdf into C code
Attention: this is not a general purpose program yet
It is especially written for this single purpose!

This file is in the public domain
}

program bdfread (input, output, stderr);

const
  MaxCode = $1FFFFF;

const 
  PixelSize = 14;
  DefaultChar = 0;

var 
  inp: text;
  MaxUsedCode: LongInt;
  fontstarted: boolean; { started definition of the font }
  charoffset: LongInt; { Offset of current char in the font-table }
  transtable : array [ 0 .. MaxCode ] of Cardinal;

procedure help;
begin
WriteLn(' Usage: bdfread 7x14.bdf');
WriteLn;
WriteLn('program to convert the file 7x14.bdf or 7x14B.bdf into C code');
WriteLn('Attention: this is not a general purpose program yet');
WriteLn('It is especially written for this single purpose!');
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
var i: LongInt;
begin
WriteLn;
WriteLn('size_t');
WriteLn('get_font_offset (wchar_t ch)');
WriteLn('{');
WriteLn('  switch (ch)');
WriteLn('    {');
for i := 0 to MaxUsedCode do
  if transtable [i] <> DefaultChar then
    WriteLn('      case ', i, ': return ', transtable[i], ';');
WriteLn('      default: return DEFAULT_CHAR;');
WriteLn('    }');
WriteLn('}')
end;


procedure processchar(const charname: string);
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
    WriteLn('const unsigned char font[] = {');
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
inc(charoffset);
ReadLn(inp, zl);
while zl <> 'ENDCHAR' do
  begin
  Write(',0x', zl);
  inc(charoffset);
  ReadLn(inp, zl)
  end;
end;

procedure processdata;
var zl : string;
begin
ReadLn(inp, zl);
if pos('STARTFONT ', zl)<>1 then
  error ('input is not in bdf format');

While not EOF(inp) do
  begin
    ReadLn(inp, zl);

    if pos('FONT ', zl) = 1 then
      WriteLn('/* ', zl, ' */');

    if pos('COPYRIGHT ', zl) = 1 then
      WriteLn('/* ', zl, ' */');

    if pos('CHARS ', zl) = 1 then
      WriteLn('/* ', zl, ' */');

    if pos('DEFAULT_CHAR ', zl) = 1 then
      WriteLn('#define ', zl);

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
initializetranstable;

if ParamCount<>1 then help;
if ParamStr(1)[1] = '-' then help;

assign(inp, ParamStr(1));
Reset(inp);

WriteLn('/* derived from the font ', ParamStr(1), ' */');
WriteLn('/* fetched from http://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html */');
WriteLn;
WriteLn('#include <stddef.h>');
WriteLn;
WriteLn('#define FONTWIDTH 7');
WriteLn('#define FONTHEIGHT 14');
WriteLn('#define LINEHEIGHT 17 /* some space between lines */');
WriteLn;

processdata;
writetranstable;
close(inp)
end.
