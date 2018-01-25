//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UPreProcess.h"
#include <SysUtils.hpp>
#include <windows.hpp>
#include <stdio.h>
#include <IniFiles.hpp>

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmPre *frmPre;
//---------------------------------------------------------------------------
__fastcall TfrmPre::TfrmPre(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
extern AnsiString getIniFile();
void __fastcall TfrmPre::FormShow(TObject *Sender)
{
        TIniFile *ini;
        AnsiString FileName=getIniFile() ;
        if(FileExists(FileName))
        {
                ini = new TIniFile(FileName);
                PreprocCmd=ini->ReadString( "Settings", "PreCmd", "");
                PreprocPath=ini->ReadString( "Settings", "PrePath", "");;
                PreprocPathEdit->Text=PreprocPath;
                PreprocEdit->Text=PreprocCmd;
                 delete ini;
        }
        else
        {
                ShowMessage ("Settings.ini could not be located!");
                Close();

        }

}
//---------------------------------------------------------------------------

void __fastcall TfrmPre::Button3Click(TObject *Sender)
{
        TIniFile *ini;
        AnsiString FileName=ExtractFilePath( Application->ExeName)+"Settings.ini" ;
        if(FileExists(FileName))
        {
                ini = new TIniFile(FileName);
                ini->WriteString( "Settings", "PreCmd", PreprocEdit->Text);
                ini->WriteString( "Settings", "PrePath", PreprocPathEdit->Text);
                 delete ini;
        }
        Close();

}
//---------------------------------------------------------------------------

void __fastcall TfrmPre::Button4Click(TObject *Sender)
{
        Close();        
}
//---------------------------------------------------------------------------

