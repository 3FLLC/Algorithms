type
/////////////////////////////////////////////////////////////////////////////
// ma = Message Attribute
// na = Net Attribute
// in = Internet Attribute
// fa = Future Attribute
/////////////////////////////////////////////////////////////////////////////
   MessageHdrBits = (maDeleted {IMAP:DELETED}, maUnmoved, maIsNetMail, maIsPrivate,
      maReceived {IMAP:SEEN}, maIsEcho, maIsLocal, maDraft {IMAP:DRAFT}, naKillAfter, naSent,
      naFile, naCrash {IMAP:FLAGGED}, naReqRead, naAudit, naIsReceipt, naRecent {IMAP:RECENT},
      inKillAfter, inPublic, inSent, inReqRead, inIsReceipt, inBase64,
      inSQZ, inReserved, faReserved0, faReserved1, faReserved2, faReserved3,
      faReserved4, faReserved5, faReserved6, faReserved7);
/////////////////////////////////////////////////////////////////////////////
// Message Block Zero - is the first 8 bytes of ALL MSG files!
/////////////////////////////////////////////////////////////////////////////
   MessageBlockZero = Packed Record
      LoMsgNumber : LongWord;
      HiMsgNumber : LongWord;
   End;
   MessageIDX = Packed Record
      MsgNumber : LongWord;
      SeekToPos : LongWord;
   End;
   MessageHdr = Packed Record
      BlockSize    : Word;     // 2 bytes are stored but is size to read the
                               // rest of this header to EndSignature+5 mainly
                               // to make sure you read this comment!
{00}  MsgNumber    : LongWord;
{04}  Flags        : Set of MessageHdrBits;
{10}  CreatedOn    : TTimestamp;
{18}  CreatedBy    : LongWord; // RecordID from Userbase
{26}  ReceivedOn   : TTimestamp;
{34}  SentOn       : TTimestamp;
{42}  TimesRead    : LongWord;
{50}  BodyStart    : LongWord;
{58}  BodySize     : LongWord;
{59+} MsgFrom      : ShortString;
{n+}  MsgTo        : ShortString;
{n+}  MsgSubject   : ShortString;
{n+}  EndSignature : Array[0..4] of Char;
   End;

var
   LastArea:LongWord;
   AreaStr:String[5];
   ZeroRec:MessageBlockZero;
   IndexFH,BodyFH,HeaderFH:File;

procedure IsAreaDifferent(Area:LongWord;Var IndexFH,HeaderFH,BodyFH:File);
Var
   NW:Longint;

