load(functions)

TEMPLATE = subdirs
CONFIG += ordered
PROJECTS_DIR = Tools/qmake/projects

isPlatformSupported() {
    QT_FOR_CONFIG += gui-private
    !qtConfig(system-png):qtConfig(png):exists($$QTBASE_DIR): \
        SUBDIRS += $$PROJECTS_DIR/qtpng

    !qtConfig(system-jpeg):exists($$QTBASE_DIR): \
        SUBDIRS += $$PROJECTS_DIR/qtjpeg

    SUBDIRS += \
        $$PROJECTS_DIR/generate_cmake_toolchain_file.pro \
        $$PROJECTS_DIR/run_cmake.pro
} else {
    !build_pass: log("$${EOL}The WebKit build was disabled for the following reasons: $$skipBuildReason $${EOL}$${EOL}")
}
