//---------------------------------------------------------------------------

#ifndef USettingsH
#define USettingsH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Inifiles.hpp>
#include <StrUtils.hpp>
#include <Dialogs.hpp>
struct prop
{
        AnsiString Name;
        AnsiString Default;
        AnsiString Scope;
        AnsiString Engine;
};

//---------------------------------------------------------------------------
class TfrmSettings : public TForm
{
__published:	// IDE-managed Components
        TSaveDialog *SD1;
        TSaveDialog *SD2;
        TOpenDialog *OD1;
        TPanel *Panel1;
        TBevel *Bevel2;
        TBevel *Bevel1;
        TLabel *Label1;
        TLabel *Label2;
        TLabel *Label3;
        TLabel *Label4;
        TLabel *Label5;
        TLabel *Label6;
        TLabel *Label7;
        TComboBox *ComboBox1;
        TEdit *Edit1;
        TComboBox *ComboBox2;
        TButton *Button1;
        TCheckBox *CheckBox1;
        TButton *Button3;
        TButton *Button4;
        TEdit *Edit2;
        TComboBox *ComboBox3;
        TComboBox *ComboBox4;
        TMemo *Memo1;
        TButton *Button2;
        TButton *Button6;
        TEdit *Edit3;
        TBitBtn *BitBtn1;
        TBitBtn *BitBtn2;
        TBitBtn *BitBtn3;
        TCheckBox *CheckBox2;
        TMemo *Memo2;
        void __fastcall Button6Click(TObject *Sender);
        void __fastcall Button2Click(TObject *Sender);
        void __fastcall BitBtn2Click(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall ComboBox3Change(TObject *Sender);
        void __fastcall Button1Click(TObject *Sender);
        void __fastcall ComboBox2Change(TObject *Sender);
        void __fastcall BitBtn1Click(TObject *Sender);
        void __fastcall BitBtn3Click(TObject *Sender);
        void __fastcall Button3Click(TObject *Sender);
        void __fastcall Button4Click(TObject *Sender);
        void __fastcall Button5Click(TObject *Sender);
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
private:	// User declarations
public:		// User declarations
        __fastcall TfrmSettings(TComponent* Owner);
        int TfrmSettings::Init(bool silent=false);
        bool LoadAttrs(AnsiString Engine,AnsiString Scope);
        bool DeleteIniKeys(TComboBox*);
        void TfrmSettings::RunDosInMemo(String DosApp,TMemo* AMemo) ;
//        bool TfrmSettings::runproc(AnsiString szExeName,AnsiString szCommandLine,bool silent);
        AnsiString PreprocCmd;
        AnsiString PreprocPath;

//        bool TfrmSettings::preproc(AnsiString Line,bool silent);

};
//---------------------------------------------------------------------------
bool TfrmSettings::LoadAttrs(AnsiString Engine,AnsiString Scope)
{
        ComboBox4->Items->Clear();
        TStringList* props=new TStringList();
        props->LoadFromFile(ExtractFilePath(Application->ExeName)+"props.txt");
        for(int ind=0;ind <props->Count;ind++)
        {
                int tokenindex=0;
                AnsiString a="";
                AnsiString DATA[4];
                for(int ind2=1;ind2 <=props->Strings[ind].Length();ind2++)
                {
                        a=props->Strings[ind].SubString(ind2,1);
                        if(a !=",")
                        {
                                if (a != "\"")
                                        DATA[tokenindex]=DATA[tokenindex]+a;
                        }
                        else
                                tokenindex++;
                }

                if (  AnsiContainsText(DATA[2],Scope) || AnsiContainsText(DATA[2],"ANY_ELEMENT"))
                {
                        if ( AnsiContainsText(DATA[3],Scope) || AnsiContainsText(DATA[3],"ALL_ENGINES"))
                        {
                                ComboBox4->Items->Add(DATA[0]);
                        }
                }


        }
        ComboBox4->ItemIndex=0;

         return true;

}
bool TfrmSettings::DeleteIniKeys(TComboBox* A)
{
        bool go;
        AnsiString a="";
        AnsiString data="";
        AnsiString data2="";
        for (int ind=0;ind < A->Items->Count;ind++)
        {
                data=A->Items->Strings[ind];
                go=false;
                data2="";
                for (int ind2=1;ind2 <=data.Length();ind2++)
                {
                        a=data.SubString(ind2,1);
                        if (go)
                                data2=data2+a;
                        if(a=="=")
                                go=true;
                }
                A->Items->Strings[ind]=data2;
        }
        return true;

}



extern PACKAGE TfrmSettings *frmSettings;
//---------------------------------------------------------------------------
#endif
