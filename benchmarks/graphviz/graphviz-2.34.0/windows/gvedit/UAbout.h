//----------------------------------------------------------------------------
#ifndef UAboutH
#define UAboutH
//----------------------------------------------------------------------------
#include <vcl\System.hpp>
#include <vcl\Windows.hpp>
#include <vcl\SysUtils.hpp>
#include <vcl\Classes.hpp>
#include <vcl\Graphics.hpp>
#include <vcl\Forms.hpp>
#include <vcl\Controls.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Buttons.hpp>
#include <vcl\ExtCtrls.hpp>
#include <jpeg.hpp>
//----------------------------------------------------------------------------
class TfrmAbout : public TForm
{
__published:
	TPanel *Panel1;
	TImage *ProgramIcon;
	TLabel *ProductName;
	TLabel *Version;
	TButton *OKButton;
        TLabel *Label1;
private:
public:
	virtual __fastcall TfrmAbout(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TfrmAbout *frmAbout;
//----------------------------------------------------------------------------
#endif    
