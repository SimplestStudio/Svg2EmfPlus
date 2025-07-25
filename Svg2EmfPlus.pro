TEMPLATE = app
TARGET = svg2emfplus
CONFIG += c++11 utf8_sources
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += \
        $$PWD/src/nanosvg

SOURCES += \
        $$PWD/src/utils.cpp \
        $$PWD/src/main.cpp

HEADERS += \
        $$PWD/src/utils.h \
        $$PWD/src/resource.h \
        $$PWD/src/version.h

RC_FILE = \
        $$PWD/res/resource.rc

OTHER_FILES += \
        $$PWD/res/resource.rc \
        $$PWD/res/manifest/svg2emfplus.exe.manifest

CONFIG -= embed_manifest_exe

LIBS += -luser32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -lgdi32 \
        -lgdiplus
