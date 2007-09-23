{
This is just a stupid demo in (mostly) Standard Pascal,
to demonstrate, that it can be compiled with or
without an avatar with no changes to the source 
at all
}

{ This example is in the Public Domain }


program example (input, output);

const BeepSound = chr(7);

var name: string;

Begin

Write('What''s your name? '); 
ReadLn(name);

if name = '' { no name was given }
  then WriteLn (BeepSound, 'That was not nice. :-(')
  else WriteLn('The name "', name, '" is a nice name. :-)');

WriteLn;

{ This is to demonstrate, that all the compiler-magic in WriteLn
  also works with the avatar }
WriteLn ('By the way, pi is ', Pi:0:8, ' and so on...');

end.
