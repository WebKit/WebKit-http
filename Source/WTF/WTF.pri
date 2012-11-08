# -------------------------------------------------------------------
# This file contains shared rules used both when building WTF itself
# and for targets that depend in some way on WTF.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

# All external modules should include WTF headers by prefixing with "wtf" (#include <wtf/some/thing.h>).
INCLUDEPATH += $$PWD

mac {
    # Mac OS does ship libicu but not the associated header files.
    # Therefore WebKit provides adequate header files.
    INCLUDEPATH += $${ROOT_WEBKIT_DIR}/Source/WTF/icu
    LIBS += -licucore
} else {
    contains(QT_CONFIG,icu) {
        win32: LIBS += -licuin -licuuc -licudt
        else: LIBS += -licui18n -licuuc -licudata
    }
}

linux-*:use?(GSTREAMER) {
    DEFINES += ENABLE_GLIB_SUPPORT=1
    PKGCONFIG += glib-2.0 gio-2.0
}

win32-* {
    LIBS += -lwinmm
    LIBS += -lgdi32
}

qnx {
    # required for timegm
    LIBS += -lnbutil
}

mac {
    LIBS += -framework AppKit
}

# MSVC is lacking stdint.h as well as inttypes.h.
win32-msvc*|win32-icc|wince*: INCLUDEPATH += $$ROOT_WEBKIT_DIR/Source/JavaScriptCore/os-win32
