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
Window, DelLine, InsLine, AssignCrt, ScreenSize, CheckBreak

dummies for:
CheckEof, CheckSnow, DirectVideo, Sound

no support planned for:
- TextMode, LastMode:
    remove that code, or use $IfDef MSDOS
- writing to WindMin, WindMax:
    use Window
}

{$IfDef FPC}
  {$LongStrings on}
{$EndIf}

{$X+}

unit akfavatar;

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

{ defaults for SetTextDelay and SetFlipPageDelay }
const
  DefaultTextDelay = 75;
  DefaultFlipPageDelay = 2700;

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

type TScreenSize = record x, y: integer end;

{ 
  Text Attributes
  mostly compatible to the CRT unit
  the "blink-bit" means a bright background color
}
var TextAttr : byte;

{ methods to stop the program }
var
  CheckBreak: boolean; { compatible to CRT }
  CheckEsc: boolean;

{ compatible to the CRT unit }
var 
  CheckEof: boolean;
  CheckSnow: boolean;
  DirectVideo: boolean;

{ for CRT compatiblity, use ScreenSize for new programs }
{ Just for reading! }
{ These variables are only set after the avatar is visible }
var WindMin, WindMax: word;

{ load the Avatar image from a file }
{ must be used before any output took place }
procedure AvatarImageFile(FileName: string);

{ load the Avatar image from memory }
{ must be used before any output took place }
procedure AvatarImageData(data: pointer; size: LongInt);

{ set a different background color (default is grey) }
{ must be used before any output took place }
procedure setBackgroundColor(red, green, blue: byte);

{ change pace of text and page flipping }
procedure setTextDelay(delay:integer);
procedure setFlipPageDelay(delay: integer);

{ change the encoding }
procedure setEncoding(const newEncoding: string);

{ change text direction (for hebrew/yiddish texts) }
{ you should start a new line before or after this command }
procedure setTextDirection(direction: TextDirection);

{ The "Screen" is the textarea }
{ The name is chosen for compatiblity with the CRT unit }
{ This causes the library to be initialized }
{ The avatar-image and the background color must be set before this }
function ScreenSize: TScreenSize;

{ assign text-variable to the avatar }
procedure AssignAvatar(var f: text);

{ the same for CRT compatiblity }
procedure AssignCrt(var f: text);

{ Restore Input/Output system }
{ use this to output help or version information }
procedure RestoreInOut;

{$IfDef FPC}
  { the page command is defined in the Pascal standard,
    but missing in FreePascal }

  { action: wait a while and then clear the textfield }

  procedure page(var f: text);
  procedure page;
{$EndIf}

{ keyboard handling }
{ partly CRT compatible - Latin1 chars so far }
function KeyPressed: boolean;
function ReadKey: char;

{ clear the keyboard buffer }
procedure ClearKeys;

{ wait for a key }
procedure waitkey (const message: string);

{ wait some time }
{ compatible to CRT unit }
procedure delay(milliseconds: integer);

{ example use: delay (seconds (2.5)); }
function seconds(s: Real): integer;

{ clears the window (not the screen!) }
{ the name was chosen for compatiblity to the CRT unit }
procedure ClrScr;

{ clears rest of the line }
{ compatible to CRT unit }
procedure ClrEol;

{ deletes current line, the rest is scrolled up }
procedure DelLine;

{ inserts a line before the current line, the rest is scrolled down }
procedure InsLine;

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
procedure ShowImageData(data: pointer; size: LongInt);

{ play a short sound as with chr(7) }
procedure Beep;

{ a short visual flash on the screen }
procedure Flash;

{ loads Audio File
  currently only WAV files supported
  encodings: PCM, MS-ADPCM, IMA-ADPCM }
function LoadSoundFile(const FileName: string): pointer;
function LoadSoundData(data: pointer; size: LongInt): pointer;
procedure FreeSound(snd: pointer);
procedure PlaySound(snd: pointer; loop: boolean);

{ wait until the end of the audio output }
procedure WaitSoundEnd;

{ dummy function, full support planned }
procedure Sound(frequency: integer);

{ stop sound output }
procedure NoSound;

{ handle coordinates (inside the balloon) }
{ compatible to CRT unit }
function WhereX: integer;
function WhereY: integer;
procedure GotoXY(x, y: integer);
procedure Window(x1, y1, x2, y2: Byte);

{ set/get scroll mode }
{ 0 = off (page-flipping), 1 = normal }
procedure SetScrollMode(mode: integer);
function GetScrollMode: integer;

