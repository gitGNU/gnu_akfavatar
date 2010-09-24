{*
 * Pascal binding to the AKFAvatar library version 0.17.2
 * Copyright (c) 2007, 2008, 2009 Andreas K. Foerster <info@akfoerster.de>
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
    use the command Window
}

{$IfDef FPC}
  {$LongStrings on}
{$EndIf}

{$X+}

unit akfavatar;

interface

{ length of an input/output line }
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
  White        = 15; { as background-color -> balloon-color }
  Blink        = 128; { ignored }

{ for TextBackground }
const BalloonColor = 15;

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
{ should be used before any output took place }
{ but it can be changed later }
procedure AvatarImageFile(FileName: string);

{ load the Avatar image from memory }
{ should be used before any output took place }
{ but it can be changed later }
procedure AvatarImageData(data: pointer; size: LongInt);

{ load the Avatar image from XPM-data }
{ use the tool xpm2pas to import the data }
{ should be used before any output took place }
{ but it can be changed later }
{ example:  AvatarImageXPM(addr(image)); }
procedure AvatarImageXPM(data: pointer);

{ load the Avatar image from XBM-data }
{ use the tool xbm2pas to import the data }
{ should be used before any output took place }
{ but it can be changed later }
{ example: 
  AvatarImageXBM(addr(img_bits), img_width, img_height, 'black'); }
procedure AvatarImageXBM(bits: pointer; width, height: integer; 
                         colorname: string);

{ give the avatar a name }
procedure AvatarName(const Name: string);

{ set a different background color }
{ should be used before any output took place }
procedure setBackgroundColor(red, green, blue: byte);
procedure setBackgroundColorName (const Name: string);

{ set a different balloon color }
{ should be used before any output took place }
procedure setBalloonColor(red, green, blue: byte);
procedure setBalloonColorName (const Name: string);

{ change pace of text and page flipping }
{ the scale is milliseconds }
procedure setTextDelay(delay: integer);
procedure setFlipPageDelay(delay: integer);

{ change the encoding }
procedure setEncoding(const newEncoding: string);
function getEncoding: string;

{ change text direction (for hebrew/yiddish texts) }
{ you should start a new line before or after this command }
procedure setTextDirection(direction: TextDirection);

{ The "Screen" is the textarea }
{ The name is chosen for compatiblity with the CRT unit }
{ This causes the library to be initialized }
{ The avatar-image and the background color should be set before this }
function ScreenSize: TScreenSize;

{ sets the balloon size so that the text fits exactly,
  and prints the text }
procedure Tell(const txt: string);

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

{ switch cursor on or off }
{ extensions compatible to Free Pascal }
procedure CursorOn;
procedure CursorOff;

{ keyboard handling }
{ partly CRT compatible - only Latin1 chars so far }
function KeyPressed: boolean;
function ReadKey: char;

{ clear the keyboard buffer }
procedure ClearKeys;

{ wait for a key }
procedure WaitKey;

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

{ set black on white text colors, switch bold and underlined off }
{ the markup mode is also deactivated by this }
{ name compatible to CRT unit, but the colors differ }
procedure NormVideo;

{ switch bold mode on or off }
{ this is different from the CRT unit }
{ be careful, when you combine this with TextColor }
procedure HighVideo;
procedure LowVideo;

{ switch underline mode on or off }
procedure Underlined (onoff: boolean);

{ activate markup mode }
{ in markup mode the character "_" toggles the underlined mode
  and "*" toggles the bold mode on or off }
procedure MarkUp (onoff: boolean);

{ shows the avatar without the balloon }
procedure ShowAvatar;

{ moves the avatar in or out }
procedure MoveAvatarIn;
procedure MoveAvatarOut;

{ loads image
  after that call delay or waitkey 
  the supported image formats depend on your libraries
  XPM and uncompressed BMP is always supported
}
function ShowImageFile(FileName: string): boolean;
procedure ShowImageData(data: pointer; size: LongInt);

