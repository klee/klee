# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

pkgsharpdir = $(pkglibdir)/sharp
pkggodir = $(pkglibdir)/go
pkgguiledir = $(pkglibdir)/guile
pkgiodir = $(pkglibdir)/io
pkgjavadir = $(pkglibdir)/java
pkgluadir = $(pkglibdir)/lua
pkgocamldir = $(pkglibdir)/ocaml
pkgperldir = $(pkglibdir)/perl
pkgphpdir = $(pkglibdir)/php
pkgpythondir = $(pkglibdir)/python
pkgpython23dir = $(pkglibdir)/python23
pkgpython24dir = $(pkglibdir)/python24
pkgpython25dir = $(pkglibdir)/python25
pkgpython26dir = $(pkglibdir)/python26
pkgpython27dir = $(pkglibdir)/python27
pkgRdir = $(pkglibdir)/R
pkgrubydir = $(pkglibdir)/ruby
pkgtcldir = $(pkglibdir)/tcl

pkgindexdir = $(pkgtcldir)
if WITH_TCL
pkgindex_DATA = pkgIndex.tcl
endif

SUBDIRS = tclstubs tkstubs tclhandle gdtclft tcldot tclpathplan tkspline gv

pkgIndex.tcl: @TCL_PKGINDEX@ @TCL_PKGINDEX_GD@ @TCL_PKGINDEX_SWIG@ @TK_PKGINDEX@
	echo "# end" >end
	cat @TCL_PKGINDEX@ @TCL_PKGINDEX_GD@ @TCL_PKGINDEX_SWIG@ @TK_PKGINDEX@ end >$@
	rm -f end

# Installation of softlinks into non-owned directories is done as a
# separate step, with soft failures, so that a developer can do:
#    ./configure --prefix=$HOME/graphviz; make; make install
# without root priviledges.
install-data-hook:
if WITH_LUA
	-mkdir -p $(DESTDIR)@LUA_INSTALL_DIR@;
	if test -w $(DESTDIR)@LUA_INSTALL_DIR@; then \
		(cd $(DESTDIR)@LUA_INSTALL_DIR@; \
			$(LN_S) -f $(pkgluadir)/libgv_lua.so gv.so;) \
	else \
		echo "Warning: @LUA_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of lua binding."; \
	fi
endif
if WITH_PERL
	-mkdir -p $(DESTDIR)@PERL_INSTALL_DIR@;
	if test -w $(DESTDIR)@PERL_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PERL_INSTALL_DIR@; \
			$(LN_S) -f $(pkgperldir)/libgv_perl.so gv.so; \
			$(LN_S) -f $(pkgperldir)/gv.pm gv.pm;) \
	else \
		echo "Warning: @PERL_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of perl binding."; \
	fi
endif
if WITH_PHP
	-mkdir -p $(DESTDIR)@PHP_INSTALL_DIR@;
	if test -w $(DESTDIR)@PHP_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PHP_INSTALL_DIR@; \
			$(LN_S) -f $(pkgphpdir)/libgv_php.so gv.so;) \
	else \
		echo "Warning: @PHP_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of php binding."; \
	fi
	-mkdir -p $(DESTDIR)@PHP_INSTALL_DATADIR@;
	if test -w $(DESTDIR)@PHP_INSTALL_DATADIR@; then \
		(cd $(DESTDIR)@PHP_INSTALL_DATADIR@; \
			$(LN_S) -f $(pkgphpdir)/gv.php gv.php;) \
	else \
		echo "Warning: @PHP_INSTALL_DATADIR@ is not writable."; \
		echo "Skipping system installation of php binding."; \
	fi
endif
if WITH_PYTHON
	-mkdir -p $(DESTDIR)@PYTHON_INSTALL_DIR@;
	if test -w $(DESTDIR)@PYTHON_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PYTHON_INSTALL_DIR@; \
			$(LN_S) -f $(pkgpythondir)/libgv_python.so _gv.so; \
			$(LN_S) -f $(pkgpythondir)/gv.py gv.py;) \
	else \
		echo "Warning: @PYTHON_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of python binding."; \
	fi
endif
if WITH_PYTHON23
	-mkdir -p $(DESTDIR)@PYTHON23_INSTALL_DIR@;
	if test -w $(DESTDIR)@PYTHON23_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PYTHON23_INSTALL_DIR@; \
			$(LN_S) -f $(pkgpython23dir)/libgv_python23.so _gv.so; \
			$(LN_S) -f $(pkgpython23dir)/gv.py gv.py;) \
	else \
		echo "Warning: @PYTHON23_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of python23 binding."; \
	fi
endif
if WITH_PYTHON24
	-mkdir -p $(DESTDIR)@PYTHON24_INSTALL_DIR@;
	if test -w $(DESTDIR)@PYTHON24_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PYTHON24_INSTALL_DIR@; \
			$(LN_S) -f $(pkgpython24dir)/libgv_python24.so _gv.so; \
			$(LN_S) -f $(pkgpython24dir)/gv.py gv.py;) \
	else \
		echo "Warning: @PYTHON24_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of python24 binding."; \
	fi
endif
if WITH_PYTHON25
	-mkdir -p $(DESTDIR)@PYTHON25_INSTALL_DIR@;
	if test -w $(DESTDIR)@PYTHON25_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PYTHON25_INSTALL_DIR@; \
			$(LN_S) -f $(pkgpython25dir)/libgv_python25.so _gv.so; \
			$(LN_S) -f $(pkgpython25dir)/gv.py gv.py;) \
	else \
		echo "Warning: @PYTHON25_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of python25 binding."; \
	fi
