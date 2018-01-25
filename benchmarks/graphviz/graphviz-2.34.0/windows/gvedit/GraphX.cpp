//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
USEFORM("Umain.cpp", frmMain);
USEFORM("UEditor.cpp", frmEditor);
USEFORM("USettings.cpp", frmSettings);
USEFORM("UPreview.cpp", frmPreview);
USEFORM("UAbout.cpp", frmAbout);
USEFORM("UPreProcess.cpp", frmPre);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

        try
        {
                 Application->Initialize();
                 Application->CreateForm(__classid(TfrmMain), &frmMain);
                 Application->CreateForm(__classid(TfrmSettings), &frmSettings);
                 Application->CreateForm(__classid(TfrmPreview), &frmPreview);
                 Application->CreateForm(__classid(TfrmAbout), &frmAbout);
                 Application->CreateForm(__classid(TfrmPre), &frmPre);
                 Application->Run();
        }
        catch (Exception &exception)
        {
                 Application->ShowException(&exception);
        }
        catch (...)
        {
                 try
                 {
                         throw Exception("");
                 }
                 catch (Exception &exception)
                 {
                         Application->ShowException(&exception);
                 }
        }
        return 0;
}
//---------------------------------------------------------------------------