{ use the tool xpm2pas to import the X Pixmap data }
{ example: ShowImageXPM(addr(image)); }
procedure ShowImageXPM(data: pointer);

{ use the tool xbm2pas to import the X Bitmap data }
{ example: 
  ShowImageXBM(addr(img_bits), img_width, img_height, 'black'); }
procedure ShowImageXBM(bits: pointer; width, height: integer; 
                       colorname: string);

{ play a short sound as with chr(7) }
procedure Beep;

{ a short visual flash on the screen }
procedure Flash;

{ loads Audio File
  AU or WAV files supported }
function LoadSoundFile(const FileName: string): pointer;
function LoadSoundData(data: pointer; size: LongInt): pointer;
procedure FreeSound(snd: pointer);
procedure PlaySound(snd: pointer; loop: boolean);

{ wait until the end of the audio output }
procedure WaitSoundEnd;

{ play a sound of a given frequency }
{ Note: not fast, needs a delay(200) at least }
procedure Sound(frequency: integer);

{ stop sound output }
procedure NoSound;

{ handle coordinates (inside the balloon) }
{ compatible to CRT unit }
function WhereX: integer;
function WhereY: integer;
procedure GotoXY(x, y: integer);
procedure Window(x1, y1, x2, y2: Byte);

{ whether the cursor is in the home position? }
function HomePosition: boolean;

{ set the size of the balloon }
{ the window is reset to the new full size }
procedure BalloonSize(height, width: integer);
procedure BalloonWidth(width: integer);
procedure BalloonHeight(height: integer);

{ set/get scroll mode }
{ 0 = off (page-flipping), 1 = normal }
procedure SetScrollMode(mode: integer);
function GetScrollMode: integer;

{ get last error message }
function AvatarGetError: ShortString;

