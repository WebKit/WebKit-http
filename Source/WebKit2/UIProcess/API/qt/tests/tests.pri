TEMPLATE = app

VPATH += $$_PRO_FILE_PWD_
TARGET = tst_$$TARGET

HEADERS += ../bytearraytestdata.h

SOURCES += $${TARGET}.cpp \
           ../util.cpp \
           ../bytearraytestdata.cpp
INCLUDEPATH += $$PWD

QT += testlib declarative widgets quick

CONFIG += qtwebkit

DEFINES += TESTS_SOURCE_DIR=\\\"$$PWD\\\" \
           QWP_PATH=\\\"$${ROOT_BUILD_DIR}/bin\\\"
