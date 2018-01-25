//---------------------------------------------------------------------------

#ifndef UPreProcessH
#define UPreProcessH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TfrmPre : public TForm
{
__published:	// IDE-managed Components
        TPanel *Panel2;
        TLabel *Label9;
        TLabel *Label8;
        TLabel *Label10;
        TEdit *PreprocEdit;
        TEdit *PreprocPathEdit;
        TButton *Button4;
        TButton *Button3;
        void __fastcall FormShow(TObject *Sender);
        void __fastcall Button3Click(TObject *Sender);
        void __fastcall Button4Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfrmPre(TComponent* Owner);
        AnsiString PreprocCmd;
        AnsiString PreprocPath;
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmPre *frmPre;
//---------------------------------------------------------------------------
#endif
