:PACKAGE: graphviz

dotty :: dotty.pdf mswin32/dotty.c \
	-luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32

$(BINDIR) :INSTALLDIR: dotty.lefty dotty_draw.lefty dotty_edit.lefty \
	dotty_layout.lefty dotty_ui.lefty
