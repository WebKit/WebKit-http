TEMPLATE = app
TARGET = QtWebProcess
INSTALLS += target

isEmpty(INSTALL_BINS) {
    target.path = $$[QT_INSTALL_BINS]
} else {
    target.path = $$INSTALL_BINS
}

SOURCES += \
    qt/MainQt.cpp

isEmpty(OUTPUT_DIR): OUTPUT_DIR = ../..
include($$PWD/../WebKit.pri)

DESTDIR = $$OUTPUT_DIR/bin
!CONFIG(standalone_package): CONFIG -= app_bundle

QT += network
macx:QT+=xml

linux-* {
    # From Creator's src/rpath.pri:
    # Do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    # this expands to $ORIGIN (after qmake and make), it does NOT read a qmake var.
    QMAKE_RPATHDIR = \$\$ORIGIN/../lib $$QMAKE_RPATHDIR
    MY_RPATH = $$join(QMAKE_RPATHDIR, ":")

    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${MY_RPATH}\' -Wl,--no-undefined
    QMAKE_RPATHDIR =
} else {
    QMAKE_RPATHDIR = $$OUTPUT_DIR/lib $$QMAKE_RPATHDIR
}

contains(QT_CONFIG, opengl) {
    QT += opengl
    DEFINES += QT_CONFIGURED_WITH_OPENGL
}