Begin
   If Area<>LastArea then begin
      If LastArea<>0 then Begin
         CloseFile(IndexFH);
         CloseFile(HeaderFH);
         CloseFile(BodyFH);
      End;
      LastArea:=Area;
      AreaStr:=IntToHex(Area,5);

      AssignFile(IndexFH, 'mbase/MSG'+AreaStr+'.IDX');
      AssignFile(HeaderFH, 'mbase/MSG'+AreaStr+'.HDR');
      AssignFile(BodyFH, 'mbase/MSG'+AreaStr+'.BDY');
      If FileExists('mbase/MSG'+AreaStr+'.IDX') then begin
         Reset(IndexFH, 1);
         BlockRead(IndexFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
         SeekFile(IndexFH, 0);
         Reset(HeaderFH, 1);
         SeekFile(HeaderFH, SizeOf(MessageBlockZero));
         Reset(BodyFH, 1);
         Inc(ZeroRec.HiMsgNumber);
      end
      else begin
         Rewrite(IndexFH, 1);
         ZeroRec.LoMsgNumber:=1;
         ZeroRec.HiMsgNumber:=1;
         Rewrite(HeaderFH, 1);
         BlockWrite(HeaderFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
         Rewrite(BodyFH, 1);
      end;
      BlockWrite(IndexFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
   end
   else Begin
      Inc(ZeroRec.HiMsgNumber);
      SeekFile(IndexFH, 0);
      BlockWrite(IndexFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
      SeekFile(HeaderFH, 0);
      BlockWrite(HeaderFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
   End;
   FlushFiles;
// At end of IDX header, wait for DAT size
   SeekFile(IndexFH, FileSize(IndexFH));
   SeekFile(HeaderFH, FileSize(HeaderFH));
   SeekFile(BodyFH, FileSize(BodyFH));
End;

procedure AppendMessage(Area:LongWord;Msg:MessageHdr;MsgKludge,MsgBody:String;var IndexFH,HeaderFH,BodyFH:File);
//AREAS: 0=Feedback to SysOp, $00001 to $FFFFF areas (1,048,576)
var
   Idx:MessageIdx;
   Nw,X:Longint;
   Ws:String;
   Len:Byte;

Begin
   IsAreaDifferent(Area,IndexFH,HeaderFH,BodyFH);

// Store Body Starting Point:
   Msg.BodyStart:=FileSize(BodyFH);

// store IDX:
   Idx.MsgNumber:=ZeroRec.HiMsgNumber;
   Idx.SeekToPos:=FileSize(HeaderFH);
   BlockWrite(IndexFH, Idx, SizeOf(Idx), Nw);

// Calculate BodySize and Store Actual Body:
   If inSQZ in Msg.Flags then begin
      LH6Compress(MsgKludge+#13#10+MsgBody, Ws);
      Msg.BodySize:=Length(Ws);
      BlockWrite(BodyFH, Ws[1], Msg.BodySize, Nw);
   end
   else begin
      Ws:=MsgKludge+#13#10+MsgBody;
      Msg.BodySize:=Length(Ws);
      BlockWrite(BodyFH, Ws[1], Msg.BodySize, Nw);
   End;

// Store Message Header:
   Msg.BlockSize:=SizeOf(Msg.MsgNumber)+
      SizeOf(Msg.Flags)+
      SizeOf(Msg.CreatedOn)+
      SizeOf(Msg.CreatedBy)+
      SizeOf(Msg.ReceivedOn)+
      SizeOf(Msg.SentOn)+
      SizeOf(Msg.TimesRead)+
      SizeOf(Msg.BodyStart)+
      SizeOf(Msg.BodySize)+
      Length(Msg.MsgFrom)+1+
      Length(Msg.MsgTo)+1+
      Length(Msg.MsgSubject)+1+
      SizeOf(Msg.EndSignature);
   Msg.MsgNumber:=ZeroRec.HiMsgNumber;
   Msg.EndSignature[0]:=#255;
   Msg.EndSignature[1]:=#254;
   Msg.EndSignature[2]:=#253;
   Msg.EndSignature[3]:=#252;
   Msg.EndSignature[4]:=#251;

   SetLength(Ws,Msg.BlockSize+2);
   X:=Msg.BlockSize-(Length(Msg.MsgFrom)+1+Length(Msg.MsgTo)+1+Length(Msg.MsgSubject)+1+2);
   Move(Msg,Ws[1],X);
   Len:=Length(Msg.MsgFrom);
   Move(Len,Ws[X],1);
   Move(Msg.MsgFrom[1],Ws[X+1],Len);
   Inc(X,Len+1);
   Len:=Length(Msg.MsgTo);
   Move(Len,Ws[X],1);
   Move(Msg.MsgTo[1],Ws[X+1],Len);
   Inc(X,Len+1);
   Len:=Length(Msg.MsgSubject);
   Move(Len,Ws[X],1);
   Move(Msg.MsgSubject[1],Ws[X+1],Len);
   Inc(X,Len+1);
   Move(Msg.EndSignature[0],Ws[x],5);
   BlockWrite(HeaderFH,Ws[1],Msg.BlockSize+2,Nw);
End;

Procedure OpenArea(Area:LongWord;var LoMsg,HiMsg:LongWord);
Var
   NW:Longint;

begin
   If Area<>LastArea then begin
      If LastArea<>0 then Begin
         CloseFile(IndexFH);
         CloseFile(HeaderFH);
         CloseFile(BodyFH);
      End;
      LastArea:=Area;
      AreaStr:=IntToHex(Area,5);

      AssignFile(IndexFH, 'mbase/MSG'+AreaStr+'.IDX');
      AssignFile(HeaderFH, 'mbase/MSG'+AreaStr+'.HDR');
      AssignFile(BodyFH, 'mbase/MSG'+AreaStr+'.BDY');

// Process Header and Move to first Msg Index:
      If FileExists('mbase/MSG'+AreaStr+'.IDX') then begin
         Reset(IndexFH, 1);
         BlockRead(IndexFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
         Reset(HeaderFH, 1);
         SeekFile(HeaderFH, SizeOf(MessageBlockZero));
         Reset(BodyFH, 1)
      end
      else begin
         Rewrite(IndexFH, 1);
         ZeroRec.LoMsgNumber:=1;
         ZeroRec.HiMsgNumber:=0;
         BlockWrite(IndexFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
         Rewrite(HeaderFH, 1);
         BlockWrite(HeaderFH, ZeroRec, SizeOf(MessageBlockZero), Nw);
         Rewrite(BodyFH, 1);
      end;
   End;
   LoMsg:=ZeroRec.LoMsgNumber;
   HiMsg:=ZeroRec.HiMsgNumber;
end;

function FetchHeader(Area,MsgNo:LongWord;var Header:MessageHdr):Boolean;
var
   lolw,hilw:LongWord;
   mi:MessageIDX;
   Nw:Longint;
   Blk:Word;
   Ws:String;
   B:Byte;
   Os:Word;

begin
   OpenArea(Area,lolw,hilw);
   If (MsgNo>=lolw) and (MsgNo<=hilw) then begin
      SeekFile(IndexFH, MsgNo*SizeOf(MessageIDX));
      BlockRead(IndexFH, Mi, SizeOf(MessageIDX), Nw);
      SeekFile(HeaderFH, Mi.SeekToPos);
      BlockRead(HeaderFH, Blk, SizeOf(Word), Nw);
      SetLength(Ws,Blk);
      BlockRead(HeaderFH, Ws[1], (Blk), Nw);
      With Header do begin
         BlockSize:=Blk;

         Move(Ws[1],MsgNumber,4);
         Move(Ws[5],Flags,4);
         Move(Ws[9],CreatedOn,8);
         Move(Ws[17],CreatedBy,4);
         Move(Ws[21],ReceivedOn,8);
         Move(Ws[29],SentOn,4);
         Move(Ws[37],TimesRead,4); // Update+1
         Move(Ws[41],BodyStart,4);
         Move(Ws[45],BodySize,4);
         Move(Ws[49],B,1);
         SetLength(MsgFrom,B);
         Move(Ws[50],MsgFrom[1],B);
         Os:=50+B;
         Move(Ws[Os],B,1);
         SetLength(MsgTo,B);
         Move(Ws[Os+1],MsgTo[1],B);
         Inc(Os,B+1);
         Move(Ws[Os],B,1);
         SetLength(MsgSubject,B);
         Move(Ws[Os+1],MsgSubject[1],B);
         // Last 5 Bytes are for Crash Recovery //
      End;
      Result:=True;
   end
   else Result:=false;
end;

function FetchMessage(Area,MsgNo:LongWord;var Header:MessageHdr;var Body:String):Boolean;
var
   lolw,hilw:LongWord;
   mi:MessageIDX;
   Nw:Longint;
   Blk:Word;
   Ws:String;
   B:Byte;
   Os:Word;

begin
   OpenArea(Area,lolw,hilw);
   If (MsgNo>=lolw) and (MsgNo<=hilw) then begin
      SeekFile(IndexFH, MsgNo*SizeOf(MessageIDX));
      BlockRead(IndexFH, Mi, SizeOf(MessageIDX), Nw);
//Writeln('Header at ',Mi.SeekToPos);
      SeekFile(HeaderFH, Mi.SeekToPos);
      BlockRead(HeaderFH, Blk, SizeOf(Word), Nw);
//Writeln('Block: ',Blk);
      SetLength(Ws,Blk);
      BlockRead(HeaderFH, Ws[1], (Blk), Nw);
//Writeln(StrToHex(Ws));
      // 1:01000000
      // 5:00004000
      // 9:5184875800000000
      //17:00000000
      //21:5184875800000000
      //29:FFFFFFFFFFFFFFFF
      //37:00000000
      //41:00000000
      //45:87020000
      //49:0A 6D61726B206C65776973
      //   0D 4D617572696365204B696E616C
      //   0D 54657374696E6720616761696E
      //FFFEFDFCFB

      // 0A000000
      // 00004000
      // 21B8875800000000
      // 00000000
      // 21B8875800000000
      // FFFFFFFFFFFFFFFF
      // 00000000
      // 911C0000
      // 87020000
      // 0D 4D617572696365204B696E616C
      // 13 4361726F6C205368656E6B656E626572676572
      // 11 52653A2054657374696E6720616761696E
      // FFFEFDFCFB

      With Header do begin
         BlockSize:=Blk;

         Move(Ws[1],MsgNumber,4);
         Move(Ws[5],Flags,4);
         Move(Ws[9],CreatedOn,8);
         Move(Ws[17],CreatedBy,4);
         Move(Ws[21],ReceivedOn,8);
         Move(Ws[29],SentOn,4);
         Move(Ws[37],TimesRead,4); // Update+1
         Move(Ws[41],BodyStart,4);
         Move(Ws[45],BodySize,4);
         Move(Ws[49],B,1);
         SetLength(MsgFrom,B);
         Move(Ws[50],MsgFrom[1],B);
         Os:=50+B;
         Move(Ws[Os],B,1);
         SetLength(MsgTo,B);
         Move(Ws[Os+1],MsgTo[1],B);
         Inc(Os,B+1);
         Move(Ws[Os],B,1);
         SetLength(MsgSubject,B);
         Move(Ws[Os+1],MsgSubject[1],B);
         // Last 5 Bytes are for Crash Recovery //
      End;
//Writeln('Start: ',Header.BodyStart,', Size: ',Header.BodySize);

      SeekFile(BodyFH, Header.BodyStart);
      SetLength(Ws,Header.BodySize);

      BlockRead(BodyFH, Ws[1], Header.BodySize, Nw);
      If inSQZ in Header.Flags then LH6DeCompress(Ws, Body)
      Else Body:=Ws;
      Result:=True;
   end
   else Result:=false;
end;

Procedure OpenMessageBase;
Begin
   LastArea:=0;
End;

Procedure CloseMessageBase;
Begin
   If LastArea>0 then begin
      CloseFile(BodyFH);
      CloseFile(HeaderFH);
      CloseFile(IndexFH);
   End;
End;
