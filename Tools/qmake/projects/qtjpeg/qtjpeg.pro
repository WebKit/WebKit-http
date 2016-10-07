load(functions)

TARGET = qtjpeg

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off \
    installed

# Poor man's qt_helper_lib
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

# In debug_and_release build we need only one copy of library, let it be release
debug_and_release {
    CONFIG -= debug_and_release debug
    CONFIG += release
}

DESTDIR = $$ROOT_BUILD_DIR/lib

include($$QTBASE_DIR/src/3rdparty/libjpeg.pri)
