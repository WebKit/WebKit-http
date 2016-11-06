load(functions)

TEMPLATE = aux

cross_compile {
    linux {
        SYSTEM_NAME = Linux
    } else: macos {
        # We don't support uikit platforms for now, and they certainly won't work
        # with the simplistic toolchain file we are generating here.
        SYSTEM_NAME = Darwin
    } else: win32 {
        SYSTEM_NAME = Windows
    } else: freebsd {
        SYSTEM_NAME = FreeBSD
    } else: netbsd {
        SYSTEM_NAME = NetBSD
    } else: openbsd {
        SYSTEM_NAME = OpenBSD
    } else {
        error("Unknown platform, cannot produce toolchain file for CMake")
    }

    TOOLCHAIN_FILE_VARS += \
        "CMAKE_SYSTEM_NAME $$SYSTEM_NAME" \
        "CMAKE_SYSTEM_PROCESSOR $$QT_ARCH"

    isEmpty(CMAKE_SYSROOT): CMAKE_SYSROOT = $$[QT_SYSROOT]
    !isEmpty(CMAKE_SYSROOT): TOOLCHAIN_FILE_VARS += \
        "CMAKE_FIND_ROOT_PATH $$CMAKE_SYSROOT" \
        "CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER" \
        "CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY" \
        "CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY"
}

TOOLCHAIN_FILE_VARS += \
    "CMAKE_C_COMPILER \"$$QMAKE_CC\"" \
    "CMAKE_CXX_COMPILER \"$$QMAKE_CXX\""

CONANBUILDINFO_PATH = $$ROOT_BUILD_DIR/conanbuildinfo.cmake
exists($$CONANBUILDINFO_PATH): TOOLCHAIN_FILE_VARS += "CONANBUILDINFO_PATH $$CONANBUILDINFO_PATH"

for (var, TOOLCHAIN_FILE_VARS): TOOLCHAIN_FILE_CONTENTS += "set($$var)"

TOOLCHAIN_FILE = $$ROOT_BUILD_DIR/qmake_toolchain.cmake

!build_pass {
    log("$${EOL}Generating CMake toolchain file $$TOOLCHAIN_FILE $${EOL}$${EOL}"$$join(TOOLCHAIN_FILE_CONTENTS, $${EOL})$${EOL}$${EOL})
    write_file($$TOOLCHAIN_FILE, TOOLCHAIN_FILE_CONTENTS)|error()
}
