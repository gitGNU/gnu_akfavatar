{*
 * Pascal binding to the AKFAvatar library
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
 *
 * Can be used with GNU-Pascal or FreePascal
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
 *}

{ 
CRT compatiblity

supported: 
ClrScr, ClrEol, GotoXY, WhereX, WhereY, Delay, TextColor, TextBackground,
NormVideo, HighVideo, LowVideo, TextAttr, NoSound, (ReadKey), KeyPressed, 
AssignCrt, ScreenSize, CheckBreak

not supported yet, but planned: 
DelLine, InsLine, Window

dummies for:
CheckEof, CheckSnow, DirectVideo, Sound

no support planned for:
- TextMode, LastMode:
    remove that code, or use $IfDef MSDOS
- writing to WindMin, WindMax:
    use Window, when it is supported
}

{$IfDef FPC}
  {$LongStrings on}
{$EndIf}

{$X+}

unit avatar;

interface

{ length of an output line }
{ input is one less }
const LineLength = 80;

{ Default encoding of the system }
{$IfDef LATIN1}
  const DefaultEncoding = 'ISO-8859-1';
{$Else}
  const DefaultEncoding = 'UTF-8';
{$EndIf}

{ Colors for TextColor/TextBackground }
{ compatible to the CRT unit }
const
  Black        = 0;
  Blue         = 1;
  Green        = 2;
  Cyan         = 3;
  Red          = 4;
  Magenta      = 5;
  Brown        = 6;
  LightGray    = 7;
  DarkGray     = 8;
  LightBlue    = 9;
  LightGreen   = 10;
  LightCyan    = 11;
  LightRed     = 12;
  LightMagenta = 13;
  Yellow       = 14;
  White        = 15;
  Blink        = 128; { ignored }

{$IfDef FPC}
  type LineString = AnsiString;
{$Else}
  { In UTF-8 encoding one char may take up to 4 Bytes }
  type LineString = string (4 * LineLength);
  type ShortString = string (255);
{$EndIf}

type TextDirection = (LeftToRight, RightToLeft);

{ 
  Text Attributes
  mostly compatible to the CRT unit
  the "blink-bit" means a bright background color
}
var TextAttr : byte;

{ compatible to the CRT unit }
var 
  CheckBreak: boolean;
  CheckEof: boolean;
  CheckSnow: boolean;
  DirectVideo: boolean;

{ The "Screen" is the textarea }
{ The name is chosen for compatiblity with the CRT unit }
{ Just for reading! }
{ This variable is only set after the avatar is visible }
var ScreenSize : record x, y: Integer end;

{ for CRT compatiblity, use ScreenSize for new programs }
{ Just for reading! }
{ These variables are only set after the avatar is visible }
var WindMin, WindMax: word;


{ load the Avatar image from a file }
{ must be used before any output took place }
procedure AvatarImageFile(FileName: string);

{ set a different background color (default is grey) }
{ must be used before any output took place }
procedure setBackgroundColor (red, green, blue: byte);

{ change the encoding }
procedure setEncoding(const newEncoding: string);

{ change text direction (for hebrew/yiddish texts) }
{ you should start a new line before or after this command }
procedure setTextDirection (direction: TextDirection);

{ assign text-variable to the avatar }
procedure AssignAvatar (var f: text);

{ the same for CRT compatiblity }
procedure AssignCrt (var f: text);

{ Restore Input/Output system }
{ use this to output help or version information }
procedure RestoreInOut;

{$IfDef FPC}
  { the page command is defined in the Pascal standard,
    but missing in FreePascal }

  { action: wait a while and then clear the textfield }

  procedure page (var f: text);
  procedure page;
{$EndIf}

{ keyboard handling }
{ partly CRT compatible - Latin1 chars so far }
function KeyPressed: boolean;
function ReadKey: char;

{ wait for a key }
procedure waitkey (const message: string);

{ wait some time }
{ compatible to CRT unit }
procedure delay (milliseconds: Integer);

{ example use: delay (seconds (2.5)); }
function seconds (s: Real): Integer;

{ clears the textfield (not the screen!) }
{ the name was chosen for compatiblity to the CRT unit }
procedure ClrScr;

{ clears rest of the line }
{ compatible to CRT unit }
procedure ClrEol;

{ set the text color }
{ compatible to CRT unit }
procedure TextColor (Color: Byte);

