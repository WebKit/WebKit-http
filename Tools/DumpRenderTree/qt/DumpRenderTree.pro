# -------------------------------------------------------------------
# Project file for the DumpRenderTree binary (DRT)
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

TARGET = DumpRenderTree
DESTDIR = $$ROOT_BUILD_DIR/bin

WEBKIT += wtf javascriptcore webcore

INCLUDEPATH += \
    $$PWD/ \
    $$PWD/.. \
    $${ROOT_WEBKIT_DIR}/Source/WebKit/qt/WebCoreSupport \
    $${ROOT_WEBKIT_DIR}/Source/WTF

QT = core gui network testlib webkit widgets
have?(QTPRINTSUPPORT): QT += printsupport
macx: QT += xml

have?(FONTCONFIG): PKGCONFIG += fontconfig

HEADERS += \
    $$PWD/../WorkQueue.h \
    $$PWD/../DumpRenderTree.h \
    DumpRenderTreeQt.h \
    EventSenderQt.h \
    TextInputControllerQt.h \
    WorkQueueItemQt.h \
    TestRunnerQt.h \
    GCControllerQt.h \
    QtInitializeTestFonts.h \
    testplugin.h

SOURCES += \
    $$PWD/../WorkQueue.cpp \
    $$PWD/../DumpRenderTreeCommon.cpp \
    DumpRenderTreeQt.cpp \
    EventSenderQt.cpp \
    TextInputControllerQt.cpp \
    WorkQueueItemQt.cpp \
    TestRunnerQt.cpp \
    GCControllerQt.cpp \
    QtInitializeTestFonts.cpp \
    testplugin.cpp \
    DumpRenderTreeMain.cpp

wince*: {
    INCLUDEPATH += $$QT.core.sources/../3rdparty/ce-compat $$WCECOMPAT/include
    LIBS += $$WCECOMPAT/lib/wcecompat.lib
}

DEFINES -= USE_SYSTEM_MALLOC=0
DEFINES += USE_SYSTEM_MALLOC=1

RESOURCES = DumpRenderTree.qrc
