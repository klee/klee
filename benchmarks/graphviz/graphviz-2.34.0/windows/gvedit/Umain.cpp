//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Umain.h"
#include "UEditor.h"
#include "Application.h"
#include "USettings.h"
#include "UAbout.h"
#include "UPreProcess.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#define SoftNAME        "GVexec"
TfrmMain *frmMain;
//---------------------------------------------------------------------------
__fastcall TfrmMain::TfrmMain(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TfrmMain::New1Click(TObject *Sender)
{
        TfrmEditor* Editor=new TfrmEditor(Application);
        Editor->ChangeFileName("Untitled"+IntToStr(FileSeq++),true);
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Label1Click(TObject *Sender)
{
        this->ActiveMDIChild->Caption="this is active";

}
//---------------------------------------------------------------------------



void __fastcall TfrmMain::FormCreate(TObject *Sender)
{
        FileSeq=1;
        AnsiString g=SoftNAME;
//        ODB->InitialDir=ReadFromRegistry("\\SOFTWARE\\ATT\\GRAPHVIZ\\","InstallPath")+"\\graphs";
/*        if (!FileExists(ODB->InitialDir+"\\dot.exe"))
        {
                ShowMessage (ODB->InitialDir+"\\dot.exe");
                ShowMessage ("Could not locate dot.exe , please make sure that dot.exe is in the same folder with GVedit");
                Application->Terminate();
        }*/
//        ODB->InitialDir=ExtractFilePath(Application->ExeName);
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Open1Click(TObject *Sender)
{
        if(ODB->Execute())
        {
                if(FileExists(ODB->FileName))
                {
                        TfrmEditor* Editor=new TfrmEditor(Application);
                        Editor->ChangeFileName(ODB->FileName,false);
                        FileSeq++;
                        Editor->R->Lines->LoadFromFile(ODB->FileName);
                        Editor->modified=false;
                }
        }

}
//---------------------------------------------------------------------------


void __fastcall TfrmMain::SaveAs1Click(TObject *Sender)
{
        if(this->ActiveMDIChild)
        {
                TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
                if(Ed->isEditor)
                        Ed->SaveAs();
                else
                        Ed->Editor->SaveAs();

        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Save1Click(TObject *Sender)
{
        ToolButton5->Click();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ToolButton1Click(TObject *Sender)
{
        frmSettings->Init(true);
}
//---------------------------------------------------------------------------
AnsiString TfrmMain::ReadFromRegistry(AnsiString RBKey,AnsiString RKey)
{

/*      example
        RBKey="\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"
        RKey="Path"
*/
        AnsiString S="";
        TRegistry *Registry = new TRegistry();
        try
        {
            Registry->RootKey = HKEY_LOCAL_MACHINE;
          // False because we do not want to create it if it doesn't exist
            if(Registry->OpenKey(RBKey,false))
            {
                    S=Registry->ReadString(RKey);
            }
            Registry->CloseKey();
            delete Registry;
            return S;
          }
          catch(...)
          {
            delete Registry;
            return S;
          }
}
void __fastcall TfrmMain::Undo1Click(TObject *Sender)
{

        TfrmEditor *Ed=((TfrmEditor*)this->ActiveMDIChild);
        if (Ed)
        {
                if (Ed->isEditor)
                        Ed->R->Undo();
        }
}
//---------------------------------------------------------------------------


void __fastcall TfrmMain::Cut1Click(TObject *Sender)
{
        TfrmEditor *Ed=((TfrmEditor*)this->ActiveMDIChild);
        if (Ed)
        {
                if (Ed->isEditor)
                        Ed->R->CutToClipboard();
        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Copy1Click(TObject *Sender)
{
        TfrmEditor *Ed=((TfrmEditor*)this->ActiveMDIChild);
        if (Ed)
        {
                if (Ed->isEditor)
                        Ed->R->CopyToClipboard();
        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Paste1Click(TObject *Sender)
{


        TfrmEditor *Ed=((TfrmEditor*)this->ActiveMDIChild);
        if (Ed)
        {
                if (Ed->isEditor)
                        Ed->R->PasteFromClipboard();
        }

}
//---------------------------------------------------------------------------


void __fastcall TfrmMain::SaveCommandlogtofile1Click(TObject *Sender)
{
        if( (Memo1->Text.Trim() !="")&& (SDB->Execute()))
                Memo1->Lines->SaveToFile(SDB->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Clear1Click(TObject *Sender)
{
        Memo1->Lines->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Cascade1Click(TObject *Sender)
{
        this->Cascade();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Tile1Click(TObject *Sender)
{
        this->Tile();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::DotProcess1Click(TObject *Sender)
{
        frmSettings->Init(true);

}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Settings1Click(TObject *Sender)
{
        frmSettings->Init();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ToolButton2Click(TObject *Sender)
{
        frmSettings->Init();
}
//---------------------------------------------------------------------------


void __fastcall TfrmMain::ToolButton3Click(TObject *Sender)
{
        TfrmEditor* Editor=new TfrmEditor(Application);
        Editor->ChangeFileName("Untitled"+IntToStr(FileSeq++),true);
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ToolButton6Click(TObject *Sender)
{
        if(ODB->Execute())
        {
                if(FileExists(ODB->FileName))
                {
                        TfrmEditor* Editor=new TfrmEditor(Application);
                        Editor->ChangeFileName(ODB->FileName,false);
                        FileSeq++;
                        Editor->R->Lines->LoadFromFile(ODB->FileName);
                        Editor->modified=false;
                }
        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ToolButton5Click(TObject *Sender)
{
        if(this->ActiveMDIChild)
        {
                TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
                if(Ed->isEditor)
                        Ed->Save();
                else
                        Ed->Editor->Save();

        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::AboutGraphvizEditor1Click(TObject *Sender)
{
        frmAbout->ShowModal();
}
//---------------------------------------------------------------------------
void TfrmMain::UpdateMainForm()
{
        //Updates buttons enabled/disabled respectively to active editor
        TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
        if(Ed)
        {
                ToolButton1->Enabled=true;
                ToolButton2->Enabled=true;

                ToolButton5->Enabled=Ed->modified;
                Save1->Enabled=Ed->modified;
                SaveAs1->Enabled=Ed->modified;
        }
        else
        {
                ToolButton1->Enabled=false;
                ToolButton2->Enabled=false;

                ToolButton5->Enabled=false;
                Save1->Enabled=false;
                SaveAs1->Enabled=false;

        }


}

void __fastcall TfrmMain::FormClose(TObject *Sender, TCloseAction &Action)
{
        //check all editors before closing
        TfrmEditor* EdStart=((TfrmEditor*)this->ActiveMDIChild);
        if(EdStart)
        {
                TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
                do
                {
                        Ed->Close();
                        Ed=((TfrmEditor*)this->ActiveMDIChild);
                }while (Ed);


        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Find1Click(TObject *Sender)
{
        if(this->ActiveMDIChild)
        {
                TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
                if(!Ed->isEditor)
                        Ed=Ed->Editor;
                Ed->FindDialog1->Position = Point(0, 0);
                Ed->FindDialog1->Execute();

        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Replace1Click(TObject *Sender)
{
        if(this->ActiveMDIChild)
        {
                TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
                if(!Ed->isEditor)
                        Ed=Ed->Editor;
                Ed->ReplaceDialog1->Position = Point(0, 0);
                Ed->ReplaceDialog1->Execute();

        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Timer1Timer(TObject *Sender)
{
        static int ct=0;
        Panel1->Color= (Panel1->Color==clRed)? clBtnFace:clRed;
        ct++;
        if(ct==10)
        {
                ct=0;
                Timer1->Enabled=false;
                Panel1->Color=clBtnFace;
        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Close1Click(TObject *Sender)
{
        if(this->ActiveMDIChild)
        {
                TfrmEditor* Ed=((TfrmEditor*)this->ActiveMDIChild);
                if(Ed->isEditor)
                        Ed->Close();
                else
                        Ed->Editor->Close();

        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Help2Click(TObject *Sender)
{
                AnsiString FileName="GVedit.html";
                AnsiString action="open";
                ShellExecute(NULL, action.c_str(),FileName.c_str(), NULL, NULL, SW_SHOW); //read me file, if exists it shows

}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Graphvizonnet1Click(TObject *Sender)
{
                AnsiString FileName="http://www.graphviz.org";
                AnsiString action="open";
                ShellExecute(NULL, action.c_str(),FileName.c_str(), NULL, NULL, SW_SHOW); //read me file, if exists it shows

}
//---------------------------------------------------------------------------


void __fastcall TfrmMain::PreprocessorSettings1Click(TObject *Sender)
{
        frmPre->ShowModal();
}
//---------------------------------------------------------------------------


void __fastcall TfrmMain::Button1Click(TObject *Sender)
{
        try{
                Image1->Picture->LoadFromFile("c:/gg.jpg");
        }
        catch(...)
        {
                ;
        }
}
//---------------------------------------------------------------------------


