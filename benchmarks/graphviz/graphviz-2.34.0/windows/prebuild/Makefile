# tools

WGET=wget
MAKE=make
TAR=tar
STRIP=strip

# local

LOCAL=$(CURDIR)/usr/local
LOCAL_INCLUDE=$(LOCAL)/include
LOCAL_LIB=$(LOCAL)/lib
LOCAL_BIN=$(LOCAL)/bin

# tool URL
UNZIP_BIN_URL=ftp://ftp.info-zip.org/pub/infozip/win32/unz552xn.exe
PKGCONFIG_BIN_URL=http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config-0.23-2.zip
GLIB_BIN_URL=http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.16/glib-2.16.3.zip
FLEX_BIN_URL=http://downloads.sourceforge.net/gnuwin32/flex-2.5.4a-1-bin.zip
BYACC_BIN_URL=http://downloads.sourceforge.net/gnuwin32/byacc-1.9-1-bin.zip

# package URL
ICONV_URL=http://yukihiro.nakadaira.googlepages.com/win_iconv.zip
ZLIB_URL=http://www.zlib.net/zlib-1.2.3.tar.gz
EXPAT_URL=http://downloads.sourceforge.net/expat/expat-2.0.1.tar.gz
FREETYPE_URL=http://downloads.sourceforge.net/freetype/freetype-2.3.5.tar.gz
FONTCONFIG_URL=http://fontconfig.org/release/fontconfig-2.5.91.tar.gz
PNG_URL=http://prdownloads.sourceforge.net/libpng/libpng-1.2.26.tar.gz
GD_URL=http://www.libgd.org/releases/gd-2.0.35.tar.gz
PIXMAN_URL=http://cairographics.org/releases/pixman-0.10.0.tar.gz
CAIRO_URL=http://cairographics.org/releases/cairo-1.6.4.tar.gz
GETTEXT_URL=ftp://mirrors.kernel.org/gnu/gettext/gettext-0.17.tar.gz
GLIB_URL=http://ftp.gnome.org/pub/GNOME/sources/glib/2.16/glib-2.16.3.tar.gz
PANGO_URL=http://ftp.gnome.org/pub/GNOME/sources/pango/1.20/pango-1.20.2.tar.gz

# tools
UNZIP_BIN=unzip.exe
PKGCONFIG_BIN=pkg-config.exe
GLIB_BIN=libglib-2.0-0.dll
FLEX_BIN=flex.exe
YACC_BIN=yacc.exe

# libraries
ICONV=libiconv.dll.a
ZLIB=libz.dll.a
EXPAT=libexpat.la
FREETYPE=libfreetype.la
FONTCONFIG=libfontconfig.la
PNG=libpng.dll.a
GD=libgd.la
PIXMAN=libpixman-1.la
CAIRO=libcairo.la
INTL=libintl.la
GLIB=libglib-2.0.la
PANGO=libpango-1.0.la

# tool archives
UNZIP_BIN_EXE=$(notdir $(UNZIP_BIN_URL))
PKGCONFIG_BIN_ZIP=$(notdir $(PKGCONFIG_BIN_URL))
GLIB_BIN_ZIP=$(notdir $(GLIB_BIN_URL))
FLEX_BIN_ZIP=$(notdir $(FLEX_BIN_URL))
BYACC_BIN_ZIP=$(notdir $(BYACC_BIN_URL))

# library directories
dirname=$(basename $(basename $(notdir $(1))))
ICONV_DIR=$(call dirname, $(ICONV_URL))
ZLIB_DIR=$(call dirname, $(ZLIB_URL))
EXPAT_DIR=$(call dirname, $(EXPAT_URL))
FREETYPE_DIR=$(call dirname, $(FREETYPE_URL))
FONTCONFIG_DIR=$(call dirname, $(FONTCONFIG_URL))
PNG_DIR=$(call dirname, $(PNG_URL))
GD_DIR=$(call dirname, $(GD_URL))
PIXMAN_DIR=$(call dirname, $(PIXMAN_URL))
CAIRO_DIR=$(call dirname, $(CAIRO_URL))
GETTEXT_DIR=$(call dirname, $(GETTEXT_URL))
GLIB_DIR=$(call dirname, $(GLIB_URL))
PANGO_DIR=$(call dirname, $(PANGO_URL))

