//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UEditor.h"
#include "Umain.h"
#include "Application.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmEditor *frmEditor;
//---------------------------------------------------------------------------
__fastcall TfrmEditor::TfrmEditor(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditor::RSelectionChange(TObject *Sender)
{
        int LineNumber = SendMessage(R->Handle, EM_LINEFROMCHAR, R->SelStart, 0);
        int ColNumber = (R->SelStart - SendMessage(R->Handle, EM_LINEINDEX, LineNumber, 0));
        StatusBar1->Panels->Items[0]->Text=IntToStr(LineNumber+1)+":"+IntToStr(ColNumber+1);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::FormClose(TObject *Sender,
      TCloseAction &Action)
{
        if (!isEditor) //if preview
        {
                this->Editor->Editor=NULL;
                delete this;
        }
        else
        {
                if (!modified)
                {
                        delete this->Editor;
                        delete this;

                }
                else
                {
                        int a=MessageDlg(FileName+" has changed , do you want to save changes?",mtInformation, TMsgDlgButtons() << mbYes<<mbNo << mbCancel, 0);
                        if(a==mrYes)
                        {
                                if(Save())
                                {
                                        delete this->Editor;
                                        delete this;
                                }
                                else
                                        Abort();


                        }
                        if(a==mrNo)
                                delete this;
                        if(a==mrCancel)
                                Abort();
                }
        }
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditor::FormActivate(TObject *Sender)
{
/*        frmMain->Caption=SOFTWARE_SHORT_NAME;
        frmMain->Caption = frmMain->Caption + " editing "+FileName;*/
        R->Color=clWhite;

}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::FormDeactivate(TObject *Sender)
{
        R->Color=clSilver;
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::RChange(TObject *Sender)
{
        modified=true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::BitBtn5Click(TObject *Sender)
{
        Image1->Align=alNone;
        Image1->Width=Image1->Picture->Width ;
        Image1->Height=Image1->Picture->Height;
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::BitBtn4Click(TObject *Sender)
{
        Image1->Align=alClient;
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditor::FormCreate(TObject *Sender)
{
        Engine=-1;
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditor::SB1Click(TObject *Sender)
{
        Image1->Align=alNone;
        Image1->Width=Image1->Picture->Width ;
        Image1->Height=Image1->Picture->Height;
        
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::SB2Click(TObject *Sender)
{
        Image1->Align=alClient;
        
}
//---------------------------------------------------------------------------



void __fastcall TfrmEditor::BitBtn2Click(TObject *Sender)
{
        if(Image1->Picture)
        {
                if(SDB->Execute())
                {
                        try{
                                Image1->Picture->SaveToFile(SDB->FileName);
                        }
                        catch(...)
                        {
                                ShowMessage ("File could not be saved, disk might be read only"); 

                        }


                }


        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::FindDialog1Find(TObject *Sender)
{
        int FoundAt, StartPos, ToEnd;
        // begin the search after the current selection
        // if there is one
        // otherwise, begin at the start of the text
        if (R->SelLength)
                StartPos = R->SelStart + R->SelLength;
        else
                StartPos = 0;

        // ToEnd is the length from StartPos
        // to the end of the text in the rich edit control

        ToEnd = R->Text.Length() - StartPos;
        FoundAt = R->FindText(FindDialog1->FindText, StartPos, ToEnd, TSearchTypes()<< stMatchCase);
        if (FoundAt != -1)
        {
                R->SetFocus();
                R->SelStart = FoundAt;
                R->SelLength = FindDialog1->FindText.Length();
        }
        else
        {
                if (R->SelLength)
                {
                        int a=MessageDlg("There is no more "+FindDialog1->FindTextA+" in the document , do you want to start over?",mtInformation, TMsgDlgButtons() << mbYes<<mbNo << mbCancel, 0);
                        if(a==mrYes)
                        {
                                R->SelLength=0;
                                FindDialog1->Execute();
                        }
                }
                else
                        ShowMessage ("Could not find specified text in the document");

        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::ReplaceDialog1Find(TObject *Sender)
{
        int FoundAt, StartPos, ToEnd;
        // begin the search after the current selection
        // if there is one
        // otherwise, begin at the start of the text
        if (R->SelLength)
                StartPos = R->SelStart + R->SelLength;
        else
                StartPos = 0;

        // ToEnd is the length from StartPos
        // to the end of the text in the rich edit control

        ToEnd = R->Text.Length() - StartPos;
        FoundAt = R->FindText(ReplaceDialog1->FindText, StartPos, ToEnd, TSearchTypes()<< stMatchCase);
        if (FoundAt != -1)
        {
                R->SetFocus();
                R->SelStart = FoundAt;
                R->SelLength = ReplaceDialog1->FindText.Length();
        }
        else
        {
                if (R->SelLength)
                {
                        int a=MessageDlg("There is no more "+ReplaceDialog1->FindTextA+" in the document , do you want to start over?",mtInformation, TMsgDlgButtons() << mbYes<<mbNo << mbCancel, 0);
                        if(a==mrYes)
                        {
                                R->SelLength=0;
                                ReplaceDialog1->Execute();
                        }
                }
                else
                        ShowMessage ("Could not find specified text in the document");

        }


}
//---------------------------------------------------------------------------

void __fastcall TfrmEditor::Timer1Timer(TObject *Sender)
{
        long GENERIC_ACCESS = 268435456;  //  &H10000000
        long EXCLUSIVE_ACCESS = 0;
        long OP= 3;
        void* ll_handle;
        static int counter=0;
        AnsiString filename=tempFolder+"__temp.jpg";
        counter++;

        if ((FileExists(filename)) || (counter == 10000))
        {
                if (counter==10000)      //could not create the preview
                {
                        frmMain->Memo1->Lines->Add("preview file for "+FileName+" could not be created!!");
                        frmMain->Memo1->SelStart=frmMain->Memo1->Text.Length()-1;
                        SendMessage(frmMain->Memo1->Handle, EM_SCROLLCARET,0,0);
                        counter=0;
                }
                else
                {
                                Timer1->Enabled=false;
                                try {
                                        Image1->Picture->LoadFromFile(filename);
                                }
                                catch(...)
                                {
                                        Timer1->Enabled=true;
                                        counter=0;
                                }

                }

        }
}
//---------------------------------------------------------------------------

