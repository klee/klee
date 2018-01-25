//---------------------------------------------------------------------------

#include <shlobj.h>

#define NO_WIN32_LEAN_AND_MEAN 1
#include <vcl.h>
#pragma hdrstop

#include "USettings.h"
#include "Umain.h"
#include "UEditor.h"
#include "windows.h"
#include "SysUtils.hpp"

#include <stdio.h>
#define countof( array ) ( sizeof( array )/sizeof( array[0] ) )

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmSettings *frmSettings;

#define BUFSIZE 4096 
 
HANDLE hChildStdinRd, hChildStdinWr,  
   hChildStdoutRd, hChildStdoutWr, 
   hInputFile, hStdout;
bool preprocflag=false;

BOOL CreateChildProcess();
VOID WriteToPipe(VOID);
VOID ReadFromPipe(VOID);
VOID ErrorExit(LPSTR);
bool preproc();


//---------------------------------------------------------------------------
__fastcall TfrmSettings::TfrmSettings(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
int TfrmSettings::Init(bool silent)
{

        int Engine;
        AnsiString OutputFile;
        int OutputType;
        bool Preview;
        AnsiString Options;

        TfrmEditor* Ed=((TfrmEditor*)frmMain->ActiveMDIChild);
   if(Ed)
   {

            ComboBox2Change(NULL);
        if(!Ed->isEditor) //if preview window is highlighted make teditor the active
        {
                Ed->Editor->Show();
                Ed=((TfrmEditor*)frmMain->ActiveMDIChild);
        }



        if((Ed) && (Ed->isEditor))
        {
                Edit1->Text=ChangeFileExt(Ed->FileName,"."+ComboBox2->Items->Strings[ComboBox2->ItemIndex]);
                //load editor window values
                if (Ed->Engine != -1)
                {
                        ComboBox1->ItemIndex=Ed->Engine;
                        Edit1->Text=Ed->OutputFile;
                        ComboBox2->ItemIndex=Ed->OutputType;
                        CheckBox1->Checked=Ed->Preview;
                        Memo1->Text=Ed->Options;
                }
                if(!silent)
                        return ShowModal();

                else
                {
                        if (Ed->Engine != -1)
                                Button3->Click();
                        else
                                return ShowModal();
                }

        }
    }
    return -1;
}

void __fastcall TfrmSettings::Button6Click(TObject *Sender)
{
                AnsiString FileName="http://www.graphviz.org/doc/info/attrs.html";
                AnsiString action="open";
                ShellExecute(NULL, action.c_str(),FileName.c_str(), NULL, NULL, SW_SHOW); //read me file, if exists it shows


}
//---------------------------------------------------------------------------

void __fastcall TfrmSettings::Button2Click(TObject *Sender)
{
        AnsiString Scope[]={"G","N","E"};
        if(Edit3->Text.Trim()!="")
        {
                Memo1->Lines->Add("-"+Scope[ComboBox3->ItemIndex]+ComboBox4->Items->Strings[ComboBox4->ItemIndex]+"=\""+Edit3->Text+"\"");

        }
        else
        {
                ShowMessage ("You need to specify a value");
                Edit3->SetFocus();
        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmSettings::BitBtn2Click(TObject *Sender)
{
        if(Memo1->Text.Trim() !="")
        {
                int a=MessageDlg("Are you sure that you want to clear attributes?",mtInformation, TMsgDlgButtons() << mbYes<<mbNo << mbCancel, 0);
                if(a==mrYes)
                {
                        Memo1->Lines->Clear();
                }
        }

}
//---------------------------------------------------------------------------
AnsiString getIniFile()
{
        char bf[5012];
        SHGetSpecialFolderPath(NULL,bf,CSIDL_LOCAL_APPDATA,1);
        AnsiString rv=AnsiString(bf);
        rv=rv+"\\Settings.ini";
        if(!FileExists(rv))/*create content of the ini file as first time*/
        {
                TStringList* sl=new TStringList();
                sl->Add("[Settings]");
                sl->Add("Layout=0");
                sl->Add("Output=6");
                sl->Add("Preview=1");
                sl->Add("InitialDir1=C:\\");
                sl->Add("InitialDir2=C:\\");
                sl->Add("InitialDir3=C:\\");
                sl->Add("binPath=C:\\Program Files\\Graphviz2.26\\bin\\");
                sl->Add("init=1");
                sl->SaveToFile(rv);
        }
        return rv;
}

void __fastcall TfrmSettings::FormCreate(TObject *Sender)
{
        LoadAttrs(ComboBox1->Items->Strings[ComboBox1->ItemIndex],ComboBox3->Items->Strings[ComboBox3->ItemIndex]);
        //load Graphviz bin Folder

        AnsiString binPath;
        AnsiString FileName=getIniFile();
        TIniFile *ini;
        ini = new TIniFile(FileName);
        preprocflag=false;
        if(FileExists(FileName))
                binPath=ini->ReadString( "Settings", "binPath", "");
        if(!FileExists(binPath+"\\dot.exe"))
        {
                binPath=frmMain->ReadFromRegistry("\\SOFTWARE\\ATT\\GRAPHVIZ","InstallPath")+"\\bin";
                if(!FileExists(binPath+"\\dot.exe"))
                {
                        if(!FileExists(ExtractFilePath(Application->ExeName) + "\\dot.exe"))
                        {
                                ShowMessage ("Gvedit could not locate dot.exe.please modify graphviz bin folder manually");
                        }
                        else
                                Edit2->Text=ExtractFilePath(Application->ExeName);
                }
                else
                        Edit2->Text=binPath;
        }
        else
                Edit2->Text=binPath;


        //get default settings
        preprocflag=false;
        if(FileExists(FileName))
        {
                ComboBox1->ItemIndex=ini->ReadInteger( "Settings", "Layout", 0);
                ComboBox2->ItemIndex=ini->ReadInteger( "Settings", "Output", 0);
                CheckBox1->Checked=(ini->ReadInteger( "Settings", "Preview", 0) > 0);
                SD1->InitialDir=ini->ReadString( "Settings", "InitialDir1", "");
                SD2->InitialDir=ini->ReadString( "Settings", "InitialDir2", "");
                OD1->InitialDir=ini->ReadString( "Settings", "InitialDir3", "");
        }
        delete ini;
}
//---------------------------------------------------------------------------


void __fastcall TfrmSettings::ComboBox3Change(TObject *Sender)
{
        LoadAttrs(ComboBox1->Items->Strings[ComboBox1->ItemIndex],ComboBox3->Items->Strings[ComboBox3->ItemIndex]);
}
//---------------------------------------------------------------------------

void __fastcall TfrmSettings::Button1Click(TObject *Sender)
{
        //set extensions for save dialog
       // JPG|*.jpg|JPEG|*.jpeg
        SD1->Filter=ComboBox2->Items->Strings[ComboBox2->ItemIndex]+" Files |*."+ComboBox2->Items->Strings[ComboBox2->ItemIndex]+"|All Files|*.*";
        if(frmMain->ActiveMDIChild)
                SD1->FileName=ChangeFileExt(((TfrmEditor*)frmMain->ActiveMDIChild)->FileName,"."+ComboBox2->Items->Strings[ComboBox2->ItemIndex]);
        if(SD1->Execute())
        {
                Edit1->Text=SD1->FileName;

        }
}
//---------------------------------------------------------------------------


void __fastcall TfrmSettings::ComboBox2Change(TObject *Sender)
{
        AnsiString FileExtension;
        if(frmMain->ActiveMDIChild)
        {
                FileExtension="."+ComboBox2->Items->Strings[ComboBox2->ItemIndex];
                if(FileExtension.UpperCase().Trim()==".XDOT")
                        FileExtension=".dot";
                if(FileExtension.UpperCase().Trim()==".PS2")
                        FileExtension=".ps";
                Edit1->Text=ChangeFileExt(Edit1->Text,FileExtension);
        }
        //set cairo check box ,enable disable according to file type
        //cairo supports
        if(
        (ComboBox2->Items->Strings[ComboBox2->ItemIndex].UpperCase()=="PS")
                        ||
        (ComboBox2->Items->Strings[ComboBox2->ItemIndex].UpperCase()=="PS2")
                        ||
        (ComboBox2->Items->Strings[ComboBox2->ItemIndex].UpperCase()=="PNG")
                        ||
        (ComboBox2->Items->Strings[ComboBox2->ItemIndex].UpperCase()=="PDF")
                        ||
        (ComboBox2->Items->Strings[ComboBox2->ItemIndex].UpperCase()=="SVG") )
        {
                CheckBox2->Enabled=true;

        }
        else
                CheckBox2->Enabled=false;


}
//---------------------------------------------------------------------------

void __fastcall TfrmSettings::BitBtn1Click(TObject *Sender)
{
        SD1->Filter="";
        if(SD1->Execute())
        {
                try
                {
                        Memo1->Lines->SaveToFile(SD1->FileName);
                }
                catch(...)
                {
                        ShowMessage ("File could not be saved! , make sure that disk is not read only");
                }

        }

}
//---------------------------------------------------------------------------

void __fastcall TfrmSettings::BitBtn3Click(TObject *Sender)
{
        if(OD1->Execute())
        {
                try
                {
                        Memo1->Lines->LoadFromFile(OD1->FileName);
                }
                catch(...)
                {
                        ShowMessage ("File could not be opened!");
                }
        }
}
//---------------------------------------------------------------------------

bool runproc(AnsiString szExeName,AnsiString szCommandLine,AnsiString tempFile,AnsiString engine,AnsiString outFile,bool silent)
{
/*        AnsiString tempfile="";
        if (!preprocflag)
                tempfile="\""+ExtractFilePath(Application->ExeName)+"__temp.dot"+"\""; //to replace with real name in error messages
        else
                tempfile="\""+ExtractFilePath(Application->ExeName)+"__temp2.dot"+"\""; //to replace with real name in error messages*/

        szCommandLine =szCommandLine+" "+tempFile;
        AnsiString szTempFile="tempfile.txt";
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
        sa.bInheritHandle = TRUE;
        //creating pipe
        HANDLE hStdoutRd, hStdoutWr;
        if ( !CreatePipe ( &hStdoutRd, &hStdoutWr, &sa, 0 ))
                return 0;

        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi;
        si.dwFlags = STARTF_USESTDHANDLES+STARTF_USESHOWWINDOW;
        si.hStdError = hStdoutWr;
        si.wShowWindow=SW_HIDE ;
        if ( !CreateProcess ( szExeName.c_str(), szCommandLine.c_str(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi ))
                return 0;

        CloseHandle ( hStdoutWr );
        //create temp file
        if (!silent)     //for preview we dont need stdioerr
                return 0;
        HANDLE hTempFile;
        hTempFile = CreateFile ( szTempFile.c_str(), GENERIC_WRITE, 0, NULL,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( INVALID_HANDLE_VALUE == hTempFile )
                return 0;
        BYTE buff[1024];
        DWORD dwRead, dwWritten;
        //write to temp file
        while ( ReadFile ( hStdoutRd, buff, countof(buff), &dwRead, NULL ) && dwRead != 0 )
                WriteFile ( hTempFile, buff, dwRead, &dwWritten, NULL );

        CloseHandle(hStdoutRd);
        CloseHandle(hTempFile);
        //now read the ouptput and delete the temp file
        if (FileExists(szTempFile))
        {
                TStringList* tmpString=new TStringList();
                tmpString->LoadFromFile(szTempFile);
                if(tmpString->Text.Trim() !="")
                {
                        tmpString->Text=StringReplace(tmpString->Text,tempFile,((TfrmEditor*)frmMain->ActiveMDIChild)->FileName,TReplaceFlags () << rfReplaceAll);
                        frmMain->Memo1->Text=frmMain->Memo1->Text+tmpString->Text;
                        frmMain->Memo1->SelStart=frmMain->Memo1->Text.Length()-1;
                        SendMessage(frmMain->Memo1->Handle, EM_SCROLLCARET,0,0);
                        DeleteFile(szTempFile);

                }
                else
                {
                        frmMain->Memo1->Lines->Add(engine+" has created "+outFile+" sucessfully.");
                        frmMain->Memo1->SelStart=frmMain->Memo1->Text.Length()-1;
                        SendMessage(frmMain->Memo1->Handle, EM_SCROLLCARET,0,0);
                        DeleteFile(szTempFile);
                }
                if (preprocflag)
                        DeleteFile(tempFile);
                   delete tmpString;
        }
        return true;
}

void __fastcall TfrmSettings::Button3Click(TObject *Sender)
{
/*
WIN32 API for getting temporary file folder

DWORD WINAPI GetTempPath(
  __in   DWORD nBufferLength,
  __out  LPTSTR lpBuffer

)

WIN32 get unique file name
UINT WINAPI GetTempFileName(
  __in   LPCTSTR lpPathName,
  __in   LPCTSTR lpPrefixString,
  __in   UINT uUnique,
  __out  LPTSTR lpTempFileName
);


*/

        char tempPath[1024];
        char tempFileIn[1024];
        char tempFileOut[1024];
        GetTempPath(1024,tempPath);
        AnsiString tempFolder=AnsiString(tempPath);
        AnsiString filter="";
        if((CheckBox2->Enabled) && (CheckBox2->Checked))
                filter=":cairo";

        //need to create a temp dot file to get the final changes without saving it
//        AnsiString tempfile="\""+ExtractFilePath(Application->ExeName)+"__temp.dot"+"\"";
//        ((TfrmEditor*)frmMain->ActiveMDIChild)->R->Lines->SaveToFile(ExtractFilePath(Application->ExeName)+"__temp.dot");
        AnsiString tempfile=tempFolder+"__temp.dot";
        ((TfrmEditor*)frmMain->ActiveMDIChild)->R->Lines->SaveToFile(tempfile);

        AnsiString ExeFileName=Edit2->Text+"\\"+ComboBox1->Items->Strings[ComboBox1->ItemIndex]+".exe";
        AnsiString Params=" -T"+ComboBox2->Items->Strings[ComboBox2->ItemIndex]+filter;
        AnsiString Params2=" -Tjpg";

        Params=Params+" -o\""+Edit1->Text+"\""+" -K"+ComboBox1->Items->Strings[ComboBox1->ItemIndex];
        Params2=Params2+" -o\""+tempFolder+"__temp.jpg\""+" -K"+ComboBox1->Items->Strings[ComboBox1->ItemIndex];
        Memo1->Text=Memo1->Text.Trim();
        for (int ind=0; ind < Memo1->Lines->Count;ind ++)
        {
                Params = Params +" " +Memo1->Lines->Strings[ind];
                Params2 = Params2 +" " +Memo1->Lines->Strings[ind];

        }

//        ShowMessage(Params);
//        ShowMessage(Params2);
        frmMain->Memo1->Lines->Add("executing->"+ExeFileName+" "+Params);
        preproc();

//bool runproc(AnsiString szExeName,AnsiString szCommandLine,AnsiString tempFile,AnsiString engine,AnsiString outFile,bool silent)
        runproc(ExeFileName,Params,tempFolder+"__temp.dot",ComboBox1->Items->Strings[ComboBox1->ItemIndex],Edit1->Text,true);
        runproc(ExeFileName,Params2,tempFolder+"__temp.dot",ComboBox1->Items->Strings[ComboBox1->ItemIndex],Edit1->Text,false);



        TfrmEditor* Ed=((TfrmEditor*)frmMain->ActiveMDIChild);
        if (CheckBox1->Checked) //need preview
        {
                        if (!Ed->Editor) //if preview window is already created
                        {
                                Ed->Editor=new TfrmEditor(Application);
                                Ed->Editor->Editor=Ed;
                        }
                        Ed->Editor->tempFolder=tempFolder;
                        Ed->Editor->SwitchToPreview();
                        Ed->Editor->Caption="Preview:"+Ed->FileName;
                        Ed->Editor->Show();
                        Ed->SB2->Click();
                        Ed->SB1->Click();




        }
        //update Editor Setting values
        Ed->Engine=ComboBox1->ItemIndex;
        Ed->OutputFile=Edit1->Text;
        Ed->OutputType=ComboBox2->ItemIndex;
        Ed->Preview=CheckBox1->Checked;
        Ed->Options=Memo1->Text;
        //updating settings file
        TIniFile *ini;
        AnsiString FileName=ExtractFilePath( Application->ExeName)+"Settings.ini" ;
        if(FileExists(FileName))
        {
                ini = new TIniFile(FileName);
                ini->WriteInteger( "Settings", "Layout", ComboBox1->ItemIndex);
                ini->WriteInteger( "Settings", "Output", ComboBox2->ItemIndex);
                ini->WriteInteger( "Settings", "Preview", CheckBox1->Checked);
                ini->WriteString( "Settings", "InitialDir1", SD1->InitialDir);
                ini->WriteString( "Settings", "InitialDir2", SD2->InitialDir);
                ini->WriteString( "Settings", "InitialDir3", OD1->InitialDir);
                ini->WriteString( "Settings", "binPath", Edit2->Text);

                 delete ini;
        }

        ModalResult=1;
}




void __fastcall TfrmSettings::Button4Click(TObject *Sender)
{
        ModalResult=-1;

}
//---------------------------------------------------------------------------



void __fastcall TfrmSettings::Button5Click(TObject *Sender)
{
        preproc();
}
//---------------------------------------------------------------------------



bool preproc()
{
        //check if anything defined



        TIniFile *ini;
        AnsiString FileName=getIniFile() ;
        preprocflag=false;
        if(FileExists(FileName))
        {
                ini = new TIniFile(FileName);
                frmSettings->PreprocCmd=ini->ReadString( "Settings", "PreCmd", "");
                frmSettings->PreprocPath=ini->ReadString( "Settings", "PrePath", "");;
                 delete ini;
        }
        else
        {
                ShowMessage ("Settings.ini could not be located!");
                return false;

        }
        if (frmSettings->PreprocCmd.Trim() =="")
                return false;

   SECURITY_ATTRIBUTES saAttr;
   BOOL fSuccess;
   AnsiString hinputfile=ExtractFilePath(Application->ExeName)+"__temp.dot";
// Set the bInheritHandle flag so pipe handles are inherited.

   if (!FileExists(hinputfile))
   {
      ErrorExit("Temporary file is missing!");
      return 0;
   }
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
      ErrorExit("Stdout pipe creation failed\n");

// Ensure the read handle to the pipe for STDOUT is not inherited.

   SetHandleInformation( hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

// Create a pipe for the child process's STDIN.

   if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
      ErrorExit("Stdin pipe creation failed\n");

// Ensure the write handle to the pipe for STDIN is not inherited.

   SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

// Now create the child process.

   fSuccess = CreateChildProcess();
   if (! fSuccess)
      ErrorExit("Create process failed with");

// Get a handle to the parent's input file.

   if (hinputfile.Trim() =="")
      ErrorExit("Please specify an input file");

   hInputFile = CreateFile(hinputfile.c_str(), GENERIC_READ, 0, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

   if (hInputFile == INVALID_HANDLE_VALUE)
      ErrorExit("CreateFile failed");

// Write to pipe that is the standard input for a child process.

   WriteToPipe();

// Read from pipe that is the standard output for child process.

   ReadFromPipe();

   return 0;
}
BOOL CreateChildProcess()
{
        AnsiString Line=frmSettings->PreprocCmd;
        Line=Line.Trim();
        //need to parse the line assuming first word is the exe name , left is command line
        int ind=1;
        AnsiString szExeName="";
        AnsiString szCommandLine="";
        while ((ind <= Line.Length()) && (Line.SubString(ind,1)!=" "))
        {
                szExeName = szExeName + Line.SubString(ind,1);
                ind ++;
        }
        //if preproc exe path is specified use it
        if (frmSettings->PreprocPath.Trim() !="")
                szExeName =frmSettings->PreprocPath+"\\"+szExeName;
        //else use application path
        else
                szExeName =ExtractFilePath(Application->ExeName)+szExeName;
        szCommandLine=Line.SubString (ind+1,Line.Length()-ind);

   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bFuncRetn = FALSE;

// Set up members of the PROCESS_INFORMATION structure.

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

// Set up members of the STARTUPINFO structure.

   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
//   siStartInfo.hStdError = hChildStdoutWr;
   siStartInfo.hStdOutput = hChildStdoutWr;
   siStartInfo.hStdInput = hChildStdinRd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

// Create the child process.

   bFuncRetn = CreateProcess(szExeName.c_str(),
      szCommandLine.c_str(),     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      0,             // creation flags
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo);  // receives PROCESS_INFORMATION

   if (bFuncRetn == 0)
      ErrorExit("CreateProcess failed\n");
   else
   {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
   }
   return bFuncRetn;

}

VOID WriteToPipe(VOID)
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];

// Read from a file and write its contents to a pipe.

   for (;;)
   {
      if (! ReadFile(hInputFile, chBuf, BUFSIZE, &dwRead, NULL) ||
         dwRead == 0) break;
      if (! WriteFile(hChildStdinWr, chBuf, dwRead,
         &dwWritten, NULL)) break;
   }


// Close the pipe handle so the child process stops reading.

   if (! CloseHandle(hChildStdinWr))
      ErrorExit("Close pipe failed\n");
   if (! CloseHandle(hInputFile))
      ErrorExit("Close pipe failed for temporary file\n");
}

VOID ReadFromPipe(VOID)
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];
   preprocflag=false;
// Close the write end of the pipe before reading from the
// read end of the pipe.

   if (!CloseHandle(hChildStdoutWr))
      ErrorExit("Closing handle failed");

// Read output from the child process, and write to parent's STDOUT.

        TStringList* tmpString=new TStringList();
   for (;;)
   {
      if( !ReadFile( hChildStdoutRd, chBuf, BUFSIZE, &dwRead,
         NULL) || dwRead == 0) break;
        tmpString->Text=tmpString->Text+chBuf;
        /*      if (! WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL))
         break;*/

   }

     if(!CloseHandle(hChildStdoutRd))
              ErrorExit("Closing handle failed");

     if (tmpString->Text.Trim() !="")
     {
        preprocflag=true;
        tmpString->SaveToFile(ExtractFilePath(Application->ExeName)+"__temp2.dot");
     }
/*     if (frmSettings->CheckBox3->Checked)
     {
        TfrmEditor* Editor=new TfrmEditor(Application);
        Editor->ChangeFileName("Untitled"+IntToStr(frmMain->FileSeq++),true);
        Editor->R->Text=tmpString->Text;
        Editor->modified=true;
     }*/
   delete tmpString;
}

VOID ErrorExit (LPSTR lpszMessage)
{
   ShowMessage(lpszMessage);
}
void __fastcall TfrmSettings::FormClose(TObject *Sender,
      TCloseAction &Action)
{
/*        AnsiString tempfile="";
                tempfile=ExtractFilePath(Application->ExeName)+"__temp.dot";
                        DeleteFile(tempfile);
                tempfile=ExtractFilePath(Application->ExeName)+"__temp2.dot";
                        DeleteFile(tempfile);*/


}
//---------------------------------------------------------------------------