#
# target
#
# --prefix to dump results into usr/local not /usr/local
# --disable-dependency-tracking since this is one-off build
# --enable-static=no since this doesn't build static libraries
# --enable-shared=yes since this builds dynamic libraries
# -O2 to optimize code for speed and performance
# -mms-bitfields to use Microsoft bit field layout

BASE_CONFIG=--prefix=$(LOCAL) --disable-dependency-tracking --enable-static=no --enable-shared=yes PKG_CONFIG="$(LOCAL_BIN)/$(PKGCONFIG_BIN)"
FLAG_CONFIG=CFLAGS="-O2 -mms-bitfields -I$(LOCAL_INCLUDE)" LDFLAGS="-L$(LOCAL_LIB)"

install: $(LOCAL_LIB)/$(PANGO) $(LOCAL_LIB)/$(GD) $(LOCAL_BIN)/$(FLEX_BIN) $(LOCAL_BIN)/$(YACC_BIN)

uninstall:
	$(MAKE) -C $(PANGO_DIR) uninstall
	$(MAKE) -C $(GLIB_DIR) uninstall
	$(MAKE) -C $(GETTEXT_DIR) uninstall
	$(MAKE) -C $(CAIRO_DIR) uninstall
	$(MAKE) -C $(PIXMAN_DIR) uninstall
	$(MAKE) -C $(GD_DIR) uninstall
	rm -rf $(LOCAL_BIN)/*png* $(LOCAL_INCLUDE)/*png* $(LOCAL_LIB)/*png* $(LOCAL_LIB)/pkgconfig/*png*
	$(MAKE) -C $(FONTCONFIG_DIR) uninstall
	$(MAKE) -C $(FREETYPE_DIR) uninstall
	$(MAKE) -C $(EXPAT_DIR) uninstall
	$(MAKE) -C $(ZLIB_DIR) uninstall
	rm -rf $(LOCAL_BIN)/*iconv* $(LOCAL_INCLUDE)/*iconv* $(LOCAL_LIB)/*iconv*
	rm -rf $(LOCAL_BIN)/$(PKGCONFIG_BIN)
	rm -rf $(LOCAL_BIN)/$(GLIB_BIN)
	rm -rf $(LOCAL_BIN)/flex.exe
	rm -rf $(LOCAL_BIN)/flex++.exe
	rm -rf $(LOCAL_BIN)/$(YACC_BIN)
	
distclean:
	$(MAKE) -C $(PANGO_DIR) distclean
	$(MAKE) -C $(GLIB_DIR) distclean
	$(MAKE) -C $(GETTEXT_DIR) distclean
	$(MAKE) -C $(CAIRO_DIR) distclean
	$(MAKE) -C $(PIXMAN_DIR) distclean
	$(MAKE) -C $(GD_DIR) distclean
	$(MAKE) -C $(PNG_DIR) clean
	$(MAKE) -C $(FONTCONFIG_DIR) distclean
	$(MAKE) -C $(FREETYPE_DIR) distclean
	$(MAKE) -C $(EXPAT_DIR) distclean
	$(MAKE) -C $(ZLIB_DIR) -f win32/Makefile.gcc clean
	$(MAKE) -C $(ICONV_DIR) clean

#
# pango
#
# --with-included-modules=yes to embed pango modules within pango itself
# patch to run installed instead of local pango-querymodules, otherwise dependent DLL won't load

$(LOCAL_LIB)/$(PANGO): $(PANGO_DIR)/pango/$(PANGO)
	@echo
	@echo INSTALLING PANGO...
	@echo
	$(MAKE) -C $(PANGO_DIR) install-strip

$(PANGO_DIR)/pango/$(PANGO): $(PANGO_DIR)/Makefile
	@echo
	@echo MAKING PANGO...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(PANGO_DIR) clean; fi && \
	PATH="$$PATH:/$(LOCAL_BIN)" && $(MAKE) -C $(PANGO_DIR)
	
$(PANGO_DIR)/Makefile: $(LOCAL_LIB)/$(CAIRO) $(LOCAL_LIB)/$(GLIB) | $(LOCAL_BIN)/$(PKGCONFIG_BIN) $(PANGO_DIR)/configure
	@echo
	@echo CONFIGURING PANGO...
	@echo
	cd $(PANGO_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG) --with-included-modules=yes

$(PANGO_DIR)/configure: | $(PANGO_DIR).tar.gz
	@echo
	@echo UNTARRING PANGO...
	@echo
	$(TAR) xzf $| && patch -p0 <pango.patch
		
$(PANGO_DIR).tar.gz:
	@echo
	@echo TRANSFERRING PANGO...
	@echo
	$(WGET) $(PANGO_URL) || rm -f $(PANGO_DIR).tar.gz

#
# glib
#
# path to installed gettext tools msgfmt and xgettext
# make glib DLL first, then copy over dependent DLL's and finally make everything else, so that glib-genmarshal will have the right DLL's to run

$(LOCAL_LIB)/$(GLIB): $(GLIB_DIR)/glib/$(GLIB)
	@echo
	@echo INSTALLING GLIB...
	@echo
	$(MAKE) -C $(GLIB_DIR) install-strip
	
$(GLIB_DIR)/glib/$(GLIB): $(GLIB_DIR)/Makefile
	@echo
	@echo MAKING GLIB...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(GLIB_DIR) clean; fi && \
	$(MAKE) -C $(GLIB_DIR)/glib && mkdir $(GLIB_DIR)/gobject/.libs && cp $(GLIB_DIR)/glib/.libs/libglib-2.0-0.dll $(GLIB_DIR)/gobject/.libs/ && cp $(LOCAL_BIN)/libintl-8.dll $(GLIB_DIR)/gobject/.libs/ && cp $(LOCAL_BIN)/iconv.dll $(GLIB_DIR)/gobject/.libs/ && $(MAKE) -C $(GLIB_DIR) 
	
$(GLIB_DIR)/Makefile: $(LOCAL_LIB)/$(INTL) |  $(LOCAL_BIN)/$(PKGCONFIG_BIN) $(GLIB_DIR)/configure 
	@echo
	@echo CONFIGURING GLIB...
	@echo
	cd $(GLIB_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG) MSGFMT="/$(LOCAL_BIN)/msgfmt" XGETTEXT="/$(LOCAL_BIN)/xgettext"
	
$(GLIB_DIR)/configure: | $(GLIB_DIR).tar.gz
	@echo
	@echo UNTARRING GLIB...
	@echo
	$(TAR) xzf $|
	
$(GLIB_DIR).tar.gz:
	@echo
	@echo TRANSFERRING GLIB...
	@echo
	$(WGET) $(GLIB_URL) || rm -f $(GLIB_DIR).tar.gz

#
# gettext
#
# actually tries to run code linked to iconv.dll, so we need to copy it over first
# patch to correct uninitialized _nl_msg_cat_cntr

$(LOCAL_LIB)/$(INTL): $(GETTEXT_DIR)/gettext-runtime/intl/$(INTL)
	@echo
	@echo INSTALLING GETTEXT...
	@echo
	$(MAKE) -C $(GETTEXT_DIR) install-strip
	
$(GETTEXT_DIR)/gettext-runtime/intl/$(INTL): $(GETTEXT_DIR)/Makefile
	@echo
	@echo MAKING GETTEXT...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(GETTEXT_DIR) clean; fi && \
	$(MAKE) -C $(GETTEXT_DIR)
	
$(GETTEXT_DIR)/Makefile: $(LOCAL_LIB)/$(ICONV) | $(GETTEXT_DIR)/configure
	@echo
	@echo CONFIGURING GETTEXT...
	@echo
	cd $(GETTEXT_DIR) && cp $(LOCAL_BIN)/iconv.dll gettext-runtime/ && cp $(LOCAL_BIN)/iconv.dll gettext-tools/ && ./configure $(BASE_CONFIG) $(FLAG_CONFIG)
	
$(GETTEXT_DIR)/configure: | $(GETTEXT_DIR).tar.gz
	@echo
	@echo UNTARRING GETTEXT...
	@echo
	$(TAR) xzf $| && patch -p0 <gettext.patch
	
$(GETTEXT_DIR).tar.gz:
	@echo
	@echo TRANSFERRING GETTEXT...
	@echo
	$(WGET) $(GETTEXT_URL) || rm -f $(GETTEXT_DIR).tar.gz

#
# cairo
#

$(LOCAL_LIB)/$(CAIRO): $(CAIRO_DIR)/src/$(CAIRO)
	@echo
	@echo INSTALLING CAIRO...
	@echo
	$(MAKE) -C $(CAIRO_DIR) install-strip
	
$(CAIRO_DIR)/src/$(CAIRO): $(CAIRO_DIR)/Makefile
	@echo
	@echo MAKING CAIRO...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(CAIRO_DIR) clean; fi && \
	$(MAKE) -C $(CAIRO_DIR)
	
$(CAIRO_DIR)/Makefile: $(LOCAL_LIB)/$(PIXMAN) $(LOCAL_LIB)/$(PNG) $(LOCAL_LIB)/$(FONTCONFIG) | $(LOCAL_BIN)/$(PKGCONFIG_BIN) $(CAIRO_DIR)/configure
	@echo
	@echo CONFIGURING CAIRO...
	@echo
	cd $(CAIRO_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG) 

$(CAIRO_DIR)/configure: | $(CAIRO_DIR).tar.gz
	@echo
	@echo UNTARRING CAIRO...
	@echo
	$(TAR) xzf $|
	
$(CAIRO_DIR).tar.gz:
	@echo
	@echo TRANSFERRING CAIRO...
	@echo
	$(WGET) $(CAIRO_URL) || rm -f $(CAIRO_DIR).tar.gz

#
# pixman
#
# patch to use -no-undefined libtool option to force DLL creation

$(LOCAL_LIB)/$(PIXMAN): $(PIXMAN_DIR)/pixman/$(PIXMAN)
	@echo
	@echo INSTALLING PIXMAN...
	@echo
	$(MAKE) -C $(PIXMAN_DIR) install-strip
	
$(PIXMAN_DIR)/pixman/$(PIXMAN): $(PIXMAN_DIR)/Makefile
	@echo
	@echo MAKING PIXMAN...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(PIXMAN_DIR) clean; fi && \
	$(MAKE) -C $(PIXMAN_DIR)
	
$(PIXMAN_DIR)/Makefile: | $(LOCAL_BIN)/$(PKGCONFIG_BIN) $(PIXMAN_DIR)/configure 
	@echo
	@echo CONFIGURING PIXMAN...
	@echo
	cd $(PIXMAN_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG)
		
$(PIXMAN_DIR)/configure: | $(PIXMAN_DIR).tar.gz
	@echo
	@echo UNTARRING PIXMAN...
	@echo
	$(TAR) xzf $| && patch -p0 <pixman.patch
	
$(PIXMAN_DIR).tar.gz:
	@echo
	@echo TRANSFERRING PIXMAN...
	@echo
	$(WGET) $(PIXMAN_URL) || rm -f $(PIXMAL_DIR).tar.gz

#
# gd
#
# define BGDWIN32 to force DLL import/export dance
# patch to use -no-undefined libtool option to force DLL creation

$(LOCAL_LIB)/$(GD): $(GD_DIR)/$(GD)
	@echo
	@echo INSTALLING GD...
	@echo
	$(MAKE) -C $(GD_DIR) install-strip
	
$(GD_DIR)/$(GD): $(GD_DIR)/Makefile
	@echo
	@echo MAKING GD...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(GD_DIR) clean; fi && \
	$(MAKE) -C $(GD_DIR)
	
$(GD_DIR)/Makefile: $(LOCAL_LIB)/$(ICONV) $(LOCAL_LIB)/$(PNG) $(LOCAL_LIB)/$(FONTCONFIG)| $(GD_DIR)/configure
	@echo
	@echo CONFIGURING GD...
	@echo
	cd $(GD_DIR) && ./configure $(BASE_CONFIG) CFLAGS="-O2 -DBGDWIN32 -mms-bitfields -I$(LOCAL_INCLUDE)" LDFLAGS="-L$(LOCAL_LIB)" --with-freetype=$(LOCAL)
	
$(GD_DIR)/configure: | $(GD_DIR).tar.gz
	@echo
	@echo UNTARRING GD...
	@echo
	$(TAR) xzf $| && patch -p0 <gd.patch && touch $(GD_DIR)/aclocal.m4 && touch $@ && touch $(GD_DIR)/Makefile.in && touch $(GD_DIR)/config.hin
	
$(GD_DIR).tar.gz:
	@echo
	@echo TRANSFERRING GD...
	@echo
	$(WGET) $(GD_URL) || rm -f $(GD_DIR).tar.gz
	
#
# fontconfig
#
# look for freetype and expat here
# patch to run installed instead of local fc-cache, otherwise dependent DLL won't load
# patch to include fcfreetype.h functions in library definition

$(LOCAL_LIB)/$(FONTCONFIG): $(FONTCONFIG_DIR)/src/$(FONTCONFIG)
	@echo
	@echo INSTALLING FONTCONFIG...
	@echo
	$(MAKE) -C $(FONTCONFIG_DIR) install-strip
	
$(FONTCONFIG_DIR)/src/$(FONTCONFIG): $(FONTCONFIG_DIR)/Makefile
	@echo
	@echo MAKING FONTCONFIG...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(FONTCONFIG_DIR) clean; fi && \
	$(MAKE) -C $(FONTCONFIG_DIR)
	
$(FONTCONFIG_DIR)/Makefile: $(LOCAL_LIB)/$(EXPAT) $(LOCAL_LIB)/$(FREETYPE) $(LOCAL_LIB)/$(ICONV) |  $(LOCAL_BIN)/$(PKGCONFIG_BIN) $(FONTCONFIG_DIR)/configure
	@echo
	@echo CONFIGURING FONTCONFIG...
	@echo
	cd $(FONTCONFIG_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG) --with-freetype-config=$(LOCAL_BIN)/freetype-config --with-expat=$(LOCAL)

$(FONTCONFIG_DIR)/configure: | $(FONTCONFIG_DIR).tar.gz
	@echo
	@echo UNTARRING FONTCONFIG...
	@echo
	$(TAR) xzf $| && patch -p0 <fontconfig.patch
	
$(FONTCONFIG_DIR).tar.gz:
	@echo
	@echo TRANSFERRING FONTCONFIG...
	@echo
	$(WGET) $(FONTCONFIG_URL) || rm -f $(FONTCONFIG_DIR).tar.gz
		
#
# freetype
#
# no install-strip option, so do our own stripping

$(LOCAL_LIB)/$(FREETYPE): $(FREETYPE_DIR)/objs/$(FREETYPE)
	@echo
	@echo INSTALLING FREETYPE...
	@echo
	$(MAKE) -C $(FREETYPE_DIR) install && $(STRIP) $(LOCAL_BIN)/libfreetype-6.dll
	
$(FREETYPE_DIR)/objs/$(FREETYPE): $(FREETYPE_DIR)/config.mk
	@echo
	@echo MAKING FREETYPE...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(FREETYPE_DIR) clean; fi && \
	$(MAKE) -C $(FREETYPE_DIR)
	
$(FREETYPE_DIR)/config.mk:  $(LOCAL_LIB)/$(ZLIB) | $(LOCAL_BIN)/$(PKGCONFIG_BIN) $(FREETYPE_DIR)/configure
	@echo
	@echo CONFIGURING FREETYPE...
	@echo
	cd $(FREETYPE_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG)
	
$(FREETYPE_DIR)/configure: | $(FREETYPE_DIR).tar.gz
	@echo
	@echo UNTARRING FREETYPE...
	@echo
	$(TAR) xzf $|
	
$(FREETYPE_DIR).tar.gz:
	@echo
	@echo TRANSFERRING FREETYPE...
	@echo
	$(WGET) $(FREETYPE_URL) || rm -f $(FREETYPE_DIR).tar.gz

#
# expat
#
# no install-strip option, so do our own stripping

$(LOCAL_LIB)/$(EXPAT): $(EXPAT_DIR)/$(EXPAT)
	@echo
	@echo INSTALLING EXPAT...
	@echo
	$(MAKE) -C $(EXPAT_DIR) install && $(STRIP) $(LOCAL_BIN)/libexpat-1.dll
	
$(EXPAT_DIR)/$(EXPAT): $(EXPAT_DIR)/Makefile
	@echo
	@echo MAKING EXPAT...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(EXPAT_DIR) clean; fi && \
	$(MAKE) -C $(EXPAT_DIR)
	
$(EXPAT_DIR)/Makefile: | $(EXPAT_DIR)/configure
	@echo
	@echo CONFIGURING EXPAT...
	@echo
	cd $(EXPAT_DIR) && ./configure $(BASE_CONFIG) $(FLAG_CONFIG)
	
$(EXPAT_DIR)/configure: | $(EXPAT_DIR).tar.gz
	@echo
	@echo UNTARRING EXPAT...
	@echo
	$(TAR) xzf $|
	
$(EXPAT_DIR).tar.gz:
	@echo
	@echo TRANSFERRING EXPAT...
	@echo
	$(WGET) $(EXPAT_URL) || rm -f $(EXPAT_DIR).tar.gz

#
# png
#
# don't use configure, use MinGW-specific makefile
# no install-strip option, so do our own stripping
# make calls install but because MinGW by default includes . on the PATH, it ends up calling the local install instead...

$(LOCAL_LIB)/$(PNG): $(PNG_DIR)/$(PNG)
	@echo
	@echo INSTALLING PNG...
	@echo
	mv $(PNG_DIR)/install $(PNG_DIR)/install.old && $(MAKE) -C $(PNG_DIR) -f scripts/makefile.mingw install prefix=$(LOCAL) && $(STRIP) $(LOCAL_BIN)/libpng12.dll && mv $(PNG_DIR)/install.old $(PNG_DIR)/install

$(PNG_DIR)/$(PNG): $(PNG_DIR)/scripts/makefile.mingw $(LOCAL_LIB)/$(ZLIB)
	@echo
	@echo MAKING PNG...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(PNG_DIR) -f scripts/makefile.mingw clean; fi && \
	$(MAKE) -C $(PNG_DIR) -f scripts/makefile.mingw MINGW_CCFLAGS="-mms-bitfields -I$(LOCAL_INCLUDE)" MINGW_LDFLAGS="-L$(LOCAL_LIB)"

$(PNG_DIR)/scripts/makefile.mingw: | $(PNG_DIR).tar.gz
	@echo
	@echo UNTARRING PNG...
	@echo
	$(TAR) xzf $|

$(PNG_DIR).tar.gz:
	@echo
	@echo TRANSFERRING PNG...
	@echo
	$(WGET) $(PNG_URL) || rm -f $(PNG_DIR).tar.gz
	
#
# zlib
#
# don't use configure, use MinGW-specific makefile
# no install-strip option, so do our own stripping
# patch to write implib properly (copied from mingwPORT file for zlib)

$(LOCAL_LIB)/$(ZLIB): $(ZLIB_DIR)/$(ZLIB)
	@echo
	@echo INSTALLING ZLIB...
	@echo
	$(MAKE) -C $(ZLIB_DIR) -f win32/Makefile.gcc install prefix=$(LOCAL) && $(STRIP) $(LOCAL_BIN)/zlib1.dll
	
$(ZLIB_DIR)/$(ZLIB): $(ZLIB_DIR)/win32/Makefile.gcc
	@echo
	@echo MAKING ZLIB...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(ZLIB_DIR) -f win32/Makefile.gcc clean; fi && \
	$(MAKE) -C $(ZLIB_DIR) -f win32/Makefile.gcc $(FLAG_CONFIG)
	
$(ZLIB_DIR)/win32/Makefile.gcc: | $(ZLIB_DIR).tar.gz
	@echo
	@echo UNTARRING ZLIB...
	@echo
	$(TAR) xzf $| && patch -p0 <zlib.patch
	
$(ZLIB_DIR).tar.gz:
	@echo
	@echo TRANSFERRING ZLIB...
	@echo
	$(WGET) $(ZLIB_URL) || rm -f $(ZLIB_DIR).tar.gz
	
#
# iconv
#
# no install option, copy over manually; DLL's already stripped
# patch to build implib, proper header fencing

$(LOCAL_LIB)/$(ICONV): $(ICONV_DIR)/src/$(ICONV)
	@echo
	@echo INSTALLING ICONV...
	@echo
	cp $(ICONV_DIR)/src/iconv.h $(LOCAL_INCLUDE) && cp $(ICONV_DIR)/src/win_iconv.exe $(LOCAL_BIN) && cp $(ICONV_DIR)/src/iconv.dll $(LOCAL_BIN) && cp $(ICONV_DIR)/src/libiconv.dll.a $(LOCAL_LIB) && cp $(ICONV_DIR)/src/win_iconv.exe $(LOCAL_BIN)
	
$(ICONV_DIR)/src/$(ICONV): $(ICONV_DIR)/src/Makefile
	@echo
	@echo MAKING ICONV...
	@echo
	if [[ -e $@ ]]; then $(MAKE) -C $(ICONV_DIR)/src clean; fi && \
	$(MAKE) -C $(ICONV_DIR)/src $(FLAG_CONFIG)

$(ICONV_DIR)/src/Makefile: | $(LOCAL_BIN)/$(UNZIP_BIN) $(ICONV_DIR).zip
	@echo
	@echo UNZIPPING ICONV...
	@echo
	$(LOCAL_BIN)/$(UNZIP_BIN) $(ICONV_DIR).zip && patch -p0 <iconv.patch
	
$(ICONV_DIR).zip:
	@echo
	@echo TRANSFERRING ICONV...
	@echo
	$(WGET) $(ICONV_URL) || rm -f $(ICONV_DIR).tar.gz
	
#
# pkgconfig
#
# install pkgconfig binary and dependent glib DLL
# NOTE: the glib build will evetually overwrite this DLL, but that should not disturb pkgconfig

$(LOCAL_BIN)/$(PKGCONFIG_BIN): | $(LOCAL_BIN)/$(UNZIP_BIN) $(LOCAL_BIN)/$(GLIB_BIN) $(PKGCONFIG_BIN_ZIP)
	@echo
	@echo UNZIPPING PKGCONFIG BIN...
	@echo
	mkdir -p $(LOCAL_BIN) && $(LOCAL_BIN)/$(UNZIP_BIN) $(PKGCONFIG_BIN_ZIP) bin/$(PKGCONFIG_BIN) -d $(LOCAL)

$(LOCAL_BIN)/$(GLIB_BIN): | $(LOCAL_BIN)/$(UNZIP_BIN) $(GLIB_BIN_ZIP)
	@echo
	@echo UNZIPPING GLIB BIN...
	@echo
	mkdir -p $(LOCAL_BIN) && $(LOCAL_BIN)/$(UNZIP_BIN) $(GLIB_BIN_ZIP) bin/$(GLIB_BIN) -d $(LOCAL)

$(PKGCONFIG_BIN_ZIP):
	@echo
	@echo TRANSFERRING PKGCONFIG BIN...
	@echo
	$(WGET) $(PKGCONFIG_BIN_URL) || rm -f $(PKGCONFIG_BIN_ZIP)

$(GLIB_BIN_ZIP):
	@echo
	@echo TRANSFERRING GLIB BIN...
	@echo
	$(WGET) $(GLIB_BIN_URL) || rm -f $(GLIB_BIN_ZIP)

#
# yacc
#
# install yacc binary

$(LOCAL_BIN)/$(YACC_BIN): | $(LOCAL_BIN)/$(UNZIP_BIN) $(BYACC_BIN_ZIP)
	@echo
	@echo UNZIPPING YACC BIN...
	@echo
	mkdir -p $(LOCAL_BIN) && $(LOCAL_BIN)/$(UNZIP_BIN) $(BYACC_BIN_ZIP) bin/yacc.exe -d $(LOCAL)

$(BYACC_BIN_ZIP):
	@echo
	@echo TRANSFERRING YACC BIN...
	@echo
	$(WGET) $(BYACC_BIN_URL) || rm -f $(BYACC_BIN_ZIP)

#
# flex
#
# install flex binary

$(LOCAL_BIN)/$(FLEX_BIN): | $(LOCAL_BIN)/$(UNZIP_BIN) $(FLEX_BIN_ZIP)
	@echo
	@echo UNZIPPING FLEX BIN...
	@echo
	mkdir -p $(LOCAL_BIN) && $(LOCAL_BIN)/$(UNZIP_BIN) $(FLEX_BIN_ZIP) bin/flex.exe bin/flex++.exe -d $(LOCAL)

$(FLEX_BIN_ZIP):
	@echo
	@echo TRANSFERRING FLEX BIN...
	@echo
	$(WGET) $(FLEX_BIN_URL) || rm -f $(FLEX_BIN_ZIP)

#
# unzip
#
# install unzip tool
 
$(LOCAL_BIN)/$(UNZIP_BIN): | $(UNZIP_BIN_EXE)
	mkdir -p $(LOCAL_BIN) && ./$(UNZIP_BIN_EXE) $(UNZIP_BIN) -d $(LOCAL_BIN)
	
$(UNZIP_BIN_EXE):
	@echo
	@echo TRANSFERRING ZIP BIN...
	@echo
	$(WGET) $(UNZIP_BIN_URL) || rm -f $(UNZIP_BIN_EXE)
	
#
# phony
#

.PHONY: install uninstall clean distclean
