# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Add-in.
# ------------------------------------------------------

TEMPLATE = app
TARGET = QtStarter
DESTDIR = ../Win32/Release
QT += core network widgets gui winextras
CONFIG += release
DEFINES += WIN64 QT_DLL QT_WIDGETS_LIB QT_WINEXTRAS_LIB
INCLUDEPATH += ./GeneratedFiles \
    . \
    ./GeneratedFiles/Release
LIBS += -llibyaml-cppmd
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
win32:RC_FILE = QtStarter.rc
HEADERS += ./utils.h \
    ./stdafx.h \
    ./textprogressbar.h \
    ./qtstarter.h \
    ./logger.h \
    ./downloadmanager.h
SOURCES += ./downloadmanager.cpp \
    ./logger.cpp \
    ./main.cpp \
    ./utils.cpp \
    ./qtstarter.cpp \
    ./textprogressbar.cpp
FORMS += ./qtstarter.ui
RESOURCES += qtstarter.qrc
