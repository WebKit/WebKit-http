# -------------------------------------------------------------------
# Project file for WebKitTestRunner's InjectedBundle
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = WTRInjectedBundle

mac:debug_and_release:TARGET = $$qtLibraryTarget($$TARGET)

SOURCES += \
    AccessibilityController.cpp \
    AccessibilityTextMarker.cpp \
    AccessibilityTextMarkerRange.cpp \
    AccessibilityUIElement.cpp \
    InjectedBundle.cpp \
    InjectedBundle.h \
    InjectedBundleMain.cpp \
    InjectedBundlePage.cpp \
    InjectedBundlePage.h \
    EventSendingController.cpp \
    EventSendingController.h \
    GCController.cpp \
    GCController.h \
    LayoutTestController.cpp \
    LayoutTestController.h \
    TextInputController.cpp \
    TextInputController.h \
    Bindings/JSWrapper.cpp \
    qt/ActivateFontsQt.cpp \
    qt/InjectedBundleQt.cpp \
    qt/LayoutTestControllerQt.cpp

# Adds the generated sources to SOURCES
include(DerivedSources.pri)

HEADERS += \
    AccessibilityController.h \
    AccessibilityTextMarker.h \
    AccessibilityTextMarkerRange.h \
    AccessibilityUIElement.h \
    ActivateFonts.h \
    EventSendingController.h \
    GCController.h \
    InjectedBundle.h \
    InjectedBundlePage.h \
    LayoutTestController.h \
    TextInputController.h \

DESTDIR = $${ROOT_BUILD_DIR}/lib

QT += declarative widgets

load(features)
load(wtf)
load(javascriptcore)
load(webcore)

CONFIG += plugin qtwebkit

INCLUDEPATH += \
    $$PWD/.. \
    $$PWD/Bindings \
    $${ROOT_WEBKIT_DIR}/Source/WebCore/testing/js

PREFIX_HEADER = $$PWD/../WebKitTestRunnerPrefix.h
*-g++*:QMAKE_CXXFLAGS += "-include $$PREFIX_HEADER"

!embedded: PKGCONFIG += fontconfig

linux-* {
    QMAKE_LFLAGS += -Wl,--no-undefined
}
