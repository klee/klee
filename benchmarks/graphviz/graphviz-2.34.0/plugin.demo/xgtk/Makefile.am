ACLOCAL_AMFLAGS = -I m4

SUBDIRS=src

.PHONY: rpm
rpm: dist
	rpmbuild -ta xgtk-plugin-@VERSION@.tar.gz

EXTRA_DIST = xgtk-plugin.spec

