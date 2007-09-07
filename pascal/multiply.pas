{
 * simple multiplication
 *
 * This example shows how to write programs, that use features
 * of the Avatar library, but can still be compiled without it.
 *
 * use "gpcavatar" or "fpcavatar" to compile it with AKFAvatar
 * or compile it directly with gpc or fpc to get a console program
 *
 * Copyright (c) 2007 Andreas K. Foerster <info@akfoerster.de>
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


{$IfNdef deutsch}
  {$Define english}
{$EndIf}


program multiply;

{ use CRT if it is not compiled for AKFAvatar: }
{$IfNDef AKFAVATAR}
  uses CRT;
{$EndIf}

const
  a_minimum = 1;
  a_maximum = 10;
  b_minimum = 1;
  b_maximum = 10;

{$IfDef english}
  const question = 'exercise 1) multiplication or 2) division?';
  const
    correct = 'correct';
    wrong = 'wrong';
{$EndIf}

{$IfDef deutsch}
  const question = '1) multiplizieren oder 2) dividieren '#252'ben?';
  const
    correct = 'richtig';
    wrong = 'falsch';
{$EndIf}

{$IfDef AKFAVATAR}
  const multiplicationSign = chr($B7); { middle dot }
{$Else} 
  const multiplicationSign = '*';
{$EndIf}

const divisionSign = '/';

var endRequest: boolean;
var exercise: (multiplication, division);

var 
  correctsnd: Pointer = NIL;
  wrongsnd: Pointer = NIL;

function GetRandomNumber(minimum, maximum: integer): integer;
begin
GetRandomNumber := Random(maximum-minimum+1) + minimum
end;

function askResult: integer;
var line: string[255];
var result, code: integer;
begin
ReadLn(line);

if line = '' then endRequest := true;

val (line, result, code);
if code=0
  then askResult := result
  else askResult := -1 { error }
end;

procedure AskWhatToExercise;
var c: char;
begin
WriteLn(question);

repeat
  c := ReadKey
until (c='1') or (c='2');

case c of
  '1' : exercise := multiplication;
  '2' : exercise := division;
  end;

ClrScr
end;

procedure query;
var a, b, r, e: integer;
var counter: integer;
var isCorrect: boolean;
begin
counter := 0;

repeat
  inc(counter);
  a := GetRandomNumber(a_minimum, a_maximum);
  b := GetRandomNumber(b_minimum, b_maximum);
  r := a * b;

  repeat
    Write('':5); { some space at the beginning }
    Write(counter:3, ') ');
    
    case exercise of
    multiplication: 
      begin
      Write(a, multiplicationSign, b, '=');
      e := askResult;
      isCorrect := e = r
      end;
    division : 
      begin
      Write(r, divisionSign, a, '=');
      e := askResult;
      isCorrect := e = b
      end
    end;
    
    if not endRequest and (e<>-1) then
      begin
      GotoXY(30, WhereY-1);
      if isCorrect
        then begin 
	     {$IfDef AKFAVATAR} PlaySound(correctsnd); {$EndIf}
	     TextColor(green);
	     WriteLn(correct) 
	     end
        else begin 
	     {$IfDef AKFAVATAR} PlaySound(wrongsnd); {$EndIf}
	     TextColor(red); 
	     WriteLn(wrong) 
	     end;
      NormVideo
      end
  until isCorrect or endRequest
until endRequest
end;


Begin { main program }

{ use IfDef for AKFAVATAR specific commands: }
{$IfDef AKFAVATAR}
  SetEncoding('ISO-8859-1');
  SetBackgroundColor($BB, $BB, $55);

  { load the image of the teacher, if there is one }
  AvatarImageFile('teacher.bmp');
  
  correctsnd := LoadSoundFile('correct.wav');
  wrongsnd := LoadSoundFile('wrong.wav');
{$EndIf}

{ This is either defined in AKFAvatar or in CRT }
ClrScr;

endRequest := false;
Randomize;

AskWhatToExercise;
query;

{$IfDef AKFAVATAR}
  FreeSound(correctsnd);
  FreeSound(wrongsnd);
  
  { Avoid waiting for a keypress }
  MoveAvatarOut;
{$EndIf}
end.
