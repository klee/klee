
DEFINES += HAVE_CONFIG_H
LIBS += \
        -L$(top_builddir)/lib/gvc/.libs -lgvc \
        -L$(top_builddir)/lib/cgraph/.libs -lcgraph \
        -L$(top_builddir)/lib/cdt/.libs -lcdt \
        -lexpat -lz -lltdl

INCLUDEPATH += \
	../../lib/gvc \
	../../lib/common \
	../../lib/pathplan \
	../../lib/cgraph \
	../../lib/cdt \
	../..

CONFIG += qt
HEADERS = mainwindow.h mdichild.h csettings.h imageviewer.h ui_settings.h
SOURCES = main.cpp mainwindow.cpp mdichild.cpp csettings.cpp imageviewer.cpp
RESOURCES     = mdi.qrc