{ set the text background color }
{ compatible to CRT unit, but light colors can be used }
procedure TextBackground (Color: Byte);

{ set black on white text colors }
{ name compatible to CRT unit, but the colors differ }
procedure NormVideo;

{ set high color intensity }
procedure HighVideo;

{ set low color intensity }
procedure LowVideo;

{ shows the avatar without the balloon }
procedure ShowAvatar;

{ moves the avatar in or out }
procedure MoveAvatarIn;
procedure MoveAvatarOut;

{ loads image
  after that call delay or waitkey 
  the supported image formats depend on your libraries
  uncompressed BMP is always supported
}
function ShowImageFile(FileName: string): boolean;

{ plays Audio File
  currently only WAV files supported
  encodings: PCM, MS-ADPCM, IMA-ADPCM }
procedure PlaySoundFile(const FileName: string);

{ wait until the end of the audio output }
procedure WaitSoundEnd;

{ dummy function, full support planned }
procedure Sound(frequency: Integer);

{ stop sound output }
procedure NoSound;

{ handle coordinates (inside the balloon) }
{ compatible to CRT unit }
function WhereX: Integer;
function WhereY: Integer;
procedure GotoXY (x, y: Integer);

{ get last error message }
function AvatarGetError: ShortString;

{ ignore TextColor TextBackground and so on }
{ compatible with GNU-Pascal's CRT unit }
procedure SetMonochrome(monochrome: boolean);

{-----------------------------------------------------------------------}

implementation

{$IfDef FPC}
  uses DOS;
  {$IfNDef NoLink}
    {$LinkLib akfavatar}
    {$LinkLib SDL}
    {$IfDef Linux} {$LinkLib pthread} {$EndIf}
  {$EndIf}
{$EndIf}

{$IfDef __GPC__} 
  uses GPC;

  {$IfNDef NoLink}
    {$L akfavatar}
    {$L SDL}
    {$IfDef __linux__} {$L pthread} {$EndIf}
  {$EndIf}

  {$Define cdecl attribute(cdecl)} 
{$EndIf}


{$IfDef FPC}
  type 
    CInteger = LongInt;
    CString = PChar;
{$EndIf}

{$IfDef __GPC__}
  {$if __GPC_RELEASE__ < 20041218}
    type CInteger = Integer;
  {$EndIf}
{$EndIf}


type PAvatarImage = Pointer;

type 
  PGimpImage = ^TGimpImage;
  TGimpImage = 
    record
    width, height, bytes_per_pixel : CInteger;
    pixel_data : char; { startpoint }
    end;

var OldTextAttr : byte;
var FastQuit : boolean;
var isMonochrome : boolean;
var fullscreen, initialized, audioinitialized: boolean;
var AvatarImage: PAvatarImage;
var InputBuffer: array [ 0 .. (4 * LineLength) + 2] of char;

const KeyboardBufferSize = 40;
var KeyboardBuffer: array [ 0 .. KeyboardBufferSize-1 ] of char;
var KeyboardBufferRead, KeyboardBufferWrite: integer;

function avt_default: PAvatarImage; cdecl; external name 'avt_default';

function avt_get_status: CInteger; 
  cdecl; external name 'avt_get_status';

function avt_say_mb(t: CString): CInteger; 
  cdecl; external name 'avt_say_mb';

procedure avt_clear; 
  cdecl; external name 'avt_clear';

procedure avt_clear_eol;
  cdecl; external name 'avt_clear_eol';

function avt_mb_encoding (encoding: CString): CInteger;
  cdecl; external name 'avt_mb_encoding';

function avt_ask_mb(t: Pointer; size: CInteger): CInteger; 
  cdecl; external name 'avt_ask_mb';

function avt_wait(milliseconds: CInteger): CInteger; 
  cdecl; external name 'avt_wait';

function avt_wait_key_mb(message : CString): CInteger; 
  cdecl; external name 'avt_wait_key_mb';

function avt_move_in: CInteger; 
  cdecl; external name 'avt_move_in';

function avt_move_out: CInteger; 
  cdecl; external name 'avt_move_out';

procedure avt_show_avatar;
  cdecl; external name 'avt_show_avatar';

function avt_import_gimp_image(gimp_image: PGimpImage): PAvatarImage;
   cdecl; external name 'avt_import_gimp_image';

function avt_import_image_file (FileName: CString): PAvatarImage;
  cdecl; external name 'avt_import_image_file';

function avt_show_image_file(FileName: CString): CInteger;
  cdecl; external name 'avt_show_image_file';

procedure avt_set_background_color (red, green, blue: CInteger);
  cdecl; external name 'avt_set_background_color';

procedure avt_set_text_color (red, green, blue: CInteger);
  cdecl; external name 'avt_set_text_color';

procedure avt_set_text_background_color (red, green, blue: CInteger);
  cdecl; external name 'avt_set_text_background_color';

function initialize(title, icon: CString;
                     image: PAvatarImage;
                     mode: CInteger): CInteger;
  cdecl; external name 'avt_initialize';

function avt_initialize_audio: CInteger; 
  cdecl; external name 'avt_initialize_audio';

procedure avt_quit; cdecl; external name 'avt_quit';

procedure avt_quit_audio; cdecl; external name 'avt_quit_audio';

function avt_load_wave_file(f: CString): CInteger;
  cdecl; external name 'avt_load_wave_file';

procedure avt_free_wave; cdecl; external name 'avt_free_wave';

function avt_play_audio: CInteger; 
  cdecl; external name 'avt_play_audio';

function avt_wait_audio_end: CInteger;
  cdecl; external name 'avt_wait_audio_end';

procedure avt_stop_audio; cdecl; external name 'avt_stop_audio';

function avt_get_error: CString; cdecl; external name 'avt_get_error';

function avt_where_x: CInteger; cdecl; external name 'avt_where_x';
function avt_where_y: CInteger; cdecl; external name 'avt_where_y';
procedure avt_move_x (x: CInteger); cdecl; external name 'avt_move_x';
procedure avt_move_y (x: CInteger); cdecl; external name 'avt_move_y';
function avt_get_max_x: CInteger; cdecl; external name 'avt_get_max_x'; 
function avt_get_max_y: CInteger; cdecl; external name 'avt_get_max_y'; 

procedure avt_text_direction (direction: CInteger); 
  cdecl; external name 'avt_text_direction';
  
procedure avt_register_keyhandler (handler: Pointer);
  cdecl; external name 'avt_register_keyhandler';

{$IfNDef __GPC__}

  function String2CString(s: string): CString;
  begin
  String2CString := CString(s)
  end;

  function CString2String(s: CString): string;
  begin
  CString2String := strpas(s)
  end;

{$EndIf}

procedure setBackgroundColor (red, green, blue: byte);
begin
avt_set_background_color(red, green, blue)
end;

procedure setEncoding(const newEncoding: string);
begin
avt_mb_encoding(String2CString(newEncoding))
end;

procedure setTextDirection (direction: TextDirection);
begin
avt_text_direction (ord (direction))
end;

procedure AvatarImageFile(FileName: string);
begin
{ when it is already initialized, it's too late }
if AvatarImage = NIL then
  AvatarImage := avt_import_image_file (String2CString(FileName))
end;

procedure RestoreInOut;
begin
{$I-}
Close (input);
Close (output);
{$I+}

InOutRes := 0;

Assign (input, '');
Assign (output, '');
Reset (input);
Rewrite (output)
end;

procedure Quit;
begin
RestoreInOut;

{ the order is important! }
if audioinitialized then avt_quit_audio;
if initialized then avt_quit
end;

procedure initializeAvatar;
begin
if AvatarImage = NIL then AvatarImage := avt_default;

if initialize('AKFAvatar', 'AKFAvatar', AvatarImage,
              ord(fullscreen)) < 0 
  then 
    begin
    WriteLn(stderr, 'cannot initialize graphics: ', AvatarGetError);
    Halt(1)
    end;

if avt_get_status = 1 then Halt; { shouldn't happen here yet }

initialized := true;
audioinitialized := false;
ScreenSize.x := avt_get_max_x;
ScreenSize.y := avt_get_max_y;

{ set WindMin und WindMax }
WindMin := $0000;

if ScreenSize.y-1 >= $FF
  then WindMax := $FF shl 8
  else WindMax := (ScreenSize.y-1) shl 8;

if ScreenSize.x-1 >= $FF
  then WindMax := WindMax or $FF
  else WindMax := WindMax or (ScreenSize.x-1);

NormVideo;
avt_move_in
end;

procedure TextColor (Color: Byte);
begin
if not initialized then initializeAvatar;
if isMonochrome then exit;

{ strip blink attribute }
Color := Color and $0F;

TextAttr := (TextAttr and $F0) or Color;
OldTextAttr := TextAttr;

case Color of
  Black        : avt_set_text_color ($00, $00, $00);
  Blue         : avt_set_text_color ($00, $00, $88);
  Green        : avt_set_text_color ($00, $88, $00);
  Cyan         : avt_set_text_color ($00, $88, $88);
  Red          : avt_set_text_color ($88, $00, $00);
  Magenta      : avt_set_text_color ($88, $00, $88);
  Brown        : avt_set_text_color ($88, $44, $22);
  LightGray    : avt_set_text_color ($88, $88, $88);
  DarkGray     : avt_set_text_color ($55, $55, $55);
  LightBlue    : avt_set_text_color ($00, $00, $FF);
  LightGreen   : avt_set_text_color ($00, $FF, $00);
  LightCyan    : avt_set_text_color ($00, $FF, $FF);
  LightRed     : avt_set_text_color ($FF, $00, $00); 
  LightMagenta : avt_set_text_color ($FF, $00, $FF);
  Yellow       : avt_set_text_color ($E0, $E0, $00);
  White        : avt_set_text_color ($FF, $FF, $FF)
  end
end;

procedure TextBackground (Color: Byte);
begin
if not initialized then initializeAvatar;
if isMonochrome then exit;

{ strip what we don't need }
Color := Color and $0F;

TextAttr := (TextAttr and $0F) or (Color shl 4);
OldTextAttr := TextAttr;

case Color of
  Black        : avt_set_text_background_color ($00, $00, $00);
  Blue         : avt_set_text_background_color ($00, $00, $88);
  Green        : avt_set_text_background_color ($00, $88, $00);
  Cyan         : avt_set_text_background_color ($00, $88, $88);
  Red          : avt_set_text_background_color ($88, $00, $00);
  Magenta      : avt_set_text_background_color ($88, $00, $88);
  Brown        : avt_set_text_background_color ($88, $44, $22);
  LightGray    : avt_set_text_background_color ($88, $88, $88);
  DarkGray     : avt_set_text_background_color ($55, $55, $55);
  LightBlue    : avt_set_text_background_color ($00, $00, $FF);
  LightGreen   : avt_set_text_background_color ($00, $FF, $00);
  LightCyan    : avt_set_text_background_color ($00, $FF, $FF);
  LightRed     : avt_set_text_background_color ($FF, $00, $00); 
  LightMagenta : avt_set_text_background_color ($FF, $00, $FF);
  Yellow       : avt_set_text_background_color ($FF, $FF, $00);
  White        : avt_set_text_background_color ($FF, $FF, $FF)
  end
end;

procedure UpdateTextAttr;
begin
TextBackground(TextAttr shr 4);
TextColor(TextAttr and $0F);

{$IfDef Debug}
  if TextAttr<>OldTextAttr then RunError;
{$EndIf}
end;

procedure NormVideo;
begin
if not initialized then initializeAvatar;
TextAttr := $F0;
OldTextAttr := TextAttr;
avt_set_text_color ($00, $00, $00);
avt_set_text_background_color ($FF, $FF, $FF)
end;

procedure HighVideo;
begin
{ set highcolor bit }
TextColor ((TextAttr and $0F) or $08)
end;

procedure LowVideo;
begin
{ unset highcolor bit }
TextColor (TextAttr and $07)
end;

procedure SetMonochrome (monochrome: Boolean);
begin
isMonochrome := monochrome
end;

procedure delay (milliseconds: Integer);
begin
if not initialized then initializeAvatar;
if avt_wait (milliseconds) <> 0 then Halt
end;

procedure MoveAvatarIn;
begin
if not initialized then initializeAvatar;
if avt_move_in <> 0 then Halt
end;

procedure MoveAvatarOut;
begin
if not initialized then initializeAvatar;
if avt_move_out <> 0 then Halt
end;

procedure ShowAvatar;
begin
if not initialized then initializeAvatar;
avt_show_avatar
end;

procedure ClrScr;
begin
if not initialized then initializeAvatar;
if TextAttr<>OldTextAttr then UpdateTextAttr;
avt_clear
end;

procedure ClrEol;
begin
if not initialized then initializeAvatar;
if TextAttr<>OldTextAttr then UpdateTextAttr;
avt_clear_eol
end;

function ShowImageFile (FileName: string): boolean;
var result : CInteger;
begin
if not initialized then initializeAvatar;
result := avt_show_image_file (String2CString(FileName));

if result = 1 then Halt; { halt requested }
if result = 0 
  then ShowImageFile := true { success }
  else ShowImageFile := false { failure }
end;

function seconds(s: Real): Integer;
begin seconds := trunc (s * 1000) end;

function AvatarGetError: ShortString;
begin
AvatarGetError := CString2String (avt_get_error)
end;

function WhereX: Integer;
begin
WhereX := avt_where_x
end;

function WhereY: Integer;
begin
WhereY := avt_where_y
end;

Procedure GotoXY (x, y: Integer);
begin
avt_move_x (x);
avt_move_y (y)
end;

procedure waitkey(const message: string);
begin
if not initialized then initializeAvatar;
if avt_wait_key_mb(String2CString(message))<>0 then Halt
end;

procedure checkParameters;
var i: Integer;
begin
for i := 1 to ParamCount do
  if (ParamStr(i)='--fullscreen') or (ParamStr(i)='-f')
    then fullscreen := true
end;

procedure InitializeAudio;
begin
if avt_initialize_audio<>0 then Halt;
audioinitialized := true
end;

procedure PlaySoundFile(const FileName: string);
var status: Integer;
begin
if not audioinitialized then InitializeAudio;
status := avt_load_wave_file(String2CString(FileName));
if status = 1 then Halt;
if status = 0 then 
  if avt_play_audio<>0 then Halt
end;

procedure WaitSoundEnd;
begin
if avt_wait_audio_end<>0 then Halt
end;

{ dummy function, full support planned }
procedure Sound(frequency: Integer);
begin end;

procedure NoSound;
begin
avt_stop_audio
end;

{$IfDef FPC}

  procedure page (var f: text);
  begin
  Write (f, chr (12))
  end;
  
  procedure page;
  begin
  Write (output, chr (12))
  end;

{$EndIf} { FPC }

procedure KeyHandler(sym, modifiers, unicode: CInteger); cdecl;
begin
{$IfDef Debug}
  WriteLn(stderr, 'sym: ', sym, ' modifiers: ', modifiers,
          ' unicode: ', unicode);
{$EndIf}

{ CheckBreak }
if CheckBreak and (unicode=3) then 
  begin
  FastQuit := true;
  Halt
  end;

{ put ISO-8859-1 characters into buffer }
if (unicode>0) and (unicode<=255) then
  begin
  KeyBoardBuffer[KeyboardBufferWrite] := chr(unicode);
  KeyboardBufferWrite := (KeyboardBufferWrite + 1) mod KeyboardBufferSize
  end
end;

function KeyPressed: boolean;
begin
if not initialized then initializeAvatar;

if avt_wait(1)<>0 then Halt;

KeyPressed := (KeyboardBufferRead <> KeyboardBufferWrite)
end;

function ReadKey: char;
begin
if not initialized then initializeAvatar;

{ wait for key to be pressed }
while KeyboardBufferRead=KeyboardBufferWrite do 
  if avt_wait(1)<>0 then Halt;

ReadKey := KeyboardBuffer [KeyboardBufferRead];
KeyboardBufferRead := (KeyboardBufferRead + 1) mod KeyboardBufferSize
end;

{ ---------------------------------------------------------------------}
{ Input/output handling }

procedure AssignCrt (var f: text);
begin
AssignAvatar(f)
end;

{$IfDef FPC}

  function fpc_io_dummy (var F: TextRec): Integer;
  begin
  fpc_io_dummy := 0
  end;

  function fpc_io_close (var F: TextRec): Integer;
  begin
  F.Mode := fmClosed;
  fpc_io_close := 0
  end;

  function fpc_io_write (var F: TextRec): Integer;
  var 
    s: CString;
    Status: Integer;
  begin
  if F.BufPos > 0 then
    begin
    if not initialized then initializeAvatar;

    if TextAttr<>OldTextAttr then UpdateTextAttr;

    GetMem (s, F.BufPos + 1);
    move (F.BufPtr^, s^, F.BufPos);
    s [F.BufPos {-1+1}] := #0;
    F.BufPos := 0; { everything read }
    Status := avt_say_mb (s);
    FreeMem (s);
    if Status <> 0 then Halt
    end;
  fpc_io_write := 0
  end;

  function fpc_io_read (var F: TextRec): Integer;
  begin
  if not initialized then initializeAvatar;
  if TextAttr<>OldTextAttr then UpdateTextAttr;

  if avt_ask_mb (F.BufPtr, F.BufSize) <> 0 then Halt;

  F.BufPos := 0;
  F.BufEnd := Length(F.BufPtr^) + 2;

  { sanity check }
  if F.BufEnd > F.BufSize then RunError (201);

  F.BufPtr^ [F.BufEnd-2] := #13;
  F.BufPtr^ [F.BufEnd-1] := #10;

  fpc_io_read := 0
  end;

  function fpc_io_open (var F: TextRec): Integer;
  begin
  if F.Mode = fmOutput 
    then begin
         F.InOutFunc := @fpc_io_write;
         F.FlushFunc := @fpc_io_write;
         end
    else begin
         F.Mode := fmInput;
         F.InOutFunc := @fpc_io_read;
         F.FlushFunc := @fpc_io_dummy; { sic }
	 F.BufPtr    := @InputBuffer;
	 F.BufSize   := SizeOf (InputBuffer);
         end;

  F.BufPos := F.BufEnd;
  F.CloseFunc := @fpc_io_close;
  fpc_io_open := 0
  end;

  procedure AssignAvatar (var f: text);
  begin
  Assign (f, ''); { sets sane defaults }
  TextRec(f).OpenFunc := @fpc_io_open;
  end;
{$EndIf} { FPC }

{$IfDef __GPC__}

  function gpc_io_write (var unused; const Buffer; size: SizeType): SizeType;
  var s: array [1 .. size+1] of char;
  begin
  if size > 0 then
    begin
    if not initialized then initializeAvatar;
    if TextAttr<>OldTextAttr then UpdateTextAttr;
    
    move (Buffer, s, size);
    s [size+1] := #0;
    if avt_say_mb (s) <> 0 then Halt
    end;
  
  gpc_io_write := size
  end;

  function gpc_io_read (var unused; var Buffer; size: SizeType): SizeType;
  var 
    i: SizeType;
    CharBuf: array [0 .. size-1] of char absolute Buffer;
  begin
  if not initialized then initializeAvatar;
  if TextAttr<>OldTextAttr then UpdateTextAttr;

  if avt_ask_mb (addr (InputBuffer), sizeof (InputBuffer)) <> 0 
    then Halt;
  
  i := 0;
  while (InputBuffer [i] <> chr (0)) and (i < size-1) do
    begin
    CharBuf [i] := InputBuffer [i];
    inc (i)
    end;

  CharBuf [i] := #10;
  inc (i);
  
  gpc_io_read := i
  end;

  procedure AssignAvatar (var f: text);
  begin
  AssignTFDD (f, NIL, NIL, NIL, 
                 gpc_io_read, gpc_io_write, NIL, NIL, NIL, NIL);
  end;

{$EndIf} { __GPC__ }

{ ---------------------------------------------------------------------}

Initialization

  AvatarImage := NIL;
  fullscreen := false;
  initialized := false;
  audioinitialized := false;
  isMonochrome := false;
  KeyboardBufferRead := 0;
  KeyboardBufferWrite := 0;

  { these values are not yet known }
  ScreenSize.x := -1;
  ScreenSize.y := -1;
  WindMin := $0000;
  WindMax := $0000;
 
  TextAttr := $F0;
  OldTextAttr := TextAttr;
  CheckBreak := true;
  CheckEof := false;
  CheckSnow := true;
  DirectVideo := true;
  FastQuit := false;

  checkParameters;

  avt_mb_encoding(DefaultEncoding);
  
  avt_register_keyhandler(@KeyHandler);

  { redirect i/o to Avatar }
  { do they have to be closed? Problems under Windows then }
  {Close (input);  Close (output);}
  AssignAvatar (input);
  AssignAvatar (output);
  Reset (input);
  Rewrite (output);



Finalization

  { avoid procedures which may call "Halt" again! }

  if initialized and not FastQuit then
    if avt_get_status = 0 then 
      begin
      { wait for key, when balloon is visible }
      if avt_where_x >= 0 then 
        if avt_wait_key_mb('>>>') = 0 then avt_move_out
      end;

  Quit
end.
