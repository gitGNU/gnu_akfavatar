{*
 * simple multiplication
 * Copyright (c) 2007,2008,2012,2013 Andreas K. Foerster <info@akfoerster.de>
 *
 * This example shows how to write programs, that use features of the 
 * AKFAvatar library.
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
{$IfNDef AKFAVATAR}
  uses AKFAvatar;
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

const NormalColor = White;

{$IfDef english}
  const question = 'What to exercise?';
  const q_multiplication = 'multication';
  const q_multiples_of = 'multiples of';
  const q_division = 'division';
  const q_division_by = 'division by';
  const correct = 'correct';
  const wrong = 'wrong';
  const q_continue = 'Do you want to continue?';
{$EndIf}

{$IfDef deutsch}
  const question = 'Was '#252'ben?';
  const q_multiplication = 'multiplizieren';
  const q_multiples_of = 'Vielfache von';
  const q_division = 'dividieren';
  const q_division_by = 'dividieren durch';
  const correct = 'richtig';
  const wrong = 'falsch';
  const q_continue = 'Willst du weitermachen?';
{$EndIf}

const multiplicationSign = chr($B7); { middle dot }
const divisionSign = ':';

type Texercise = (multiplication, division);

var endRequest: boolean;
var exercise: Texercise;
var SpecificTable: Integer;

var 
  correctsnd: Pointer = NIL;
  wrongsnd: Pointer = NIL;

function GetRandomNumber(minimum, maximum: integer): integer;
begin
GetRandomNumber := Random(maximum-minimum+1) + minimum
end;

function askNumber: integer;
var line: string[255];
var result, code: integer;
begin
ReadLn(line);

if line = '' then endRequest := true;

val(line, result, code);
if code=0
  then askNumber := result
  else askNumber := -1 { error }
end;

procedure AskWhichTable(ex: Texercise);
begin
exercise := ex;

repeat
  ClrScr;
  if exercise = multiplication
    then Write(q_multiples_of, ' ')
    else Write(q_division_by, ' ');

  SpecificTable := askNumber;
until SpecificTable > 0
end;

procedure AskWhatToExercise;
begin
ClrScr;
setTextDelay(0);
SpecificTable := 0;

WriteLn(question);
WriteLn;
WriteLn('1) ', q_multiplication);
WriteLn('2) ', q_multiples_of, ' ...');
WriteLn('3) '+ q_division);
Write  ('4) '+ q_division_by, ' ...');

case Choice(3, 4, '1', false, false) of
  1: exercise := multiplication;
  2: AskWhichTable(multiplication);
  3: exercise := division;
  4: AskWhichTable(division)
  end;

setTextDelay(DefaultTextDelay);
ClrScr
end;

procedure sayCorrect;
begin 
PlaySound(correctsnd, AVT_PLAY);
TextColor(LightGreen);
WriteLn(correct);
TextColor(NormalColor)
end;

procedure sayWrong;
begin 
PlaySound(wrongsnd, AVT_PLAY);
TextColor(LightRed);
WriteLn(wrong);
TextColor(NormalColor)
end;

procedure query;
var a, b, r, e: integer;
var counter, tries: integer;
var isCorrect: boolean;
begin
counter := 0;

repeat
  inc(counter);
  if SpecificTable <= 0 then
    a := GetRandomNumber(a_minimum, a_maximum)
  else
    a := SpecificTable;

  b := GetRandomNumber(b_minimum, b_maximum);
  r := a * b;

  tries := 0;

  repeat
    Write(counter:3, ') ');

    case exercise of
    multiplication: 
      begin
      Write(a, multiplicationSign, b, '=');
      e := askNumber;
      isCorrect := e = r
      end;
    division : 
      begin
      Write(r, divisionSign, a, '=');
      e := askNumber;
      isCorrect := e = b
      end
    end;
    
    if not endRequest and (e<>-1) then
      begin
      inc(tries);
      GotoXY(30, WhereY-1);
      if isCorrect then sayCorrect
        else begin
             sayWrong;

             if tries >= 2 then { help him }
               begin
               HighVideo;
               TextBackground(LightRed);
               TextColor(Black);
               Write(counter:3, ') ');
               if exercise = multiplication then
                 Write(a, multiplicationSign, b, '=', r, '  ')
               else
                 Write(r, divisionSign, a, '=', b, '  ');
               NormVideo;
               TextColor(NormalColor);
               WriteLn;
               isCorrect := true; { the teacher is always rght ;-) }
               end
             end
      end
  until isCorrect or endRequest
until endRequest
end;

function WantToContinue: boolean;
begin
ClrScr;
Write(q_continue);
WantToContinue := Decide;
end;

Begin { main program }
SetEncoding('ISO-8859-1');
SetBackgroundColorName('tan');

{ load the image of the teacher }
AvatarImageXPM(Addr(teacher));

{ or use this to load the image from a file: }
{ AvatarImageFile('teacher.xpm'); }

correctsnd := LoadSoundData(Addr(snd_positive), sizeof(snd_positive), AVT_LOAD);
wrongsnd   := LoadSoundData(Addr(snd_negative), sizeof(snd_negative), AVT_LOAD);

Randomize;

setAvatarMode(footer);
MoveAvatarIn;

setBalloonColor(0, $55, 0);
TextColor(NormalColor);
Window(20, 1, 20+40, ScreenSize.y);

repeat
  endRequest := false;
  AskWhatToExercise;
  query;
until not WantToContinue;

FreeSound(correctsnd);
FreeSound(wrongsnd);

{ Avoid waiting for a keypress }
MoveAvatarOut;
end.
