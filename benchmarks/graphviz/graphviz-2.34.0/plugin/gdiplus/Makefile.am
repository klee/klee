# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt
	
if WITH_GDIPLUS
PATCHES = GdiPlusEnums.h.patch GdiPlusHeaders.h.patch GdiPlusimageAttributes.h.patch GdiPlusImaging.h.patch
PATCHED_HEADS = $(PATCHES:.h.patch=.h)

noinst_LTLIBRARIES = libgvplugin_gdiplus_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_gdiplus.la
else
pkglib_LTLIBRARIES = libgvplugin_gdiplus.la
endif
endif

noinst_HEADERS = FileStream.h gvplugin_gdiplus.h

nodist_libgvplugin_gdiplus_C_la_SOURCES = GdiPlus*.h
libgvplugin_gdiplus_C_la_SOURCES = \
	FileStream.cpp \
	gvdevice_gdiplus.cpp \
	gvloadimage_gdiplus.cpp \
	gvplugin_gdiplus.cpp \
	gvrender_gdiplus.cpp \
	gvtextlayout_gdiplus.cpp

# libtool doesn't want to run with actual Windows import libs, so we force GdiPlus.lib through to the linker
libgvplugin_gdiplus_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@ -Wl,"$(PLATFORMSDKLIB)\GdiPlus.lib"
nodist_libgvplugin_gdiplus_la_SOURCES = GdiPlus*.h
libgvplugin_gdiplus_la_SOURCES = $(libgvplugin_gdiplus_C_la_SOURCES)
libgvplugin_gdiplus_la_LIBADD = -lgdi32 -lole32 -lstdc++ -luuid $(top_builddir)/lib/gvc/libgvc.la

if WITH_WIN32
libgvplugin_gdiplus_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_gdiplus_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_gdiplus_LTX_library
endif

# object code is dependent on all the local GDI+ headers

gvdevice_gdiplus.lo : $(GDIPLUS_HEADS)

gvloadimage_gdiplus.lo : $(GDIPLUS_HEADS)

gvrender_gdiplus.lo : $(GDIPLUS_HEADS)

if WITH_GDIPLUS
# patched GDI+ headers are dependent on the patches
$(PATCHED_HEADS) : %.h : %.h.patch
	
# all the local GDI+ headers are dependent on the originals in PlatformSDK
# if the GDI+ header needs to be patched, patch it, else just copy it over
$(GDIPLUS_HEADS) : %.h : $(PLATFORMSDKINCLUDE_ESCAPED)/%.h
	if expr "$(PATCHED_HEADS)" : '.*$*.h' >/dev/null 2>&1; \
	then patch -o $*.h "$(PLATFORMSDKINCLUDE)\$*.h" $*.h.patch; \
	else cp "$(PLATFORMSDKINCLUDE)\$*.h" $*.h; \
	fi
endif
	
EXTRA_DIST = $(PATCHES) gvplugin_gdiplus.vcproj gvplugin_gdiplus.def
