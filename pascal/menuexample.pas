{ Switch FPC to mode TP or Delphi to get the same behaviour as with GPC }
{$IfDef FPC}
{$Mode TP}
{$EndIf}

program menuexample;
uses AKFAvatar;

procedure MenuEntry(Nr: Integer);
begin
  case Nr of
    1: Write('Pizza');
    2: Write('Spaghetti');
    3: Write('Pommes');
    4: Write('Hamburger');
    5: Write('Spinach');
    end
end;


var food: Integer;

begin
WriteLn ('What''s for dinner?');
food := Menu(5, MenuEntry);

ClrScr;

case food of
  1..4: Tell ('Hmmm!');
  else Tell ('Yuck!');
  end
end.
