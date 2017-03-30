# This file was copied from qtbase/src/3rdparty/libpng/libpng.pro
# in order to build a copy of libqtpng specifically for QtWebKit module
# Must be kept in sync with qtbase

load(functions)

TARGET = qtpng

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off

load(qt_helper_lib)

DESTDIR = $$ROOT_BUILD_DIR/lib
PNGDIR = $$QTBASE_DIR/src/3rdparty/libpng

DEFINES += PNG_ARM_NEON_OPT=0
SOURCES += \
    $$PNGDIR/png.c \
    $$PNGDIR/pngerror.c \
    $$PNGDIR/pngget.c \
    $$PNGDIR/pngmem.c \
    $$PNGDIR/pngpread.c \
    $$PNGDIR/pngread.c \
    $$PNGDIR/pngrio.c \
    $$PNGDIR/pngrtran.c \
    $$PNGDIR/pngrutil.c \
    $$PNGDIR/pngset.c \
    $$PNGDIR/pngtrans.c \
    $$PNGDIR/pngwio.c \
    $$PNGDIR/pngwrite.c \
    $$PNGDIR/pngwtran.c \
    $$PNGDIR/pngwutil.c

TR_EXCLUDE += $$PWD/*

include($$QTBASE_DIR/src/3rdparty/zlib_dependency.pri)
