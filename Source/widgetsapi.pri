# -------------------------------------------------------------------
# Target file for the QtWebKitWidgets dynamic library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

# Use Qt5's module system
load(qt_build_config)

TEMPLATE = lib
TARGET = QtWebKitWidgets

WEBKIT_DESTDIR = $${ROOT_BUILD_DIR}/lib

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source/WebKit

INCLUDEPATH += \
    $$SOURCE_DIR/qt/Api \
    $$SOURCE_DIR/qt/WidgetApi \
    $$SOURCE_DIR/qt/WebCoreSupport \
    $$SOURCE_DIR/qt/WidgetSupport \
    $$ROOT_WEBKIT_DIR/Source/WTF/wtf/qt

enable?(DEVICE_ORIENTATION)|enable?(ORIENTATION_EVENTS) {
    QT += sensors
}

enable?(GEOLOCATION): QT += location

use?(QT_MULTIMEDIA): QT *= multimediawidgets

contains(CONFIG, texmap): DEFINES += WTF_USE_TEXTURE_MAPPER=1

use?(PLUGIN_BACKEND_XLIB): PKGCONFIG += x11

QT += network widgets
have?(QTQUICK): QT += quick
have?(QTPRINTSUPPORT): QT += printsupport

use?(TEXTURE_MAPPER_GL)|enable?(WEBGL) {
    QT *= opengl
}
QT += webkit

use?(3D_GRAPHICS): WEBKIT += ANGLE

WEBKIT += javascriptcore wtf webcore

MODULE = webkitwidgets

# We want the QtWebKit API forwarding includes to live in the root build dir.
MODULE_BASE_DIR = $$_PRO_FILE_PWD_
MODULE_BASE_OUTDIR = $$ROOT_BUILD_DIR

# This is the canonical list of dependencies for the public API of
# the QtWebKitWidgets library, and will end up in the library's prl file.
QT_API_DEPENDS = core gui widgets network webkit

# ---------------- Custom developer-build handling -------------------
#
# The assumption for Qt developer builds is that the module file
# will be put into qtbase/mkspecs/modules, and the libraries into
# qtbase/lib, with rpath/install_name set to the Qt lib dir.
#
# For WebKit we don't want that behavior for the libraries, as we want
# them to be self-contained in the WebKit build dir.
#
!production_build: CONFIG += force_independent

BASE_TARGET = $$TARGET

load(qt_module)

# Allow doing a debug-only build of WebKit (not supported by Qt)
macx:!debug_and_release:debug: TARGET = $$BASE_TARGET

SOURCES += \
    $$PWD/WebKit/qt/WidgetApi/qgraphicswebview.cpp \
    $$PWD/WebKit/qt/WidgetApi/qwebframe.cpp \
    $$PWD/WebKit/qt/WidgetApi/qwebpage.cpp \
    $$PWD/WebKit/qt/WidgetApi/qwebview.cpp \
    $$PWD/WebKit/qt/WidgetApi/qwebinspector.cpp \
    $$PWD/WebKit/qt/WidgetSupport/QtFallbackWebPopup.cpp \
    $$PWD/WebKit/qt/WidgetSupport/QtWebComboBox.cpp \
    $$PWD/WebKit/qt/WidgetSupport/QWebUndoCommand.cpp \
    $$PWD/WebKit/qt/WidgetSupport/DefaultFullScreenVideoHandler.cpp \
    $$PWD/WebKit/qt/WidgetSupport/InitWebKitQt.cpp \
    $$PWD/WebKit/qt/WidgetSupport/InspectorClientWebPage.cpp \
    $$PWD/WebKit/qt/WidgetSupport/PageClientQt.cpp \
    $$PWD/WebKit/qt/WidgetSupport/QStyleFacadeImp.cpp \
    $$PWD/WebKit/qt/WidgetSupport/QGraphicsWidgetPluginImpl.cpp \
    $$PWD/WebKit/qt/WidgetSupport/QWidgetPluginImpl.cpp

HEADERS += \
    $$PWD/WebKit/qt/WidgetApi/qgraphicswebview.h \
    $$PWD/WebKit/qt/WidgetApi/qwebframe.h \
    $$PWD/WebKit/qt/WidgetApi/qwebframe_p.h \
    $$PWD/WebKit/qt/WidgetApi/qwebpage.h \
    $$PWD/WebKit/qt/WidgetApi/qwebpage_p.h \
    $$PWD/WebKit/qt/WidgetApi/qwebview.h \
    $$PWD/WebKit/qt/WidgetApi/qwebinspector.h \
    $$PWD/WebKit/qt/WidgetApi/qwebinspector_p.h \
    $$PWD/WebKit/qt/Api/qwebkitplatformplugin.h \
    $$PWD/WebKit/qt/WidgetSupport/InitWebKitQt.h \
    $$PWD/WebKit/qt/WidgetSupport/InspectorClientWebPage.h \
    $$PWD/WebKit/qt/WidgetSupport/DefaultFullScreenVideoHandler.h \
    $$PWD/WebKit/qt/WidgetSupport/QtFallbackWebPopup.h \
    $$PWD/WebKit/qt/WidgetSupport/QtWebComboBox.h \
    $$PWD/WebKit/qt/WidgetSupport/QWebUndoCommand.h \
    $$PWD/WebKit/qt/WidgetSupport/PageClientQt.h \
    $$PWD/WebKit/qt/WidgetSupport/QGraphicsWidgetPluginImpl.h \
    $$PWD/WebKit/qt/WidgetSupport/QStyleFacadeImp.h \
    $$PWD/WebKit/qt/WidgetSupport/QWidgetPluginImpl.h \

contains(QT_CONFIG, accessibility) {
    SOURCES += $$PWD/WebKit/qt/WidgetApi/qwebviewaccessible.cpp
    HEADERS += $$PWD/WebKit/qt/WidgetApi/qwebviewaccessible_p.h 
}

INCLUDEPATH += \
    $$PWD/qt/Api \
    $$PWD/qt/WebCoreSupport

enable?(VIDEO) {
    !use?(QTKIT):!use?(GSTREAMER):use?(QT_MULTIMEDIA) {
        HEADERS += $$PWD/WebKit/qt/WidgetSupport/FullScreenVideoWidget.h
        SOURCES += $$PWD/WebKit/qt/WidgetSupport/FullScreenVideoWidget.cpp
    }
}

