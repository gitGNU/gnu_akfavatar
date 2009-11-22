{*
 * simple multiplication
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
 *
 * This example shows how to write programs, that use features
 * of the Avatar library, but can still be compiled without it.
 *
 * use "gpcavatar" or "fpcavatar" to compile it with AKFAvatar
 * or compile it directly with gpc or fpc to get a console program
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

{ include image of the teacher (made with xpm2pas) }
{$I teacher.inc}

{ include sound data (made with bin2obj from FreePascal) }
{$I positive.inc}
{$I negative.inc}

const
  a_minimum = 1;
  a_maximum = 10;
  b_minimum = 1;
  b_maximum = 10;

{$IfDef english}
  const question = 'What to exercise?';
  const q_multiplication = 'multication';
  const q_division = 'division';
  const
    correct = 'correct';
    wrong = 'wrong';
  const q_continue = 'Do you want to continue?';
{$EndIf}

{$IfDef deutsch}
  const question = 'Was '#252'ben?';
  const q_multiplication = 'multiplizieren';
  const q_division = 'dividieren';
  const
    correct = 'richtig';
    wrong = 'falsch';
  const q_continue = 'Willst du weitermachen?';
{$EndIf}

{$IfDef AKFAVATAR}
  const multiplicationSign = chr($B7); { middle dot }
{$Else} 
  const multiplicationSign = '*';
{$EndIf}

const divisionSign = ':';

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

{$IfDef AKFAVATAR}

procedure AskWhatToExercise;
begin
BalloonSize (3, 40);

ClrScr;
WriteLn(question);
WriteLn('1) '+ q_multiplication);
Write  ('2) '+ q_division);
case Choice(2, 2, '1', false, false) of
  1: exercise := multiplication;
  2: exercise := division
  end;

ClrScr
end;

{$Else}

procedure AskWhatToExercise;
var c: char;
begin
ClrScr;
WriteLn(question);
WriteLn('1) '+ q_multiplication);
WriteLn('2) '+ q_division);

repeat
  c := ReadKey
until (c='1') or (c='2');

case c of
  '1' : exercise := multiplication;
  '2' : exercise := division;
  end;

ClrScr
end;

{$EndIf}

procedure sayCorrect;
begin 
{$IfDef AKFAVATAR} 
  PlaySound(correctsnd, false);
{$EndIf}

TextColor(green);
WriteLn(correct) 
end;

procedure sayWrong;
begin 
{$IfDef AKFAVATAR}
  PlaySound(wrongsnd, false);
{$EndIf}

TextColor(red); 
WriteLn(wrong) 
end;

procedure query;
var a, b, r, e: integer;
var counter: integer;
var isCorrect: boolean;
begin
{$IfDef AKFAVATAR}
  BalloonSize (4, 40);
{$EndIf}

counter := 0;

repeat
  inc(counter);
  a := GetRandomNumber(a_minimum, a_maximum);
  b := GetRandomNumber(b_minimum, b_maximum);
  r := a * b;

  repeat
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
      if isCorrect then sayCorrect else sayWrong;
      NormVideo
      end
  until isCorrect or endRequest
until endRequest
end;

function WantToContinue: boolean;
begin
{$IfDef AKFAVATAR}
  BalloonSize (1, 40);
  ClrScr;
  Write(q_continue);

  WantToContinue := Decide;
{$else}
  WantToContinue := false;
{$EndIf}
end;

Begin { main program }

{ use IfDef for AKFAVATAR specific commands: }
{$IfDef AKFAVATAR}
  SetEncoding('ISO-8859-1');
  SetBackgroundColorName('tan');

  { load the image of the teacher }
  AvatarImageXPM(Addr(teacher));

  { or use this to load the image from a file: }
  { AvatarImageFile('teacher.xpm'); }

  correctsnd := LoadSoundData(Addr(snd_positive), sizeof(snd_positive));
  wrongsnd   := LoadSoundData(Addr(snd_negative), sizeof(snd_negative));
{$EndIf}

Randomize;

repeat
  endRequest := false;
  AskWhatToExercise;
  query;
until not WantToContinue;

{$IfDef AKFAVATAR}
  FreeSound(correctsnd);
  FreeSound(wrongsnd);

  { Avoid waiting for a keypress }
  MoveAvatarOut;
{$EndIf}
end.
