set root=<ROOT>
set kind=Release
mkdir %root%
mkdir %root%\lib
mkdir %root%\bin
mkdir %root%\include

cd cmd
cd dot\%kind%
copy dot.exe %root%\bin
copy dot.exe %root%\bin\neato.exe
copy dot.exe %root%\bin\twopi.exe
copy dot.exe %root%\bin\circo.exe
copy dot.exe %root%\bin\fdp.exe
cd ..\..

cd lefty\%kind%
copy lefty.exe %root%\bin
cd ..\..
cd dotty\%kind%
copy dotty.exe %root%\bin
cd ..\..
cd lneato\%kind%
copy lneato.exe %root%\bin
cd ..\..

cd tools\%kind%
copy Acyclic.exe %root%\bin
copy ccomps.exe %root%\bin
copy gvcolor.exe %root%\bin
copy gc.exe %root%\bin
copy nop.exe %root%\bin
copy sccmap.exe %root%\bin
copy tred.exe %root%\bin
copy unflatten.exe %root%\bin
copy gxl2dot.exe %root%\bin
copy dijkstra.exe %root%\bin
copy bcomps.exe %root%\bin
copy gvpack.exe %root%\bin
copy gxl2dot.exe %root%\bin\dot2gxl.exe
cd ..\..

copy gvui\gvui.exe %root%\bin

cd dotty
copy dotty.lefty %root%\bin
copy dotty_draw.lefty %root%\bin
copy dotty_edit.lefty %root%\bin
copy dotty_layout.lefty %root%\bin
copy dotty_ui.lefty %root%\bin
cd ..

cd ..\lib\lib\%kind% 
copy ingraphs.lib %root%\lib
copy agraph.lib %root%\lib
copy cdt.lib %root%\lib
copy circogen.lib %root%\lib
copy common.lib %root%\lib
copy dotgen.lib %root%\lib
copy fdpgen.lib %root%\lib
copy gd.lib %root%\lib
copy graph.lib %root%\lib
copy gvc.lib %root%\lib
copy neatogen.lib %root%\lib
copy pack.lib %root%\lib
copy pathplan.lib %root%\lib
copy twopigen.lib %root%\lib
cd ..\..\..

cd plugin\%kind% 
copy plugin.lib %root%\lib
cd ..\..

cd third-party\lib
copy *.dll %root%\bin
copy ft.lib %root%\lib
copy jpeg.lib %root%\lib
copy libexpat.lib %root%\lib
copy libexpatw.lib %root%\lib
copy libz.lib %root%\lib
copy png.lib %root%\lib
copy z.lib %root%\lib
cd ..\..

copy lib\agraph\agraph.h %root%\include
copy lib\cdt\cdt.h %root%\include
copy lib\common\color.h %root%\include
copy lib\common\geom.h %root%\include
copy lib\graph\graph.h %root%\include
copy lib\gvc\gvc.h %root%\include
copy lib\gvc\gvcext.h %root%\include
copy lib\gvc\gvcjob.h %root%\include
copy lib\gvc\gvcommon.h  %root%\include
copy lib\gvc\gvplugin.h  %root%\include
copy lib\gvc\gvplugin_layout.h  %root%\include
copy lib\gvc\gvplugin_loadimage.h  %root%\include
copy lib\gvc\gvplugin_render.h  %root%\include
copy lib\gvc\gvplugin_textlayout.h %root%\include
copy lib\ingraphs\ingraphs.h %root%\include
copy lib\pack\pack.h %root%\include
copy lib\pathplan\pathgeom.h %root%\include
copy lib\common\textpara.h %root%\include
copy lib\common\types.h %root%\include
copy lib\common\usershape.h %root%\include

