# -------------------------------------------------------------------
# This file contains shared rules used both when building WebCore
# itself, and by targets that use WebCore.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source/WebCore

QT *= network sql core-private gui-private

WEBCORE_GENERATED_SOURCES_DIR = $${ROOT_BUILD_DIR}/Source/WebCore/$${GENERATED_SOURCES_DESTDIR}

INCLUDEPATH += \
    $$SOURCE_DIR \
    $$SOURCE_DIR/Modules/filesystem \
    $$SOURCE_DIR/Modules/geolocation \
    $$SOURCE_DIR/Modules/indexeddb \
    $$SOURCE_DIR/Modules/navigatorcontentutils \
    $$SOURCE_DIR/Modules/notifications \
    $$SOURCE_DIR/Modules/quota \
    $$SOURCE_DIR/Modules/webaudio \
    $$SOURCE_DIR/Modules/webdatabase \
    $$SOURCE_DIR/Modules/websockets \
    $$SOURCE_DIR/accessibility \
    $$SOURCE_DIR/bindings \
    $$SOURCE_DIR/bindings/generic \
    $$SOURCE_DIR/bridge \
    $$SOURCE_DIR/bridge/qt \
    $$SOURCE_DIR/css \
    $$SOURCE_DIR/dom \
    $$SOURCE_DIR/dom/default \
    $$SOURCE_DIR/editing \
    $$SOURCE_DIR/fileapi \
    $$SOURCE_DIR/history \
    $$SOURCE_DIR/html \
    $$SOURCE_DIR/html/canvas \
    $$SOURCE_DIR/html/parser \
    $$SOURCE_DIR/html/shadow \
    $$SOURCE_DIR/html/track \
    $$SOURCE_DIR/inspector \
    $$SOURCE_DIR/loader \
    $$SOURCE_DIR/loader/appcache \
    $$SOURCE_DIR/loader/archive \
    $$SOURCE_DIR/loader/cache \
    $$SOURCE_DIR/loader/icon \
    $$SOURCE_DIR/mathml \
    $$SOURCE_DIR/page \
    $$SOURCE_DIR/page/animation \
    $$SOURCE_DIR/page/qt \
    $$SOURCE_DIR/page/scrolling \
    $$SOURCE_DIR/platform \
    $$SOURCE_DIR/platform/animation \
    $$SOURCE_DIR/platform/audio \
    $$SOURCE_DIR/platform/graphics \
    $$SOURCE_DIR/platform/graphics/filters \
    $$SOURCE_DIR/platform/graphics/filters/arm \
    $$SOURCE_DIR/platform/graphics/opengl \
    $$SOURCE_DIR/platform/graphics/opentype \
    $$SOURCE_DIR/platform/graphics/qt \
    $$SOURCE_DIR/platform/graphics/surfaces \
    $$SOURCE_DIR/platform/graphics/texmap \
    $$SOURCE_DIR/platform/graphics/transforms \
    $$SOURCE_DIR/platform/image-decoders \
    $$SOURCE_DIR/platform/image-decoders/bmp \
    $$SOURCE_DIR/platform/image-decoders/ico \
    $$SOURCE_DIR/platform/image-decoders/gif \
    $$SOURCE_DIR/platform/image-decoders/jpeg \
    $$SOURCE_DIR/platform/image-decoders/png \
    $$SOURCE_DIR/platform/image-decoders/webp \
    $$SOURCE_DIR/platform/leveldb \
    $$SOURCE_DIR/platform/mock \
    $$SOURCE_DIR/platform/network \
    $$SOURCE_DIR/platform/network/qt \
    $$SOURCE_DIR/platform/qt \
    $$SOURCE_DIR/platform/sql \
    $$SOURCE_DIR/platform/text \
    $$SOURCE_DIR/platform/text/transcoder \
    $$SOURCE_DIR/plugins \
    $$SOURCE_DIR/rendering \
    $$SOURCE_DIR/rendering/mathml \
    $$SOURCE_DIR/rendering/style \
    $$SOURCE_DIR/rendering/svg \
    $$SOURCE_DIR/storage \
    $$SOURCE_DIR/svg \
    $$SOURCE_DIR/svg/animation \
    $$SOURCE_DIR/svg/graphics \
    $$SOURCE_DIR/svg/graphics/filters \
    $$SOURCE_DIR/svg/properties \
    $$SOURCE_DIR/testing \
    $$SOURCE_DIR/websockets \
    $$SOURCE_DIR/workers \
    $$SOURCE_DIR/xml \
    $$SOURCE_DIR/xml/parser \
    $$SOURCE_DIR/../ThirdParty

