//---------------------------------------------------------------------------

#ifndef UEditorH
#define UEditorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Dialogs.hpp>
#include <Buttons.hpp>
#include <jpeg.hpp>
//---------------------------------------------------------------------------
class TfrmEditor : public TForm
{
__published:	// IDE-managed Components
        TStatusBar *StatusBar1;
        TSaveDialog *SDB;
        TPanel *Panel2;
        TRichEdit *R;
        TPanel *Panel1;
        TScrollBox *ScrollBox1;
        TImage *Image1;
        TPanel *Panel3;
        TSpeedButton *SB1;
        TBitBtn *BitBtn2;
        TSpeedButton *SB2;
        TFindDialog *FindDialog1;
        TReplaceDialog *ReplaceDialog1;
        TTimer *Timer1;
        TSaveDialog *SDB2;
        void __fastcall RSelectionChange(TObject *Sender);
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
        void __fastcall FormActivate(TObject *Sender);
        void __fastcall FormDeactivate(TObject *Sender);
        void __fastcall RChange(TObject *Sender);
        void __fastcall BitBtn5Click(TObject *Sender);
        void __fastcall BitBtn4Click(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall SB1Click(TObject *Sender);
        void __fastcall SB2Click(TObject *Sender);
        void __fastcall BitBtn2Click(TObject *Sender);
        void __fastcall FindDialog1Find(TObject *Sender);
        void __fastcall ReplaceDialog1Find(TObject *Sender);
        void __fastcall Timer1Timer(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfrmEditor(TComponent* Owner);
        bool isEditor; //I use same class for preview , false for preview window
        bool modified;
        bool NeedFileName;
        AnsiString FileName;
        TfrmEditor *Editor;     //for preview windows this stores the editor file of preview
        //Setting Window values
        int Engine;
        AnsiString OutputFile;
        int OutputType;
        bool Preview;
        AnsiString Options;
        AnsiString tempFolder;

        bool TfrmEditor::ChangeFileName(AnsiString NewFileName,bool newfile=false);
        bool TfrmEditor::SaveAs();
        bool TfrmEditor::Save();
        bool SwitchToPreview();
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmEditor *frmEditor;
//---------------------------------------------------------------------------
bool TfrmEditor::SwitchToPreview()
{
        StatusBar1->Visible=false;
        Panel2->Visible=false;
        Timer1->Enabled=true;
        Image1->Picture=NULL;
        return true;
}

bool TfrmEditor::ChangeFileName(AnsiString NewFileName,bool newfile)
{
        //if newfile is true even save commands runs like save as..

        FileName=NewFileName;
        Caption="Graphviz Layout("+NewFileName+")";
        NeedFileName=newfile;
        isEditor=true;
        return true;


}
bool TfrmEditor::SaveAs()
{
        SDB2->FileName=FileName;
        if(SDB2->Execute())
        {
                try{
                        R->Lines->SaveToFile(SDB2->FileName);
                        ChangeFileName(SDB2->FileName,false);
                        modified=false;
                        return true;
                }
                catch(...)
                {;}
        }
        return false;

}
bool TfrmEditor::Save()
{
        if (NeedFileName)
                return SaveAs();
        else
        {
                try{
                        R->Lines->SaveToFile(FileName);
                        modified=false;
                        return true;
                }
                catch(...)
                {
                        ShowMessage("Unexpected error, could not save the file");
                        return false;
                }
        }
}



#endif