endif
if WITH_PYTHON26
	-mkdir -p $(DESTDIR)@PYTHON26_INSTALL_DIR@;
	if test -w $(DESTDIR)@PYTHON26_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PYTHON26_INSTALL_DIR@; \
			$(LN_S) -f $(pkgpython26dir)/libgv_python26.so _gv.so; \
			$(LN_S) -f $(pkgpython26dir)/gv.py gv.py;) \
	else \
		echo "Warning: @PYTHON26_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of python26 binding."; \
	fi
endif
if WITH_PYTHON27
	-mkdir -p $(DESTDIR)@PYTHON27_INSTALL_DIR@;
	if test -w $(DESTDIR)@PYTHON27_INSTALL_DIR@; then \
		(cd $(DESTDIR)@PYTHON27_INSTALL_DIR@; \
			$(LN_S) -f $(pkgpython27dir)/libgv_python27.so _gv.so; \
			$(LN_S) -f $(pkgpython27dir)/gv.py gv.py;) \
	else \
		echo "Warning: @PYTHON27_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of python27 binding."; \
	fi
endif
if WITH_RUBY
	-mkdir -p $(DESTDIR)@RUBY_INSTALL_DIR@;
	if test -w $(DESTDIR)@RUBY_INSTALL_DIR@; then \
		(cd $(DESTDIR)@RUBY_INSTALL_DIR@; \
			$(LN_S) -f $(pkgrubydir)/libgv_ruby.so gv.so;) \
	else \
		echo "Warning: @RUBY_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of ruby binding."; \
	fi
endif
if WITH_TCL
	-mkdir -p $(DESTDIR)@TCL_INSTALL_DIR@;
	if test -w $(DESTDIR)@TCL_INSTALL_DIR@/; then \
		(cd $(DESTDIR)@TCL_INSTALL_DIR@; \
			$(LN_S) -f $(pkgtcldir) @PACKAGE_NAME@;) \
	else \
		echo "Warning: @TCL_INSTALL_DIR@ is not writable."; \
		echo "Skipping system installation of tcl bindings."; \
	fi
endif

# removal of installs into @xxx_INSTALL_DIR@ fail if root
#     has installed a system copy
uninstall-hook:
	-rm -rf $(DESTDIR)$(pkgsharpdir);
	-rm -rf $(DESTDIR)$(pkggodir);
	-rm -rf $(DESTDIR)$(pkgguiledir);
	-rm -rf $(DESTDIR)$(pkgiodir);
	-rm -rf $(DESTDIR)$(pkgjavadir);
	-rm -rf $(DESTDIR)$(pkgluadir);
	-rm -rf $(DESTDIR)@LUA_INSTALL_DIR@/gv.so;
	-rm -rf $(DESTDIR)$(pkgocamldir);
	-rm -rf $(DESTDIR)$(pkgperldir);
	-rm -rf $(DESTDIR)@PERL_INSTALL_DIR@/gv.so $(DESTDIR)@PERL_INSTALL_DIR@/gv.pm;
	-rm -rf $(DESTDIR)$(pkgphpdir);
	-rm -rf $(DESTDIR)@PHP_INSTALL_DIR@/gv.so $(DESTDIR)@PHP_INSTALL_DATADIR@/gv.php;
	-rm -rf $(DESTDIR)$(pkgpythondir);
	-rm -rf $(DESTDIR)@PYTHON_INSTALL_DIR@/_gv.so $(DESTDIR)@PYTHON_INSTALL_DIR@/gv.py;
	-rm -rf $(DESTDIR)$(pkgpython23dir);
	-rm -rf $(DESTDIR)@PYTHON23_INSTALL_DIR@/_gv.so $(DESTDIR)@PYTHON_INSTALL_DIR@/gv.py;
	-rm -rf $(DESTDIR)$(pkgpython24dir);
	-rm -rf $(DESTDIR)@PYTHON24_INSTALL_DIR@/_gv.so $(DESTDIR)@PYTHON_INSTALL_DIR@/gv.py;
	-rm -rf $(DESTDIR)$(pkgpython25dir);
	-rm -rf $(DESTDIR)@PYTHON25_INSTALL_DIR@/_gv.so $(DESTDIR)@PYTHON_INSTALL_DIR@/gv.py;
	-rm -rf $(DESTDIR)$(pkgpython26dir);
	-rm -rf $(DESTDIR)@PYTHON26_INSTALL_DIR@/_gv.so $(DESTDIR)@PYTHON_INSTALL_DIR@/gv.py;
	-rm -rf $(DESTDIR)$(pkgpython27dir);
	-rm -rf $(DESTDIR)@PYTHON27_INSTALL_DIR@/_gv.so $(DESTDIR)@PYTHON_INSTALL_DIR@/gv.py;
	-rm -rf $(DESTDIR)$(pkgRdir);
	-rm -rf $(DESTDIR)$(pkgrubydir);
	-rm -rf $(DESTDIR)@RUBY_INSTALL_DIR@/gv.so;
	-rm -rf $(DESTDIR)$(pkgtcldir);
	-rm -rf $(DESTDIR)@TCL_INSTALL_DIR@/@PACKAGE_NAME@-@PACKAGE_VERSION@;

EXTRA_DIST = Makefile.old mkpkgindex.sh

DISTCLEANFILES = pkgIndex.tcl
