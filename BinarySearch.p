program BinarySearch;

uses
   Strings;

type
   HashKey=String;
   DataStruct=Record
      Key:HashKey;
      Index:Longint;
   End;

var
   Ws,LastWs:HashKey;
   Data:Array of DataStruct;
   StrList:TStringList;
   Ctr:Longint;

function Search(Key:HashKey):Longint;
var
   H,J,L:Longint;

Begin
   L:=0;
   H:=High(Data);
   While H-L>1 do begin
      J:=(H+L) div 2;
      if key<=Data[J].Key then H:=J
      else L:=J;
   End;
   if Data[H].Key=Key then Result:=H
   else Result:=-1;
end;

Begin
   StrList.Init;
   StrList.LoadFromFile('../linux.words');
   StrList.Sort(); // sorted so you can test - but see sort algorithms for your onw usage
   Writeln('Loaded ',StrList.getCount,' words');
//--- Populate with unsorted keys:
   SetLength(Data, StrList.getCount-1);
   LastWs:='';
   Ctr:=-1;
   For var Idx:=0 to StrList.getCount-1 do begin
      Ws:=Lowercase(StrList.getStrings(Idx));
      if (LastWs<>Ws) then begin
         Inc(Ctr);
         Data[Idx].Key:=Ws;
         Data[Idx].Index:=Idx;
      End;
      LastWs:=Ws;
   End;
   SetLength(Data, Ctr);
   Writeln('There are ',Ctr,' unique words');
//--- See our Search Algorithms!!
//   SortData();
//---
   Writeln(Search('apple'));
   Writeln(Search('elf'));
   Writeln(Search('sprite'));
   Writeln(Search('troll'));
   Writeln(Search('1080'));
   Writeln(Search('zzz'));
   StrList.Free;
End.
