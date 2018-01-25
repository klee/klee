:PACKAGE: graphviz

FEATURE_DOT == 1
FEATURE_X11 ==
FEATURE_WIN32 == 1
FEATURE_MS == 1
FEATURE_CS ==
FEATURE_XAW3D == 1
FEATURE_RUSAGE == 1
FEATURE_MINTSIZE == 1
LDFLAGS = -Yl,-FORCE:MULTIPLE
_LIB == 1
_MBCS == 1
_WINDOWS == 1
NDEBUG == 1

.SOURCE : gfx dot2l os/mswin32 ws/mswin32 examples

LEFTYSCRIPTS += def.lefty fractal.lefty tree.lefty lefty.psp

BINNAME = lefty
LIBNAME = gfx

src.bin.dot :OPTIONAL: \
	dot2l/dot2l.c dot2l/dot2l.h dot2l/dotlex.c dot2l/dotparse.c \
	dot2l/dottrie.c

src.lib.win :OPTIONAL: \
	ws/mswin32/garray.c ws/mswin32/gbutton.c ws/mswin32/gcanvas.c \
	ws/mswin32/gcommon.c ws/mswin32/gcommon.h ws/mswin32/glabel.c \
	ws/mswin32/gmenu.c ws/mswin32/gpcanvas.c ws/mswin32/gquery.c \
	ws/mswin32/gscroll.c ws/mswin32/gtext.c ws/mswin32/gview.c \
	ws/mswin32/resource.h

BINSRC += lefty.c gfxview.c txtview.c internal.c display.c str.c exec.c \
	parse.c lex.c tbl.c code.c io.c os/mswin32/io.c $(src.bin.dot) lefty.res
LIBSRC += g.c mem.c common.c $(src.lib.win)
LIBLIBS += -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32

:ALL:

$(BINNAME) :: $(BINSRC) -l$(LIBNAME) $(BINLIBS) -lm lefty.1
$(LIBNAME) :LIBRARY: $(LIBSRC) $(LIBLIBS)
$(LIBDIR)/lefty :INSTALLDIR: $(LEFTYSCRIPTS)

