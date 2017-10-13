load(functions)

ROOT_QT_BUILD_DIR = $$ROOT_BUILD_DIR/..

TEMPLATE = aux

qtConfig(debug_and_release): CONFIG += debug_and_release build_all

CONFIG(debug, debug|release) {
    configuration = Debug
} else {
    configuration = Release
}

programExistsInPath(ninja): use_ninja = 1

cmake_build_dir = $$system_quote($$system_path($$ROOT_BUILD_DIR/$$lower($$configuration)))
toolchain_file = $$system_quote($$system_path($$ROOT_BUILD_DIR/qmake_toolchain.cmake))

build_pass|!debug_and_release {
    CMAKE_CONFIG += \
        PORT=Qt \
        CMAKE_BUILD_TYPE=$$configuration \
        CMAKE_TOOLCHAIN_FILE=$$toolchain_file \
        USE_LIBHYPHEN=OFF

    !isEmpty(_QMAKE_SUPER_CACHE_) {
        CMAKE_PREFIX_PATH=\"$$ROOT_QT_BUILD_DIR/qtbase;$$ROOT_QT_BUILD_DIR/qtlocation;$$ROOT_QT_BUILD_DIR/qtsensors;$$ROOT_QT_BUILD_DIR/qtdeclarative;$$ROOT_QT_BUILD_DIR/qtwebchannel\"
    } else {
        CMAKE_PREFIX_PATH=\"$$[QT_INSTALL_PREFIX]\"
    }
    CMAKE_CONFIG += CMAKE_PREFIX_PATH=$$CMAKE_PREFIX_PATH

    static: CMAKE_CONFIG += USE_THIN_ARCHIVES=OFF

    static_runtime: CMAKE_CONFIG += USE_STATIC_RUNTIME=ON

    QT_FOR_CONFIG += gui-private
    !qtConfig(system-jpeg):exists($$QTBASE_DIR) {
        CMAKE_CONFIG += \
            QT_BUNDLED_JPEG=1 \
            JPEG_INCLUDE_DIR=$$QTBASE_DIR/src/3rdparty/libjpeg \
            JPEG_LIBRARIES=$$staticLibPath(qtjpeg)
    }

    !qtConfig(system-png):qtConfig(png):exists($$QTBASE_DIR) {
        CMAKE_CONFIG += \
            QT_BUNDLED_PNG=1 \
            PNG_INCLUDE_DIRS=$$QTBASE_DIR/src/3rdparty/libpng \
            PNG_LIBRARIES=$$staticLibPath(qtpng)
    }

    !qtConfig(system-zlib):exists($$QTBASE_DIR) {
        CMAKE_CONFIG += QT_BUNDLED_ZLIB=1

        exists($$QTBASE_DIR/src/3rdparty/zlib/src/zlib.h): \
            CMAKE_CONFIG += ZLIB_INCLUDE_DIRS=$$QTBASE_DIR/src/3rdparty/zlib/src
        else: CMAKE_CONFIG += ZLIB_INCLUDE_DIRS=$$QTBASE_DIR/src/3rdparty/zlib
    }

    qtConfig(opengles2):!qtConfig(dynamicgl) {
        CMAKE_CONFIG += QT_USES_GLES2_ONLY=1
    }

    exists($$ROOT_BUILD_DIR/conanbuildinfo.cmake):exists($$ROOT_BUILD_DIR/conanfile.txt) {
        CMAKE_CONFIG += QT_CONAN_DIR=$$ROOT_BUILD_DIR
    }

    msvc:!contains(QMAKE_HOST.arch, x86_64) {
        CMAKE_CONFIG += USE_MINIMAL_DEBUG_INFO_MSVC=ON
    }

    macos {
        # Reuse the cached sdk version value from mac/sdk.prf if available
        # otherwise query for it.
        QMAKE_MAC_SDK_PATH = $$eval(QMAKE_MAC_SDK.$${QMAKE_MAC_SDK}.Path)
        isEmpty(QMAKE_MAC_SDK_PATH) {
            QMAKE_MAC_SDK_PATH = $$system("/usr/bin/xcodebuild -sdk $${QMAKE_MAC_SDK} -version Path 2>/dev/null")
        }
        exists($$QMAKE_MAC_SDK_PATH): CMAKE_CONFIG += CMAKE_OSX_SYSROOT=$$QMAKE_MAC_SDK_PATH
        !isEmpty(QMAKE_MACOSX_DEPLOYMENT_TARGET): CMAKE_CONFIG += CMAKE_OSX_DEPLOYMENT_TARGET=$$QMAKE_MACOSX_DEPLOYMENT_TARGET

        # Hack: install frameworks in debug_and_release in separate prefixes
        debug_and_release:build_all:CONFIG(debug, debug|release) {
            CMAKE_CONFIG += CMAKE_INSTALL_PREFIX=\"$$[QT_INSTALL_PREFIX]/debug\"
        }
    }

    !isEmpty(use_ninja): cmake_args += "-G Ninja"

    equals(QMAKE_HOST.os, Windows) {
        if(equals(MAKEFILE_GENERATOR, MSVC.NET)|equals(MAKEFILE_GENERATOR, MSBUILD)) {
            cmake_generator = "NMake Makefiles JOM"
            make_command_name = jom
        } else: if(equals(MAKEFILE_GENERATOR, MINGW)) {
            cmake_generator = "MinGW Makefiles"
            make_command_name = make
        } else {
            cmake_generator = "Unix Makefiles"
            make_command_name = make
        }
        isEmpty(use_ninja): cmake_args += "-G \"$$cmake_generator\""
    } else {
        make_command_name = make
    }

    !silent {
        make_args += "VERBOSE=1"
        ninja_args += "-v"
    }

    # Append additional platform options defined in CMAKE_CONFIG
    for (config, CMAKE_CONFIG): cmake_args += "-D$$config"

    !exists($$cmake_build_dir) {
        !system("$$QMAKE_MKDIR $$cmake_build_dir"): error("Failed to create cmake build directory")
    }

    cmake_cmd_base = "cd $$cmake_build_dir &&"

    log("$${EOL}Running $$cmake_env cmake $$ROOT_WEBKIT_DIR $$cmake_args $${EOL}$${EOL}")
    !system("$$cmake_cmd_base $$cmake_env cmake $$ROOT_WEBKIT_DIR $$cmake_args"): error("Running cmake failed")

    log("$${EOL}WebKit is now configured for building. Just run '$$make_command_name'.$${EOL}$${EOL}")


    build_pass:build_all: default_target.target = all
    else: default_target.target = first

    isEmpty(use_ninja) {
        default_target.commands = cd $$cmake_build_dir && $(MAKE) $$make_args
    } else {
        default_target.commands = cd $$cmake_build_dir && ninja $$ninja_args
    }
    QMAKE_EXTRA_TARGETS += default_target

    # When debug and release are built at the same time, don't install data files twice
    debug_and_release:build_all:CONFIG(debug, debug|release): cmake_install_args = "-DCOMPONENT=Code"

    install_impl_target.target = install_impl
    install_impl_target.commands = cd $$cmake_build_dir && cmake $$cmake_install_args -P cmake_install.cmake
    QMAKE_EXTRA_TARGETS += install_impl_target

    install_target.target = install
    install_target.commands = $(MAKE) -f $(MAKEFILE) install_impl $$make_args DESTDIR=$(INSTALL_ROOT)
    QMAKE_EXTRA_TARGETS += install_target
}

!build_pass:debug_and_release:!isEmpty(use_ninja) {
    # Special GNU make target for the meta Makefile that ensures that our
    # debug_and_release Makefiles won't both run ninja in parallel.
    notParallel.target = .NOTPARALLEL
    QMAKE_EXTRA_TARGETS += notParallel
}