INCLUDEPATH += \
    $$SOURCE_DIR/bridge/jsc \
    $$SOURCE_DIR/bindings/js \
    $$SOURCE_DIR/bridge/c \
    $$SOURCE_DIR/testing/js

INCLUDEPATH += $$WEBCORE_GENERATED_SOURCES_DIR

enable?(XSLT) {
    use?(LIBXML2) {
        mac {
            INCLUDEPATH += /usr/include/libxml2
            LIBS += -lxml2 -lxslt
        } else {
            PKGCONFIG += libxslt
        }
    } else {
        QT *= xmlpatterns
    }
}

use?(LIBXML2) {
    PKGCONFIG += libxml-2.0
}

use?(ZLIB) {
    LIBS += -lz
}

enable?(NETSCAPE_PLUGIN_API) {
    unix {
        mac {
            INCLUDEPATH += platform/mac
            # Note: XP_MACOSX is defined in npapi.h
        } else {
            xlibAvailable() {
                CONFIG *= x11
                LIBS += -lXrender
                DEFINES += MOZ_X11
            }
            DEFINES += XP_UNIX
            DEFINES += ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE=1
        }
    }
    win32-* {
        LIBS += \
            -ladvapi32 \
            -lgdi32 \
            -lshell32 \
            -lshlwapi \
            -luser32 \
            -lversion
    }
}

enable?(ORIENTATION_EVENTS)|enable?(DEVICE_ORIENTATION) {
    QT += sensors
}

use?(QT_MOBILITY_SYSTEMINFO) {
     CONFIG *= mobility
     MOBILITY *= systeminfo
}

enable?(GAMEPAD) {
    INCLUDEPATH += \
        $$SOURCE_DIR/platform/linux \
        $$SOURCE_DIR/Modules/gamepad
    PKGCONFIG += libudev
}

use?(GSTREAMER) {
    DEFINES += ENABLE_GLIB_SUPPORT=1
    PKGCONFIG += glib-2.0 gio-2.0 gstreamer-0.10 gstreamer-app-0.10 gstreamer-base-0.10 gstreamer-interfaces-0.10 gstreamer-pbutils-0.10 gstreamer-plugins-base-0.10 gstreamer-video-0.10
}

enable?(VIDEO) {
    use?(QTKIT) {
        INCLUDEPATH += $$SOURCE_DIR/platform/graphics/mac

        LIBS += -framework AppKit -framework AudioUnit \
                -framework AudioToolbox -framework CoreAudio \
                -framework QuartzCore -framework QTKit \
                -framework Security -framework IOKit

        # We can know the Mac OS version by using the Darwin major version
        DARWIN_VERSION = $$split(QMAKE_HOST.version, ".")
        DARWIN_MAJOR_VERSION = $$first(DARWIN_VERSION)
        equals(DARWIN_MAJOR_VERSION, "12") {
            LIBS += $${ROOT_WEBKIT_DIR}/WebKitLibraries/libWebKitSystemInterfaceMountainLion.a
        } else:equals(DARWIN_MAJOR_VERSION, "11") {
            LIBS += $${ROOT_WEBKIT_DIR}/WebKitLibraries/libWebKitSystemInterfaceLion.a
        } else:equals(DARWIN_MAJOR_VERSION, "10") {
            LIBS += $${ROOT_WEBKIT_DIR}/WebKitLibraries/libWebKitSystemInterfaceSnowLeopard.a
        } else:equals(DARWIN_MAJOR_VERSION, "9") {
            LIBS += $${ROOT_WEBKIT_DIR}/WebKitLibraries/libWebKitSystemInterfaceLeopard.a
        }
    } else:use?(GSTREAMER) {
        INCLUDEPATH += $$SOURCE_DIR/platform/graphics/gstreamer
    } else:use?(QT_MULTIMEDIA) {
        CONFIG   *= mobility
        MOBILITY *= multimedia
    }
}

