{*
 * Pascal binding to the AKFAvatar library version 0.22.0
 * Copyright (c) 2007,2008,2009,2011,2012 Andreas K. Foerster <info@akfoerster.de>
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
  type LineString = string(4 * LineLength);
  type ShortString = string(255);
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
procedure setBackgroundColorName(const Name: string);
procedure getBackgroundColor(var red, green, blue: byte);

{ set a different balloon color }
{ should be used before any output took place }
procedure setBalloonColor(red, green, blue: byte);
procedure setBalloonColorName(const Name: string);

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

{ example use: delay(seconds(2.5)); }
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
procedure TextColor(Color: Byte);

{ set the text background color }
{ compatible to CRT unit, but light colors can be used }
procedure TextBackground(Color: Byte);

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
procedure Underlined(onoff: boolean);

{ activate markup mode }
{ in markup mode the character "_" toggles the underlined mode
  and "*" toggles the bold mode on or off }
procedure MarkUp(onoff: boolean);

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

{ show image from raw data }
{ BytesPerPixel may be 3 for RGB, or 4 for RGBA }
procedure ShowRawImage(Data: Pointer; width, height, BytesPerPixel: Integer);

{ maximum size for ShowRawImage }
function ImageMaxWidth: Integer;
function ImageMaxHeight: Integer;

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
function Playing(snd: pointer): boolean;

{ for importing raw sound data }

{ constants for audio_type }
const
  AVT_AUDIO_UNKNOWN = 0;
  AVT_AUDIO_U8      = 1;  { unsigned 8 Bit }
  AVT_AUDIO_S8      = 2;  { signed 8 Bit }
  AVT_AUDIO_U16LE   = 3;  { unsigned 16 Bit little endian }
  AVT_AUDIO_U16BE   = 4;  { unsigned 16 Bit big endian }
  AVT_AUDIO_U16SYS  = 5;  { unsigned 16 Bit system's endianess }
  AVT_AUDIO_S16LE   = 6;  { signed 16 Bit little endian }
  AVT_AUDIO_S16BE   = 7;  { signed 16 Bit big endian }
  AVT_AUDIO_S16SYS  = 8;  { signed 16 Bit system's endianess }
  AVT_AUDIO_MULAW   = 100;  { 8 Bit mu-law (u-law) }
  AVT_AUDIO_ALAW    = 101;  { 8 Bit A-Law }

function LoadRawSoundData(data:pointer; size: LongInt;
           samplingrate, audio_type, channels: integer): pointer;
procedure AddRawSoundData(sound: pointer; data: pointer; size: LongInt);


{ wait until the end of the audio output }
procedure WaitSoundEnd;

{ play a sound of a given frequency }
{ Note: not fast, needs a delay(200) at least }
procedure Sound(frequency: integer);

{ stop sound output }
procedure NoSound;

{ pause or resume Sound }
procedure PauseSound(pause: boolean);

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
procedure PagerString(const txt: string; startline: integer);
procedure PagerFile(const filename: string; startline: integer);

{ lock or unlock updates - can be used for speedups }
{ when true the text_delay is set to 0 }
{ when false the textarea gets updated }
{ use with care! }
procedure LockUpdates(lock: boolean);

implementation

{-----------------------------------------------------------------------}

{$IfDef FPC}
  uses DOS, Strings, CTypes;

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
    CString = PChar;
    CBoolean = Boolean; { not cbool! }
{$EndIf}

{$IfDef __GPC__}
  type Csize_t = SizeType;

  {$if __GPC_RELEASE__ < 20041218}
    type Cint = Integer;
  {$else}
    type Cint = CInteger;
  {$EndIf}
{$EndIf}

type PAvatarImage = pointer;

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

procedure avt_reserve_single_keys(onoff: CBoolean); 
  libakfavatar 'avt_reserve_single_keys';

function avt_default: PAvatarImage; libakfavatar 'avt_default';

function avt_get_status: Cint; libakfavatar 'avt_get_status';

procedure avt_set_text_delay(delay: Cint);
  libakfavatar 'avt_set_text_delay';

procedure avt_set_flip_page_delay(delay: Cint);
  libakfavatar 'avt_set_flip_page_delay';

function avt_say_mb_len(t: pointer; size: Csize_t): Cint;
  libakfavatar 'avt_say_mb_len';

function avt_tell_mb_len(t: pointer; len: Csize_t): Cint;
  libakfavatar 'avt_tell_mb_len';

procedure avt_clear; libakfavatar 'avt_clear';

procedure avt_clear_eol; libakfavatar 'avt_clear_eol';

function avt_mb_encoding(encoding: CString): Cint;
  libakfavatar 'avt_mb_encoding';

function avt_get_mb_encoding(): CString;
  libakfavatar 'avt_get_mb_encoding';

function avt_ask_mb(t: pointer; size: Csize_t): Cint;
  libakfavatar 'avt_ask_mb';

function avt_wait(milliseconds: Csize_t): Cint; 
  libakfavatar 'avt_wait';

function avt_wait_button: Cint; 
  libakfavatar 'avt_wait_button';

function avt_move_in: Cint; libakfavatar 'avt_move_in';

function avt_move_out: Cint; libakfavatar 'avt_move_out';

procedure avt_show_avatar; libakfavatar 'avt_show_avatar';

function avt_import_image_file(FileName: CString): PAvatarImage;
  libakfavatar 'avt_import_image_file';

function avt_import_image_data(Data: Pointer; size: Csize_t): PAvatarImage;
  libakfavatar 'avt_import_image_data';

function avt_import_xbm(bits: Pointer; width, height: Cint;
                        colorname: CString): PAvatarImage;
  libakfavatar 'avt_import_xbm';

function avt_import_xpm(data: Pointer): PAvatarImage;
  libakfavatar 'avt_import_xpm';

function avt_change_avatar_image(image: PAvatarImage): Cint;
  libakfavatar 'avt_change_avatar_image';

procedure avt_free_image(image: PAvatarImage);
  libakfavatar 'avt_free_image';

function avt_set_avatar_name_mb(name: CString): Cint;
  libakfavatar 'avt_set_avatar_name_mb';

function avt_show_image_file(FileName: CString): Cint;
  libakfavatar 'avt_show_image_file';

function avt_show_image_data(Data: pointer; size: Csize_t): Cint;
  libakfavatar 'avt_show_image_data';

function avt_show_image_xbm(bits: pointer; widrh, height: Cint;
                            colorname: CString): Cint;
  libakfavatar 'avt_show_image_xbm';

function avt_show_image_xpm(data: pointer): Cint;
  libakfavatar 'avt_show_image_xpm';

function avt_show_raw_image(Data: pointer; Width, Height, BPP: Cint): Cint;
  libakfavatar 'avt_show_raw_image';

function avt_image_max_width: Cint; libakfavatar 'avt_image_max_width';
function avt_image_max_height: Cint; libakfavatar 'avt_image_max_height';

procedure avt_get_background_color(var red, green, blue: Cint);
  libakfavatar 'avt_get_background_color';

procedure avt_set_background_color(red, green, blue: Cint);
  libakfavatar 'avt_set_background_color';

procedure avt_set_background_color_name(name: CString);
  libakfavatar 'avt_set_background_color_name';

procedure avt_set_balloon_color(red, green, blue: Cint);
  libakfavatar 'avt_set_balloon_color';

procedure avt_set_balloon_color_name(name: CString);
  libakfavatar 'avt_set_balloon_color_name';

procedure avt_set_text_color(red, green, blue: Cint);
  libakfavatar 'avt_set_text_color';

procedure avt_set_text_background_color(red, green, blue: Cint);
  libakfavatar 'avt_set_text_background_color';

procedure avt_set_text_background_ballooncolor;
  libakfavatar 'avt_set_text_background_ballooncolor';

procedure avt_bold(onoff: CBoolean); libakfavatar 'avt_bold';

procedure avt_underlined(onoff: CBoolean); libakfavatar 'avt_underlined';

procedure avt_markup(onoff: CBoolean); libakfavatar 'avt_markup';

procedure avt_normal_text; libakfavatar 'avt_normal_text';

procedure avt_activate_cursor(onoff: CBoolean); 
  libakfavatar 'avt_activate_cursor';

function avt_initialize(title, icon: CString;
                        image: PAvatarImage;
                        mode: Cint): Cint;
  libakfavatar 'avt_initialize';

function avt_initialize_audio: Cint; 
  libakfavatar 'avt_initialize_audio';

procedure avt_quit; libakfavatar 'avt_quit';

procedure avt_bell; libakfavatar 'avt_bell';

procedure avt_flash; libakfavatar 'avt_flash';

function avt_load_audio_file(f: CString): pointer;
  libakfavatar 'avt_load_audio_file';

function avt_load_audio_data(Data: Pointer; size: Csize_t): Pointer;
  libakfavatar 'avt_load_audio_data';

function avt_load_raw_audio_data(Data: pointer; size: Csize_t;
                            Samplingrate, Audio_type,
                            channels: Cint): pointer;
  libakfavatar 'avt_load_raw_audio_data';

function avt_add_raw_audio_data(Sound: pointer;
                                Data: pointer;
                                size: Csize_t): Cint;
  libakfavatar 'avt_add_raw_audio_data';

function avt_audio_playing(snd: pointer): CBoolean;
  libakfavatar 'avt_audio_playing';

procedure avt_free_audio(snd: pointer); 
  libakfavatar 'avt_free_audio';

function avt_play_audio(snd: pointer; loop: CBoolean): Cint; 
  libakfavatar 'avt_play_audio';

function avt_wait_audio_end: Cint; libakfavatar 'avt_wait_audio_end';

procedure avt_stop_audio; libakfavatar 'avt_stop_audio';

procedure avt_pause_audio(pause: CBoolean); libakfavatar 'avt_pause_audio';

function avt_get_error: CString; libakfavatar 'avt_get_error';

procedure avt_viewport(x, y, width, height: Cint); 
  libakfavatar 'avt_viewport';

procedure avt_set_balloon_size(height, width: Cint);
  libakfavatar 'avt_set_balloon_size';

procedure avt_set_balloon_width(width: Cint);
  libakfavatar 'avt_set_balloon_width';

procedure avt_set_balloon_height(height: Cint);
  libakfavatar 'avt_set_balloon_height';

function avt_where_x: Cint; libakfavatar 'avt_where_x';
function avt_where_y: Cint; libakfavatar 'avt_where_y';
procedure avt_move_xy(x, y: Cint); libakfavatar 'avt_move_xy';
function avt_get_max_x: Cint; libakfavatar 'avt_get_max_x'; 
function avt_get_max_y: Cint; libakfavatar 'avt_get_max_y';
function avt_home_position: CBoolean; libakfavatar 'avt_home_position';

procedure avt_delete_lines(line, num: Cint);
  libakfavatar 'avt_delete_lines';

procedure avt_insert_lines(line, num: Cint);
  libakfavatar 'avt_insert_lines';

procedure avt_text_direction(direction: Cint); 
  libakfavatar 'avt_text_direction';
  
procedure avt_register_keyhandler(handler: pointer);
  libakfavatar 'avt_register_keyhandler';

procedure avt_set_scroll_mode(mode: Cint); 
  libakfavatar 'avt_set_scroll_mode';

function avt_get_scroll_mode: Cint; 
  libakfavatar 'avt_get_scroll_mode';


function avt_choice(var result: Cint;
                    start_line, items, key: Cint;
                    back, fwrd: CBoolean): Cint; 
  libakfavatar 'avt_choice';

procedure avt_pager_mb(txt: CString; len: Csize_t; startline: Cint); 
  libakfavatar 'avt_pager_mb';

function avt_navigate(buttons: CString): Cint;
  libakfavatar 'avt_navigate';

function avt_decide: CBoolean; libakfavatar 'avt_decide';

procedure avt_lock_updates(lock: CBoolean);
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

procedure setBackgroundColor(red, green, blue: byte);
begin
avt_set_background_color(red, green, blue)
end;

procedure setBackgroundColorName(const Name: string);
begin
avt_set_background_color_name(String2CString(name))
end;

procedure setBalloonColor(red, green, blue: byte);
begin
avt_set_balloon_color(red, green, blue)
end;

procedure setBalloonColorName(const Name: string);
begin
avt_set_balloon_color_name(String2CString(name))
end;

procedure setEncoding(const newEncoding: string);
begin
avt_mb_encoding(String2CString(newEncoding))
end;

function getEncoding: string;
begin
getEncoding := CString2String(avt_get_mb_encoding)
end;

procedure setTextDelay(delay: integer);
begin
avt_set_text_delay(delay)
end;

procedure setFlipPageDelay(delay: integer);
begin
avt_set_flip_page_delay(delay)
end;

procedure setTextDirection(direction: TextDirection);
begin
avt_text_direction(ord(direction))
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
Close(input);
Close(output);
{$I+}

InOutRes := 0;

Assign(input, '');
Assign(output, '');
Reset(input);
Rewrite(output)
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
  else WindMax :=(ScrSize.y-1) shl 8;

if ScrSize.x-1 >= $FF
  then WindMax := WindMax or $FF
  else WindMax := WindMax or(ScrSize.x-1);

avt_initialize_audio;

NormVideo;
avt_move_in { do not call Halt here }
end;

procedure AvatarName(const Name: string);
begin
if not initialized then initializeAvatar;
avt_set_avatar_name_mb(String2CString(Name))
end;

procedure Tell(const txt: string);
begin
if not initialized then initializeAvatar;
avt_tell_mb_len(String2CString(txt), length(txt))
end;

procedure TextColor(Color: Byte);
begin
if not initialized then initializeAvatar;

{ strip blink attribute }
Color := Color and $0F;

TextAttr := (TextAttr and $F0) or Color;
OldTextAttr := TextAttr;

{ keep only the highcolor-bit }
if isMonochrome then Color := Color and $08;

case Color of
  Black        : avt_set_text_color($00, $00, $00);
  Blue         : avt_set_text_color($00, $00, $88);
  Green        : avt_set_text_color($00, $88, $00);
  Cyan         : avt_set_text_color($00, $88, $88);
  Red          : avt_set_text_color($88, $00, $00);
  Magenta      : avt_set_text_color($88, $00, $88);
  Brown        : avt_set_text_color($88, $88, $00);
  LightGray    : avt_set_text_color($CC, $CC, $CC);
  DarkGray     : avt_set_text_color($88, $88, $88);
  LightBlue    : avt_set_text_color($00, $00, $FF);
  LightGreen   : avt_set_text_color($00, $FF, $00);
  LightCyan    : avt_set_text_color($00, $FF, $FF);
  LightRed     : avt_set_text_color($FF, $00, $00); 
  LightMagenta : avt_set_text_color($FF, $00, $FF);
  Yellow       : avt_set_text_color($FF, $FF, $00);
  White        : avt_set_text_color($FF, $FF, $FF)
  end
end;

procedure TextBackground(Color: Byte);
begin
if not initialized then initializeAvatar;

{ strip what we don't need }
Color := Color and $0F;

TextAttr := (TextAttr and $0F) or (Color shl 4);
OldTextAttr := TextAttr;

{ no background color }
if isMonochrome then
  begin
  avt_set_text_background_color($FF, $FF, $FF);
  exit
  end;

case Color of
  Black        : avt_set_text_background_color($00, $00, $00);
  Blue         : avt_set_text_background_color($00, $00, $88);
  Green        : avt_set_text_background_color($00, $88, $00);
  Cyan         : avt_set_text_background_color($00, $88, $88);
  Red          : avt_set_text_background_color($88, $00, $00);
  Magenta      : avt_set_text_background_color($88, $00, $88);
  Brown        : avt_set_text_background_color($88, $88, $00);
  LightGray    : avt_set_text_background_color($CC, $CC, $CC);
  DarkGray     : avt_set_text_background_color($88, $88, $88);
  LightBlue    : avt_set_text_background_color($00, $00, $FF);
  LightGreen   : avt_set_text_background_color($00, $FF, $00);
  LightCyan    : avt_set_text_background_color($00, $FF, $FF);
  LightRed     : avt_set_text_background_color($FF, $00, $00); 
  LightMagenta : avt_set_text_background_color($FF, $00, $FF);
  Yellow       : avt_set_text_background_color($FF, $FF, $00);
  White        : avt_set_text_background_ballooncolor()
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

avt_markup(false);
avt_normal_text();
TextAttr := $F0;
OldTextAttr := TextAttr;
end;

procedure HighVideo;
begin
avt_bold(true)
end;

procedure LowVideo;
begin
avt_bold(false)
end;

procedure Underlined(onoff: boolean);
begin
avt_underlined(onoff)
end;

procedure MarkUp(onoff: boolean);
begin
avt_markup(onoff)
end;

procedure SetMonochrome(monochrome: Boolean);
begin
isMonochrome := monochrome
end;

procedure delay(milliseconds: integer);
begin
if not initialized then initializeAvatar;
if avt_wait(milliseconds) <> 0 then Halt
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

function ShowImageFile(FileName: string): boolean;
var result : Cint;
begin
if not initialized then initializeAvatar;
result := avt_show_image_file(String2CString(FileName));

if result = 1 then Halt; { halt requested }
if result = 0 
  then ShowImageFile := true { success }
  else ShowImageFile := false { failure }
end;

procedure ShowImageData(data: pointer; size: LongInt);
var result : Cint;
begin
if not initialized then initializeAvatar;

result := avt_show_image_data(data, size);
if result = 1 then Halt; { halt requested }

{ ignore failure to show image }
end;

procedure ShowImageXPM(data: pointer);
var result : Cint;
begin
if not initialized then initializeAvatar;

result := avt_show_image_xpm(data);
if result = 1 then Halt; { halt requested }

{ ignore failure to show image }
end;

procedure ShowImageXBM(bits: pointer; width, height: integer; 
                       colorname: string);
var result : Cint;
begin
if not initialized then initializeAvatar;

result := avt_show_image_xbm(bits, width, height, 
                              String2CString(colorname));
if result = 1 then Halt; { halt requested }

{ ignore failure to show image }
end;

procedure ShowRawImage(Data: Pointer; width, height, BytesPerPixel: Integer);
begin
if not initialized then initializeAvatar;
if avt_show_raw_image(Data, width, height, BytesPerPixel)<>0 then Halt
end;

function ImageMaxWidth: Integer;
begin
ImageMaxWidth := avt_image_max_width
end;

function ImageMaxHeight: Integer;
begin
ImageMaxHeight := avt_image_max_height
end;

procedure getBackgroundColor(var red, green, blue: byte);
var r, g, b: Cint;
begin
avt_get_background_color(r, g, b);
red := r;
green := g;
blue := b
end;

procedure PagerString(const txt: string; startline: integer);
begin
if not initialized then initializeAvatar;
{ getting the string-length in pascal is lightweight }
{ converting to a CString would be more heavy }
avt_pager_mb(addr(txt[1]), length(txt), startline)
end;

procedure PagerFile(const filename: string; startline: integer);
var 
  f: file;
  buf: ^char;
  size, numread: LongInt;
begin
if not initialized then initializeAvatar;

assign(f, filename);
reset(f, 1);
size := FileSize(f);
if size > 0 then GetMem(buf, size);
BlockRead(f, buf^, size, numread);
close(f);

avt_pager_mb(buf, numread, startline);
if size > 0 then FreeMem(buf, size)
end;

function seconds(s: Real): integer;
begin seconds := trunc(s * 1000) end;

function AvatarGetError: ShortString;
begin
AvatarGetError := CString2String(avt_get_error)
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
HomePosition := avt_home_position
end;

procedure GotoXY(x, y: integer);
begin
avt_move_xy(x, y)
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
avt_set_balloon_width(width);
ScrSize.x := avt_get_max_x;
Window(1, 1, ScrSize.x, ScrSize.y);
end;

procedure BalloonHeight(height: integer);
begin
if not initialized then initializeAvatar;
avt_set_balloon_height(height);
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

function LoadRawSoundData(data:pointer; size: LongInt;
           samplingrate, audio_type, channels: integer): pointer;
begin
LoadRawSoundData := avt_load_raw_audio_data(data, size, samplingrate,
                     audio_type, channels)
end;

procedure AddRawSoundData(sound: pointer; data: pointer; size: LongInt);
begin
{ ignore error code }
avt_add_raw_audio_data(sound, data, size)
end;

function Playing(snd: pointer): boolean;
begin
Playing := avt_audio_playing(snd)
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
avt_play_audio(snd, loop)
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

avt_play_audio(GenSound, true)
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

procedure PauseSound(pause: boolean);
begin
avt_pause_audio(pause)
end;

{$IfDef FPC}

  procedure page(var f: text);
  begin
  Write(f, chr(12))
  end;
  
  procedure page;
  begin
  Write(output, chr(12))
  end;

{$EndIf} { FPC }

procedure CursorOff;
begin
avt_activate_cursor(false)
end;

procedure CursorOn;
begin
avt_activate_cursor(true)
end;

procedure KeyHandler(sym, modifiers, unicode: Cint); 
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
var result: Cint;
begin
if not initialized then initializeAvatar;
if avt_choice(result, start_line, items, Cint(startkey),
              back, fwrd)<>0 then Halt;
Choice := result
end;

procedure LockUpdates(lock: boolean);
begin
avt_lock_updates(lock)
end;

function Decide: boolean;
begin
if not initialized then initializeAvatar;
Decide := avt_decide;
if avt_get_status<>0 then Halt
end;

function Navigate(buttons: String): char;
var result: Cint;
begin
if not initialized then initializeAvatar;
result := avt_navigate(String2CString(buttons));
if avt_get_status<>0 then Halt;
Navigate := chr(result)
end;

{ ---------------------------------------------------------------------}
{ Input/output handling }
{ do not call Halt }

procedure AssignCrt(var f: text);
begin
AssignAvatar(f)
end;

{$IfDef FPC}

  function fpc_io_dummy(var F: TextRec): integer;
  begin
  fpc_io_dummy := 0
  end;

  function fpc_io_close(var F: TextRec): integer;
  begin
  F.Mode := fmClosed;
  fpc_io_close := 0
  end;

  function fpc_io_write(var F: TextRec): integer;
  begin
  if F.BufPos > 0 then
    begin
    if not initialized then initializeAvatar;

    if TextAttr<>OldTextAttr then UpdateTextAttr;

    avt_say_mb_len(F.BufPtr, F.BufPos);
    F.BufPos := 0; { everything read }
    end;

  fpc_io_write := 0
  end;

  function fpc_io_read(var F: TextRec): integer;
  begin
  if not initialized then initializeAvatar;
  if TextAttr<>OldTextAttr then UpdateTextAttr;

  avt_ask_mb(F.BufPtr, F.BufSize);

  F.BufPos := 0;
  F.BufEnd := strlen(F.BufPtr^) + 2;

  { sanity check }
  if F.BufEnd > F.BufSize then RunError(201);

  F.BufPtr^ [F.BufEnd-2] := #13;
  F.BufPtr^ [F.BufEnd-1] := #10;
  
  { clear KeyBoardBuffer }
  KeyboardBufferRead := KeyboardBufferWrite;
  
  fpc_io_read := 0
  end;

  function fpc_io_open(var F: TextRec): integer;
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
	 F.BufSize   := SizeOf(InputBuffer);
         end;

  F.BufPos := F.BufEnd;
  F.CloseFunc := @fpc_io_close;
  fpc_io_open := 0
  end;

  procedure AssignAvatar(var f: text);
  begin
  Assign(f, ''); { sets sane defaults }
  TextRec(f).OpenFunc := @fpc_io_open;
  end;
{$EndIf} { FPC }

{$IfDef __GPC__}

  function gpc_io_write(var unused; const Buffer; size: SizeType): SizeType;
  begin
  if size > 0 then
    begin
    if not initialized then initializeAvatar;
    if TextAttr<>OldTextAttr then UpdateTextAttr;

    avt_say_mb_len(Addr(Buffer), size)
    end;

  gpc_io_write := size
  end;

  function gpc_io_read(var unused; var Buffer; size: SizeType): SizeType;
  var 
    i: SizeType;
    CharBuf: array [0 .. size-1] of char absolute Buffer;
  begin
  if not initialized then initializeAvatar;
  if TextAttr<>OldTextAttr then UpdateTextAttr;

  avt_ask_mb(addr(InputBuffer), sizeof(InputBuffer));

  i := 0;
  while (InputBuffer [i] <> chr(0)) and (i < size-1) do
    begin
    CharBuf [i] := InputBuffer [i];
    inc(i)
    end;

  CharBuf [i] := #10;
  inc(i);
  
  { clear KeyBoardBuffer }
  KeyboardBufferRead := KeyboardBufferWrite;

  gpc_io_read := i
  end;

  procedure AssignAvatar(var f: text);
  begin
  AssignTFDD(f, NIL, NIL, NIL, 
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
  
  avt_reserve_single_keys(true); { Esc is handled in the KeyHandler }
  avt_register_keyhandler(@KeyHandler);

  { redirect i/o to Avatar }
  { do they have to be closed? Problems under Windows then }
  {Close(input);  Close(output);}
  AssignAvatar(input);
  AssignAvatar(output);
  Reset(input);
  Rewrite(output);



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