{ ignore TextColor TextBackground and so on }
{ compatible with GNU-Pascal's CRT unit }
procedure SetMonochrome(monochrome: boolean);

{ for positive/negative questions }
{ keys for positive: + 1 Enter }
{ keys for negative: - 0 Backspace }
function Decide: boolean;

{ Navigate }
{
 navigation bar
 
 buttons is a string with the following characters
 'l': left
 'r': right (play)
 'd': down
 'u': up
 'x': cancel
 'f': (fast)forward
 'b': (fast)backward
 'p': pause
 's': stop
 'e': eject
 '*': circle (record)
 '+': plus (add)
 '-': minus (remove)
 '?': help
 ' ': spacer (no button)
 
 Pressing a key with one of those characters selects it.
 For the directions you can also use the arrow keys,
 The [Pause] key returns 'p'.
 The [Help] key or [F1] return '?'.

 the function returns the letter for the selected option

 example:
   case Navigate('lxr') of ...
}

function Navigate(buttons: String): char;

{ choice for several items }
{ result is the choice number, starting from 1 }
{ startkey may be #0 }
function Choice(start_line, items: integer; startkey: char;
                back, fwrd: boolean): integer;

{ show a very long text in a pager }
{ You can navigate with up/down, page up/page down keys,
  Home and End keys, and even with the mouse-wheel }
procedure PagerString (const txt: string; startline: integer);
procedure PagerFile (const filename: string; startline: integer);

{ lock or unlock updates - can be used for speedups }
{ when true the text_delay is set to 0 }
{ when false the textarea gets updated }
{ use with care! }
procedure LockUpdates(lock: boolean);

implementation

{-----------------------------------------------------------------------}

{$IfDef FPC}
  uses DOS, Strings;
  
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

{ no real booleans used in the librarys interface }
type avt_bool_t = Byte;

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

{ for sound generator }
const 
  SampleRate = 44100;
  BufMax = 4 * Samplerate;

type TRawSoundBuf = array[0..BufMax] of SmallInt;

{ RawSoundBuf is reserved the first time Sound() is called }
var RawSoundBuf: ^TRawSoundBuf;
var GenSound: Pointer; { generated sound }


{ bindings: }

procedure avt_reserve_single_keys (onoff: avt_bool_t); 
  libakfavatar 'avt_reserve_single_keys';

function avt_default: PAvatarImage; libakfavatar 'avt_default';

function avt_get_status: CInteger; libakfavatar 'avt_get_status';

procedure avt_set_text_delay (delay: CInteger);
  libakfavatar 'avt_set_text_delay';

procedure avt_set_flip_page_delay (delay: CInteger);
  libakfavatar 'avt_set_flip_page_delay';

function avt_say_mb_len(t: pointer; size: CInteger): CInteger;
  libakfavatar 'avt_say_mb_len';

function avt_tell_mb(t: pointer): CInteger; libakfavatar 'avt_tell_mb';

procedure avt_clear; libakfavatar 'avt_clear';

procedure avt_clear_eol; libakfavatar 'avt_clear_eol';

function avt_mb_encoding (encoding: CString): CInteger;
  libakfavatar 'avt_mb_encoding';

function avt_get_mb_encoding (): CString;
  libakfavatar 'avt_get_mb_encoding';

function avt_ask_mb(t: pointer; size: CInteger): CInteger;
  libakfavatar 'avt_ask_mb';

function avt_wait(milliseconds: CInteger): CInteger; 
  libakfavatar 'avt_wait';

function avt_wait_button: CInteger; 
  libakfavatar 'avt_wait_button';

function avt_move_in: CInteger; libakfavatar 'avt_move_in';

function avt_move_out: CInteger; libakfavatar 'avt_move_out';

procedure avt_show_avatar; libakfavatar 'avt_show_avatar';

function avt_import_gimp_image(gimp_image: PGimpImage): PAvatarImage;
   libakfavatar 'avt_import_gimp_image';

function avt_import_image_file (FileName: CString): PAvatarImage;
  libakfavatar 'avt_import_image_file';

function avt_import_image_data(Data: Pointer; size: CInteger): PAvatarImage;
  libakfavatar 'avt_import_image_data';

function avt_import_xbm(bits: Pointer; width, height: CInteger;
                        colorname: CString): PAvatarImage;
  libakfavatar 'avt_import_xbm';

function avt_import_xpm(data: Pointer): PAvatarImage;
  libakfavatar 'avt_import_xpm';

function avt_change_avatar_image(image: PAvatarImage): CInteger;
  libakfavatar 'avt_change_avatar_image';

procedure avt_free_image(image: PAvatarImage);
  libakfavatar 'avt_free_image';

function avt_set_avatar_name_mb (name: CString): CInteger;
  libakfavatar 'avt_set_avatar_name_mb';

function avt_show_image_file(FileName: CString): CInteger;
  libakfavatar 'avt_show_image_file';

function avt_show_image_data(Data: pointer; size: CInteger): CInteger;
  libakfavatar 'avt_show_image_data';

function avt_show_image_xbm(bits: pointer; widrh, height: CInteger;
                            colorname: CString): CInteger;
  libakfavatar 'avt_show_image_xbm';

function avt_show_image_xpm(data: pointer): CInteger;
  libakfavatar 'avt_show_image_xpm';

procedure avt_set_background_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_background_color';

procedure avt_set_background_color_name (name: CString);
  libakfavatar 'avt_set_background_color_name';

procedure avt_set_balloon_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_balloon_color';

procedure avt_set_balloon_color_name (name: CString);
  libakfavatar 'avt_set_balloon_color_name';

procedure avt_set_text_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_text_color';

procedure avt_set_text_background_color (red, green, blue: CInteger);
  libakfavatar 'avt_set_text_background_color';

procedure avt_set_text_background_ballooncolor;
  libakfavatar 'avt_set_text_background_ballooncolor';

procedure avt_bold (onoff: avt_bool_t); libakfavatar 'avt_bold';

procedure avt_underlined (onoff: avt_bool_t); libakfavatar 'avt_underlined';

procedure avt_markup (onoff: avt_bool_t); libakfavatar 'avt_markup';

procedure avt_normal_text; libakfavatar 'avt_normal_text';

procedure avt_activate_cursor (onoff: avt_bool_t); 
  libakfavatar 'avt_activate_cursor';

function avt_initialize(title, icon: CString;
                        image: PAvatarImage;
                        mode: CInteger): CInteger;
  libakfavatar 'avt_initialize';

function avt_initialize_audio: CInteger; 
  libakfavatar 'avt_initialize_audio';

procedure avt_quit; libakfavatar 'avt_quit';

procedure avt_bell; libakfavatar 'avt_bell';

procedure avt_flash; libakfavatar 'avt_flash';

function avt_load_audio_file(f: CString): pointer;
  libakfavatar 'avt_load_audio_file';

function avt_load_audio_data (Data: Pointer; size: CInteger): Pointer;
  libakfavatar 'avt_load_audio_data';

function avt_load_raw_audio_data (Data: pointer; size: CInteger;
                            Samplingrate, Audio_type, 
                            channels: CInteger): pointer;
  libakfavatar 'avt_load_raw_audio_data';

procedure avt_free_audio(snd: pointer); 
  libakfavatar 'avt_free_audio';

function avt_play_audio(snd: pointer; loop: avt_bool_t): CInteger; 
  libakfavatar 'avt_play_audio';

function avt_wait_audio_end: CInteger; libakfavatar 'avt_wait_audio_end';

procedure avt_stop_audio; libakfavatar 'avt_stop_audio';

function avt_get_error: CString; libakfavatar 'avt_get_error';

procedure avt_viewport(x, y, width, height: CInteger); 
  libakfavatar 'avt_viewport';

procedure avt_set_balloon_size(height, width: CInteger);
  libakfavatar 'avt_set_balloon_size';

procedure avt_set_balloon_width(width: CInteger);
  libakfavatar 'avt_set_balloon_width';

procedure avt_set_balloon_height(height: CInteger);
  libakfavatar 'avt_set_balloon_height';

function avt_where_x: CInteger; libakfavatar 'avt_where_x';
function avt_where_y: CInteger; libakfavatar 'avt_where_y';
procedure avt_move_xy(x, y: CInteger); libakfavatar 'avt_move_xy';
function avt_get_max_x: CInteger; libakfavatar 'avt_get_max_x'; 
function avt_get_max_y: CInteger; libakfavatar 'avt_get_max_y';
function avt_home_position: avt_bool_t; libakfavatar 'avt_home_position';

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


function avt_choice(var result: CInteger; 
                    start_line, items, key: CInteger;
                    back, fwrd: avt_bool_t): CInteger; 
  libakfavatar 'avt_choice';

procedure avt_pager_mb (txt: CString; len, startline: CInteger); 
  libakfavatar 'avt_pager_mb';

function avt_navigate(buttons: CString): CInteger;
  libakfavatar 'avt_navigate';

function avt_decide: avt_bool_t; libakfavatar 'avt_decide';

procedure avt_lock_updates(lock: avt_bool_t);
  libakfavatar 'avt_lock_updates';

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

procedure setBackgroundColorName (const Name: string);
begin
avt_set_background_color_name (String2CString(name))
end;

procedure setBalloonColor (red, green, blue: byte);
begin
avt_set_balloon_color(red, green, blue)
end;

procedure setBalloonColorName (const Name: string);
begin
avt_set_balloon_color_name (String2CString(name))
end;

procedure setEncoding(const newEncoding: string);
begin
avt_mb_encoding(String2CString(newEncoding))
end;

function getEncoding: string;
begin
getEncoding := CString2String (avt_get_mb_encoding)
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
if AvatarImage <> NIL then avt_free_image(AvatarImage);

AvatarImage := avt_import_image_file(String2CString(FileName));

if initialized then 
  begin
  avt_change_avatar_image(AvatarImage);
  AvatarImage := NIL
  end
end;

procedure AvatarImageData(data: pointer; size: LongInt);
begin
if AvatarImage <> NIL then avt_free_image(AvatarImage);

AvatarImage := avt_import_image_data(data, size);

if initialized then
  begin
  avt_change_avatar_image(AvatarImage);
  AvatarImage := NIL
  end
end;

procedure AvatarImageXPM(data: pointer);
begin
if AvatarImage <> NIL then avt_free_image(AvatarImage);

AvatarImage := avt_import_xpm(data);

if initialized then
  begin
  avt_change_avatar_image(AvatarImage);
  AvatarImage := NIL
  end
end;

procedure AvatarImageXBM(bits: pointer; width, height: integer;
                         colorname: string);
begin
if AvatarImage <> NIL then avt_free_image(AvatarImage);

AvatarImage := avt_import_xbm(bits, width, height, 
                              String2CString(colorname));

if initialized then
  begin
  avt_change_avatar_image(AvatarImage);
  AvatarImage := NIL
  end
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
if RawSoundBuf<>NIL then Dispose(RawSoundBuf);

if initialized then 
  avt_quit
end;

procedure initializeAvatar;
begin
if AvatarImage = NIL then AvatarImage := avt_default;

if avt_initialize('AKFAvatar', 'AKFAvatar', AvatarImage,
                  ord(fullscreen)) < 0 
  then 
    begin
    WriteLn(stderr, 'cannot initialize graphics: ', AvatarGetError);
    Halt(1)
    end;

if avt_get_status = 1 then Halt; { shouldn't happen here yet }

initialized := true;

AvatarImage := NIL; { it was freed by initialize }
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

procedure AvatarName(const Name: string);
begin
if not initialized then initializeAvatar;
avt_set_avatar_name_mb (String2CString(Name))
end;

procedure Tell(const txt: string);
begin
if not initialized then initializeAvatar;
avt_tell_mb(String2CString(txt))
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
  White        : avt_set_text_background_ballooncolor ()
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

avt_markup (ord(false));
avt_normal_text ();
TextAttr := $F0;
OldTextAttr := TextAttr;
end;

procedure HighVideo;
begin
avt_bold (ord(true))
end;

procedure LowVideo;
begin
avt_bold (ord(false))
end;

procedure Underlined (onoff: boolean);
begin
avt_underlined (ord(onoff))
end;

procedure MarkUp (onoff: boolean);
begin
avt_markup (ord(onoff))
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

procedure ShowImageXPM(data: pointer);
var result : CInteger;
begin
if not initialized then initializeAvatar;

result := avt_show_image_xpm (data);
if result = 1 then Halt; { halt requested }

{ ignore failure to show image }
end;

procedure ShowImageXBM(bits: pointer; width, height: integer; 
                       colorname: string);
var result : CInteger;
begin
if not initialized then initializeAvatar;

result := avt_show_image_xbm (bits, width, height, 
                              String2CString(colorname));
if result = 1 then Halt; { halt requested }

{ ignore failure to show image }
end;

procedure PagerString (const txt: string; startline: integer);
begin
if not initialized then initializeAvatar;
{ getting the string-length in pascal is lightweight }
{ converting to a CString would be more heavy }
avt_pager_mb (addr(txt[1]), length(txt), startline)
end;

procedure PagerFile (const filename: string; startline: integer);
var 
  f: file;
  buf: ^char;
  size, numread: LongInt;
begin
if not initialized then initializeAvatar;

assign(f, filename);
reset(f, 1);
size := FileSize(f);
if size > 0 then GetMem (buf, size);
BlockRead(f, buf^, size, numread);
close(f);

avt_pager_mb(buf, numread, startline);
if size > 0 then FreeMem (buf, size)
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

function HomePosition: boolean;
begin
HomePosition := (avt_home_position<>0)
end;

procedure GotoXY (x, y: integer);
begin
avt_move_xy (x, y)
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

procedure BalloonSize(height, width: integer);
begin
if not initialized then initializeAvatar;
avt_set_balloon_size(height, width);

{ set the sizes to what we really get, not what was asked for }
ScrSize.x := avt_get_max_x;
ScrSize.y := avt_get_max_y;
Window(1, 1, ScrSize.x, ScrSize.y);
end;

procedure BalloonWidth(width: integer);
begin
if not initialized then initializeAvatar;
avt_set_balloon_width (width);
ScrSize.x := avt_get_max_x;
Window(1, 1, ScrSize.x, ScrSize.y);
end;

procedure BalloonHeight(height: integer);
begin
if not initialized then initializeAvatar;
avt_set_balloon_height (height);
ScrSize.y := avt_get_max_y;
Window(1, 1, ScrSize.x, ScrSize.y);
end;

procedure waitkey;
begin
if not initialized then initializeAvatar;
if avt_wait_button<>0 then Halt
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
LoadSoundFile := avt_load_audio_file(String2CString(FileName))
end;

function LoadSoundData(data: pointer; size: LongInt): pointer;
begin
LoadSoundData := avt_load_audio_data(data, size)
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

procedure Sound(frequency: integer);
const
  S16SYS = 8;
  Mono = 1;
  Volume = 75; { volume in percent }
  Amplitude = Volume * 32767 div 100;
var i: LongInt;
begin
if RawSoundBuf=NIL then New(RawSoundBuf);

for i := 0 to BufMax do
  RawSoundBuf^[i] := trunc(Amplitude * sin(2*pi*frequency*i/Samplerate));

if GenSound<>NIL then avt_free_audio(GenSound);
GenSound := avt_load_raw_audio_data(RawSoundBuf, BufMax, 
                                    SampleRate, S16SYS, Mono);

avt_play_audio(GenSound, ord(true))
end;

procedure NoSound;
begin
avt_stop_audio;

if GenSound<>NIL then 
  begin
  avt_free_audio(GenSound);
  GenSound := NIL
  end
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

procedure CursorOff;
begin
avt_activate_cursor (ord(false))
end;

procedure CursorOn;
begin
avt_activate_cursor (ord(true))
end;

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

function Choice(start_line, items: integer; startkey: char;
                back, fwrd: boolean): integer;
var result: CInteger;
begin
if not initialized then initializeAvatar;
if avt_choice(result, start_line, items, CInteger(startkey), 
              ord(back), ord(fwrd))<>0 then Halt;
Choice := result
end;

procedure LockUpdates(lock: boolean);
begin
avt_lock_updates(ord(lock))
end;

function Decide: boolean;
begin
if not initialized then initializeAvatar;
Decide := (avt_decide <> 0);
if avt_get_status<>0 then Halt
end;

function Navigate(buttons: String): char;
var result: CInteger;
begin
if not initialized then initializeAvatar;
result := avt_navigate(String2CString(buttons));
if avt_get_status<>0 then Halt;
Navigate := chr(result)
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
  begin
  if F.BufPos > 0 then
    begin
    if not initialized then initializeAvatar;

    if TextAttr<>OldTextAttr then UpdateTextAttr;

    if avt_say_mb_len (F.BufPtr, F.BufPos) <> 0 then Halt;
    F.BufPos := 0; { everything read }
    end;
  fpc_io_write := 0
  end;

  function fpc_io_read (var F: TextRec): integer;
  begin
  if not initialized then initializeAvatar;
  if TextAttr<>OldTextAttr then UpdateTextAttr;

  if avt_ask_mb (F.BufPtr, F.BufSize) <> 0 then Halt;

  F.BufPos := 0;
  F.BufEnd := strlen(F.BufPtr^) + 2;

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
         F.InOutFunc := @fpc_io_write; { unsure }
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
  begin
  if size > 0 then
    begin
    if not initialized then initializeAvatar;
    if TextAttr<>OldTextAttr then UpdateTextAttr;
    
    if avt_say_mb_len (Addr(Buffer), size) <> 0 then Halt
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
  RawSoundBuf := NIL;
  GenSound := NIL;

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
  
  avt_reserve_single_keys(ord(true)); { Esc is handled in the KeyHandler }
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

  NoSound;

  if initialized and not FastQuit then
    if avt_get_status = 0 then 
      begin
      { wait for key, when balloon is visible }
      if avt_where_x >= 0 then 
        if avt_wait_button = 0 then avt_move_out
      end;

  Quit
end.
