//---------------------------------------------------------------------------

#ifndef UmainH
#define UmainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ToolWin.hpp>
#include <ImgList.hpp>
#include <Dialogs.hpp>
#include <Registry.hpp>
#include <ADODB.hpp>
#include <DB.hpp>
#include <jpeg.hpp>
//---------------------------------------------------------------------------
class TfrmMain : public TForm
{
__published:	// IDE-managed Components
        TMainMenu *MainMenu1;
        TMenuItem *File1;
        TMenuItem *Graphviz1;
        TMenuItem *Help1;
        TMenuItem *Edit1;
        TMenuItem *New1;
        TMenuItem *Open1;
        TMenuItem *Save1;
        TMenuItem *SaveAs1;
        TMenuItem *Undo1;
        TMenuItem *Cut1;
        TMenuItem *Copy1;
        TMenuItem *Paste1;
        TMenuItem *DotProcess1;
        TMenuItem *AboutGraphvizEditor1;
        TStatusBar *StatusBar1;
        TMenuItem *Settings1;
        TPanel *Panel1;
        TMemo *Memo1;
        TSplitter *Splitter1;
        TPopupMenu *PopupMenu1;
        TMenuItem *SaveCommandlogtofile1;
        TMenuItem *Clear1;
        TToolBar *ToolBar1;
        TToolButton *ToolButton1;
        TImageList *ImageList1;
        TOpenDialog *ODB;
        TSaveDialog *SDB;
        TMenuItem *View1;
        TMenuItem *Cascade1;
        TMenuItem *Tile1;
        TToolButton *ToolButton2;
        TToolButton *ToolButton4;
        TToolButton *ToolButton5;
        TToolButton *ToolButton6;
        TToolButton *ToolButton3;
        TMenuItem *Find1;
        TTimer *Timer1;
        TMenuItem *Close1;
        TMenuItem *Help2;
        TMenuItem *Graphvizonnet1;
        TMenuItem *PreprocessorSettings1;
        TADOTable *ADOTable1;
        TImage *Image1;
        void __fastcall New1Click(TObject *Sender);
        void __fastcall Label1Click(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall Open1Click(TObject *Sender);
        void __fastcall SaveAs1Click(TObject *Sender);
        void __fastcall Save1Click(TObject *Sender);
        void __fastcall ToolButton1Click(TObject *Sender);
        void __fastcall Undo1Click(TObject *Sender);
        void __fastcall Cut1Click(TObject *Sender);
        void __fastcall Copy1Click(TObject *Sender);
        void __fastcall Paste1Click(TObject *Sender);
        void __fastcall SaveCommandlogtofile1Click(TObject *Sender);
        void __fastcall Clear1Click(TObject *Sender);
        void __fastcall Cascade1Click(TObject *Sender);
        void __fastcall Tile1Click(TObject *Sender);
        void __fastcall DotProcess1Click(TObject *Sender);
        void __fastcall Settings1Click(TObject *Sender);
        void __fastcall ToolButton2Click(TObject *Sender);
        void __fastcall ToolButton3Click(TObject *Sender);
        void __fastcall ToolButton6Click(TObject *Sender);
        void __fastcall ToolButton5Click(TObject *Sender);
        void __fastcall AboutGraphvizEditor1Click(TObject *Sender);
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
        void __fastcall Find1Click(TObject *Sender);
        void __fastcall Replace1Click(TObject *Sender);
        void __fastcall Timer1Timer(TObject *Sender);
        void __fastcall Close1Click(TObject *Sender);
        void __fastcall Help2Click(TObject *Sender);
        void __fastcall Graphvizonnet1Click(TObject *Sender);
        void __fastcall PreprocessorSettings1Click(TObject *Sender);
        void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfrmMain(TComponent* Owner);
        int FileSeq;
        AnsiString ReadFromRegistry(AnsiString RBKey,AnsiString RKey);
        void TfrmMain::UpdateMainForm();
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif
