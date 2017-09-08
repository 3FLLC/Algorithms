Program Fido.MsgID.v1170908;

(* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Produces a unique 8 digit serial number, used in MSGID   *
* contains TZ+OS+6 DIGIT YEAR,DOY,HH,NN,SS over a goofy    *
* base 36 character array that has been modified to remove *
* letters and numbers that could be confused based upon    *
* the font used to render.                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *)

uses Datetime, Math;

Function SerialNo:String;
Const
   Numbering='1234567890QWERTYUIOPASDFGHJKLZXCVBNM';
   Pass7=78364164096;
   Pass6=2176782336;
   Pass5=60466176;
   Pass4=1679616;
   Pass3=46656;
   Pass2=1296;
   Pass1=36;
{$IFDEF WINDOWS}
   OSValue=1;
{$ENDIF}
{$IFDEF LINUX}
   OSValue=2;
{$ENDIF}
{$IFDEF MAC}
   OSValue=4;
{$ENDIF}

var
   phase1:longword;
   rslt:longword;

begin
   If GetLocalTimeOffset<0 then begin
      Phase1:=(abs(GetLocalTimeOffset) div 60);
   end
   else begin
      Phase1:=(GetLocalTimeOffset div 60)+12;
   End;
   Result:=Numbering[Phase1]+Numbering[OSValue];
   Phase1:=(((GetYears(Timestamp)-2000)*DayOfYear(Timestamp)*
      (GetHours(Timestamp)*24)*(GetMinutes(Timestamp)*60))+
      GetSeconds(Timestamp));
   Rslt:=Trunc(phase1 / Pass5);
   Result+=Numbering[Rslt+1];
   Phase1-=Rslt*Pass5;
   Rslt:=Trunc(phase1 / Pass4);
   Result+=Numbering[Rslt+1];
   Phase1-=Rslt*Pass4;
   Rslt:=Trunc(phase1 / Pass3);
   Result+=Numbering[Rslt+1];
   Phase1-=Rslt*Pass3;
   Rslt:=Trunc(phase1 / Pass2);
   Result+=Numbering[Rslt+1];
   Phase1-=Rslt*Pass2;
   Rslt:=Trunc(phase1 / Pass1);
   Result+=Numbering[Rslt+1];
   Phase1-=Rslt*Pass1;
   Result+=Numbering[Phase1+1];
end;

Begin
   Writeln('MSGID: '+SerialNo);
end.
