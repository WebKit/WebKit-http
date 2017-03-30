load(functions)

TARGET = qtjpeg

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off

load(qt_helper_lib)

DESTDIR = $$ROOT_BUILD_DIR/lib

include($$QTBASE_DIR/src/3rdparty/libjpeg.pri)
