//---------------------------------------------------------------------------

#ifndef UPreviewH
#define UPreviewH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TfrmPreview : public TForm
{
__published:	// IDE-managed Components
        TPanel *Panel1;
        TStatusBar *StatusBar1;
        TScrollBox *ScrollBox1;
        TBitBtn *BitBtn1;
        TBitBtn *BitBtn2;
        TBitBtn *BitBtn3;
        TBitBtn *BitBtn4;
private:	// User declarations
public:		// User declarations
        __fastcall TfrmPreview(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmPreview *frmPreview;
//---------------------------------------------------------------------------
#endif