{ get last error message }
function AvatarGetError: ShortString;

{ ignore TextColor TextBackground and so on }
{ compatible with GNU-Pascal's CRT unit }
procedure SetMonochrome(monochrome: boolean);

implementation

{-----------------------------------------------------------------------}

{$IfDef FPC}
  uses DOS;
  
  {$MACRO ON}  
  {$Define libakfavatar:=cdecl; external 'akfavatar' name}
{$EndIf}

{$IfDef __GPC__} 
  uses GPC;

  {$IfNDef NoLink}
    {$L akfavatar}
  {$EndIf}

  {$Define libakfavatar external name}
{$EndIf}


{$IfDef FPC}
  type 
    CInteger = LongInt;
    CString = PChar;
{$EndIf}

{$IfDef __GPC__}
  {$if __GPC_RELEASE__ < 20041218}
    type CInteger = integer;
  {$EndIf}
{$EndIf}


type PAvatarImage = pointer;

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
var fullscreen, initialized: boolean;
var AvatarImage: PAvatarImage;
var InputBuffer: array [ 0 .. (4 * LineLength) + 2] of char;
var ScrSize : TScreenSize;

const KeyboardBufferSize = 40;
var KeyboardBuffer: array [ 0 .. KeyboardBufferSize-1 ] of char;
var KeyboardBufferRead, KeyboardBufferWrite: integer;

procedure avt_stop_on_esc (stop: CInteger); libakfavatar 'avt_stop_on_esc';

function avt_default: PAvatarImage; libakfavatar 'avt_default';

function avt_get_status: CInteger; libakfavatar 'avt_get_status';

procedure avt_set_text_delay (delay: CInteger);
  libakfavatar 'avt_set_text_delay';

procedure avt_set_flip_page_delay (delay: CInteger);
  libakfavatar 'avt_set_flip_page_delay';

function avt_say_mb(t: CString): CInteger; libakfavatar 'avt_say_mb';

procedure avt_clear; libakfavatar 'avt_clear';

procedure avt_clear_eol; libakfavatar 'avt_clear_eol';

function avt_mb_encoding (encoding: CString): CInteger;
  libakfavatar 'avt_mb_encoding';

function avt_ask_mb(t: pointer; size: CInteger): CInteger; 
  libakfavatar 'avt_ask_mb';

function avt_wait(milliseconds: CInteger): CInteger; 
  libakfavatar 'avt_wait';

function avt_wait_key_mb(message : CString): CInteger; 
  libakfavatar 'avt_wait_key_mb';

function avt_move_in: CInteger; libakfavatar 'avt_move_in';

function avt_move_out: CInteger; libakfavatar 'avt_move_out';

procedure avt_show_avatar; libakfavatar 'avt_show_avatar';

function avt_import_gimp_image(gimp_image: PGimpImage): PAvatarImage;
   libakfavatar 'avt_import_gimp_image';

function avt_import_image_file (FileName: CString): PAvatarImage;
  libakfavatar 'avt_import_image_file';

function avt_import_image_data (Data: Pointer; size: CInteger): PAvatarImage;
  libakfavatar 'avt_import_image_data';

function avt_show_image_file(FileName: CString): CInteger;
  libakfavatar 'avt_show_image_file';

function avt_show_image_data(Data: pointer; size:CInteger): CInteger;
  libakfavatar 'avt_show_image_data';

procedure avt_set_background_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_background_color';

procedure avt_set_text_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_text_color';

procedure avt_set_text_background_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_text_background_color';

function initialize(title, icon: CString;
                     image: PAvatarImage;
                     mode: CInteger): CInteger;
  libakfavatar 'avt_initialize';

function avt_initialize_audio: CInteger; 
  libakfavatar 'avt_initialize_audio';

procedure avt_quit; libakfavatar 'avt_quit';

procedure avt_quit_audio; libakfavatar 'avt_quit_audio';

procedure avt_bell; libakfavatar 'avt_bell';

procedure avt_flash; libakfavatar 'avt_flash';

function avt_load_wave_file(f: CString): pointer;
  libakfavatar 'avt_load_wave_file';

function avt_load_wave_data (Data: Pointer; size: CInteger): PAvatarImage;
  libakfavatar 'avt_load_wave_data';

procedure avt_free_audio(snd: pointer); 
  libakfavatar 'avt_free_audio';

function avt_play_audio(snd: pointer; loop: CInteger): CInteger; 
  libakfavatar 'avt_play_audio';

function avt_wait_audio_end: CInteger; libakfavatar 'avt_wait_audio_end';

procedure avt_stop_audio; libakfavatar 'avt_stop_audio';

function avt_get_error: CString; libakfavatar 'avt_get_error';

procedure avt_viewport(x, y, width, height: CInteger); 
  libakfavatar 'avt_viewport';

function avt_where_x: CInteger; libakfavatar 'avt_where_x';
function avt_where_y: CInteger; libakfavatar 'avt_where_y';
procedure avt_move_x(x: CInteger); libakfavatar 'avt_move_x';
procedure avt_move_y(x: CInteger); libakfavatar 'avt_move_y';
function avt_get_max_x: CInteger; libakfavatar 'avt_get_max_x'; 
function avt_get_max_y: CInteger; libakfavatar 'avt_get_max_y'; 

procedure avt_delete_lines(line, num: CInteger);
  libakfavatar 'avt_delete_lines';

procedure avt_insert_lines(line, num: CInteger);
  libakfavatar 'avt_insert_lines';

procedure avt_text_direction(direction: CInteger); 
  libakfavatar 'avt_text_direction';
  
procedure avt_register_keyhandler(handler: pointer);
  libakfavatar 'avt_register_keyhandler';

procedure avt_set_scroll_mode(mode: CInteger); 
  libakfavatar 'avt_set_scroll_mode';

function avt_get_scroll_mode: CInteger; 
  libakfavatar 'avt_get_scroll_mode';

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

procedure setTextDelay(delay: integer);
begin
avt_set_text_delay (delay)
end;

procedure setFlipPageDelay(delay: integer);
begin
avt_set_flip_page_delay (delay)
end;

procedure setTextDirection(direction: TextDirection);
begin
avt_text_direction (ord (direction))
end;

procedure AvatarImageFile(FileName: string);
begin
{ when it is already initialized, it's too late }
if AvatarImage = NIL then
  AvatarImage := avt_import_image_file (String2CString(FileName))
end;

procedure AvatarImageData(data: pointer; size: LongInt);
begin
{ when it is already initialized, it's too late }
if AvatarImage = NIL then
  AvatarImage := avt_import_image_data (data, size)
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
if initialized then 
  begin
  avt_quit_audio;
  avt_quit
  end
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
ScrSize.x := avt_get_max_x;
ScrSize.y := avt_get_max_y;

{ set WindMin und WindMax }
WindMin := $0000;

if ScrSize.y-1 >= $FF
  then WindMax := $FF shl 8
  else WindMax := (ScrSize.y-1) shl 8;

if ScrSize.x-1 >= $FF
  then WindMax := WindMax or $FF
  else WindMax := WindMax or (ScrSize.x-1);

avt_initialize_audio;

NormVideo;
if avt_move_in<>0 then Halt
end;

procedure TextColor (Color: Byte);
begin
if not initialized then initializeAvatar;

{ strip blink attribute }
Color := Color and $0F;

TextAttr := (TextAttr and $F0) or Color;
OldTextAttr := TextAttr;

{ keep only the highcolor-bit }
if isMonochrome then Color := Color and $08;

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

{ strip what we don't need }
Color := Color and $0F;

TextAttr := (TextAttr and $0F) or (Color shl 4);
OldTextAttr := TextAttr;

{ no background color }
if isMonochrome then
  begin
  avt_set_text_background_color ($FF, $FF, $FF);
  exit
  end;

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

procedure delay (milliseconds: integer);
begin
if not initialized then initializeAvatar;
if avt_wait (milliseconds) <> 0 then Halt
end;

procedure MoveAvatarIn;
begin
if not initialized 
  then initializeAvatar
  else if avt_move_in <> 0 then Halt
end;

procedure MoveAvatarOut;
begin
if initialized then
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

procedure ShowImageData(data: pointer; size: LongInt);
var result : CInteger;
begin
if not initialized then initializeAvatar;

result := avt_show_image_data (data, size);
if result = 1 then Halt; { halt requested }

{ ignore failure to show image }
end;

function seconds(s: Real): integer;
begin seconds := trunc (s * 1000) end;

function AvatarGetError: ShortString;
begin
AvatarGetError := CString2String (avt_get_error)
end;

function WhereX: integer;
begin
WhereX := avt_where_x
end;

function WhereY: integer;
begin
WhereY := avt_where_y
end;

procedure GotoXY (x, y: integer);
begin
avt_move_x (x);
avt_move_y (y)
end;

procedure DelLine;
begin
avt_delete_lines(avt_where_y, 1)
end;

procedure InsLine;
begin
avt_insert_lines(avt_where_y, 1)
end;

procedure Window(x1, y1, x2, y2: Byte);
begin
{ do nothing when one value is invalid (defined behaviour) }
if (x1 >= 1) and (x1 <= ScrSize.x) and
   (y1 >= 1) and (y1 <= ScrSize.y) and
   (x2 >= x1) and (x2 <= ScrSize.x) and
   (y2 >= y1) and (y2 <= ScrSize.y) then
  begin
  avt_viewport(x1, y1, x2-x1+1, y2-y1+1);
  WindMin := ((y1-1) shl 8) or (x1-1);
  WindMax := ((y2-1) shl 8) or (x2-1)
  end
end;

procedure waitkey(const message: string);
begin
if not initialized then initializeAvatar;
if avt_wait_key_mb(String2CString(message))<>0 then Halt
end;

procedure checkParameters;
var i: integer;
begin
for i := 1 to ParamCount do
  if (ParamStr(i)='--fullscreen') or (ParamStr(i)='-f')
    then fullscreen := true
end;

function LoadSoundFile(const FileName: string): pointer;
begin
LoadSoundFile := avt_load_wave_file(String2CString(FileName))
end;

function LoadSoundData(data: pointer; size: LongInt): pointer;
begin
LoadSoundData := avt_load_wave_data(data, size)
end;

procedure Beep;
begin
avt_bell
end;

procedure Flash;
begin
avt_flash
end;

procedure FreeSound(snd: pointer);
begin
avt_free_audio(snd)
end;

procedure PlaySound(snd: pointer; loop: boolean);
begin
avt_play_audio(snd, ord(loop))
end;

procedure WaitSoundEnd;
begin
if avt_wait_audio_end<>0 then Halt
end;

{ dummy function, full support planned }
procedure Sound(frequency: integer);
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

procedure KeyHandler(sym, modifiers, unicode: CInteger); 
{$IfDef FPC} cdecl; {$EndIf}
begin
{$IfDef Debug}
  WriteLn(stderr, 'sym: ', sym, ' modifiers: ', modifiers,
          ' unicode: ', unicode);
{$EndIf}

{ CheckBreak, CheckEsc }
if (CheckBreak and (unicode=3)) or 
   (CheckEsc and (unicode=27)) then 
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

procedure ClearKeys;
begin
KeyboardBufferRead := KeyboardBufferWrite
end;

procedure SetScrollMode(mode: integer);
begin
avt_set_scroll_mode(mode)
end;

function GetScrollMode: integer;
begin
GetScrollMode := avt_get_scroll_mode
end;

function ScreenSize: TScreenSize;
begin
if not initialized then initializeAvatar;
ScreenSize := ScrSize
end;

{ ---------------------------------------------------------------------}
{ Input/output handling }

procedure AssignCrt (var f: text);
begin
AssignAvatar(f)
end;

{$IfDef FPC}

  function fpc_io_dummy (var F: TextRec): integer;
  begin
  fpc_io_dummy := 0
  end;

  function fpc_io_close (var F: TextRec): integer;
  begin
  F.Mode := fmClosed;
  fpc_io_close := 0
  end;

  function fpc_io_write (var F: TextRec): integer;
  var 
    s: CString;
    Status: integer;
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

  function fpc_io_read (var F: TextRec): integer;
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
  
  { clear KeyBoardBuffer }
  KeyboardBufferRead := KeyboardBufferWrite;
  
  fpc_io_read := 0
  end;

  function fpc_io_open (var F: TextRec): integer;
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
  
  { clear KeyBoardBuffer }
  KeyboardBufferRead := KeyboardBufferWrite;

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
  isMonochrome := false;
  KeyboardBufferRead := 0;
  KeyboardBufferWrite := 0;

  { these values are not yet known }
  ScrSize.x := -1;
  ScrSize.y := -1;
  WindMin := $0000;
  WindMax := $0000;
 
  TextAttr := $F0;
  OldTextAttr := TextAttr;
  CheckEsc := true;
  CheckBreak := true;
  CheckEof := false;
  CheckSnow := true;
  DirectVideo := true;
  FastQuit := false;

  checkParameters;

  avt_mb_encoding(DefaultEncoding);
  avt_set_scroll_mode(1);
  
  avt_stop_on_esc(ord(false)); { Esc is handled in the KeyHandler }
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