enable?(WEB_AUDIO) {
    use?(GSTREAMER) {
        DEFINES += WTF_USE_WEBAUDIO_GSTREAMER=1
        INCLUDEPATH += $$SOURCE_DIR/platform/audio/gstreamer
        PKGCONFIG += gstreamer-audio-0.10 gstreamer-fft-0.10
    }
}

use?(3D_GRAPHICS) {
    contains(QT_CONFIG, opengles2):!win32: LIBS += -lEGL
}

use?(GRAPHICS_SURFACE) {
    mac: LIBS += -framework IOSurface -framework CoreFoundation
    linux-*: {
        LIBS += -lXcomposite -lXrender
        CONFIG *= x11
    }
}

!system-sqlite:exists( $${SQLITE3SRCDIR}/sqlite3.c ) {
    INCLUDEPATH += $${SQLITE3SRCDIR}
    DEFINES += SQLITE_CORE SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE
} else {
    INCLUDEPATH += $${SQLITE3SRCDIR}
    LIBS += -lsqlite3
}

use?(libjpeg): LIBS += -ljpeg
use?(libpng): LIBS += -lpng
use?(webp): LIBS += -lwebp

mac {
    LIBS += -framework Carbon -framework AppKit -framework IOKit
}

win32 {
    INCLUDEPATH += $$SOURCE_DIR/platform/win

    wince* {
        # see https://bugs.webkit.org/show_bug.cgi?id=43442
        DEFINES += HAVE_LOCALTIME_S=0

        LIBS += -lmmtimer
        LIBS += -lole32
    }
    else {
        LIBS += -lgdi32
        LIBS += -lole32
        LIBS += -luser32
    }
}

# Remove whole program optimizations due to miscompilations
win32-msvc2005|win32-msvc2008|win32-msvc2010|wince*:{
    QMAKE_CFLAGS_LTCG -= -GL
    QMAKE_CXXFLAGS_LTCG -= -GL

    # Disable incremental linking for windows 32bit OS debug build as WebKit is so big
    # that linker failes to link incrementally in debug mode.
    ARCH = $$(PROCESSOR_ARCHITECTURE)
    WOW64ARCH = $$(PROCESSOR_ARCHITEW6432)
    equals(ARCH, x86):{
        isEmpty(WOW64ARCH): QMAKE_LFLAGS_DEBUG += /INCREMENTAL:NO
    }
}

mac {
    LIBS_PRIVATE += -framework Carbon -framework AppKit
}

# -ffunction-section conflicts with -pg option
!contains(CONFIG, gprof) {
    unix:!mac:*-g++*:QMAKE_CXXFLAGS += -ffunction-sections
}
unix:!mac:*-g++*:QMAKE_CXXFLAGS += -fdata-sections
unix:!mac:*-g++*:QMAKE_LFLAGS += -Wl,--gc-sections
linux*-g++*:QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

unix|win32-g++* {
    QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui QtNetwork QtWidgets
}

# Disable C++0x mode in WebCore for those who enabled it in their Qt's mkspec
*-g++*:QMAKE_CXXFLAGS -= -std=c++0x -std=gnu++0x

enable_fast_mobile_scrolling: DEFINES += ENABLE_FAST_MOBILE_SCROLLING=1

