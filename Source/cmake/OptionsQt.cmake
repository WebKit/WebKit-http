include(FeatureSummary)
include(ECMPackageConfigHelpers)
include(ECMQueryQmake)

set(PROJECT_VERSION_MAJOR 5)
set(PROJECT_VERSION_MINOR 602)
set(PROJECT_VERSION_MICRO 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_MICRO})
set(PROJECT_VERSION_STRING "${PROJECT_VERSION}")

add_definitions(-DBUILDING_QT__=1)

if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(COMPILER_IS_GCC_OR_CLANG ON)
endif ()

WEBKIT_OPTION_BEGIN()

if (WIN32 OR APPLE)
    set(USE_LIBHYPHEN_DEFAULT OFF)
    set(USE_GSTREAMER_DEFAULT OFF)
    set(USE_QT_MULTIMEDIA_DEFAULT ON)
else ()
    set(USE_LIBHYPHEN_DEFAULT ON)
    set(USE_GSTREAMER_DEFAULT ON)
    set(USE_QT_MULTIMEDIA_DEFAULT OFF)
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(ENABLE_GAMEPAD_DEPRECATED_DEFAULT ON)
else ()
    set(ENABLE_GAMEPAD_DEPRECATED_DEFAULT OFF)
endif ()

if (WTF_CPU_X86_64 AND NOT WIN32)
    set(ENABLE_FTL_DEFAULT ON)
else ()
    set(ENABLE_FTL_DEFAULT OFF)
endif ()

WEBKIT_OPTION_DEFINE(USE_GSTREAMER "Use GStreamer implementation of MediaPlayer" PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFINE(USE_LIBHYPHEN "Use automatic hyphenation with LibHyphen" PUBLIC ${USE_LIBHYPHEN_DEFAULT})
WEBKIT_OPTION_DEFINE(USE_QT_MULTIMEDIA "Use Qt Multimedia implementation of MediaPlayer" PUBLIC ${USE_QT_MULTIMEDIA_DEFAULT})
WEBKIT_OPTION_DEFINE(ENABLE_INSPECTOR_UI "Include Inspector UI into resources" PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_PRINT_SUPPORT "Enable support for printing web pages" PUBLIC ON)

option(GENERATE_DOCUMENTATION "Generate HTML and QCH documentation" OFF)
option(ENABLE_TEST_SUPPORT "Build tools for running layout tests and related library code" ON)

# Public options shared with other WebKit ports. There must be strong reason
# to support changing the value of the option.
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ALLINONE_BUILD PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_API_TESTS PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_GRID_LAYOUT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DATABASE_PROCESS PUBLIC OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DATALIST_ELEMENT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DEVICE_ORIENTATION PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FULLSCREEN_API PUBLIC OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GAMEPAD_DEPRECATED PUBLIC ${ENABLE_GAMEPAD_DEPRECATED_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INDEXED_DATABASE PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LEGACY_WEB_AUDIO PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LINK_PREFETCH PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MEDIA_SOURCE PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_XSLT PUBLIC ON)

WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DRAG_SUPPORT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GEOLOCATION PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ICONDATABASE PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JIT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SAMPLING_PROFILER PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SPELLCHECK PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_TOUCH_EVENTS PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_VIDEO PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_AUDIO PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(USE_SYSTEM_MALLOC PUBLIC OFF)

# Private options shared with other WebKit ports. Add options here when
# we need a value different from the default defined in WebKitFeatures.cmake.
# Changing these options is completely unsupported.
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_IMAGE_SET PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_REGIONS PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_SHAPES PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_SELECTORS_LEVEL4 PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTL_JIT PRIVATE ${ENABLE_FTL_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTPDIR PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_COLOR PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MEDIA_CONTROLS_SCRIPT PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MHTML PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NOTIFICATIONS PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_USERSELECT_ALL PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_VIDEO_TRACK PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_TIMING PRIVATE ON)

WEBKIT_OPTION_DEPEND(ENABLE_MEDIA_SOURCE ENABLE_VIDEO)
WEBKIT_OPTION_DEPEND(USE_GSTREAMER ENABLE_VIDEO)
WEBKIT_OPTION_DEPEND(USE_QT_MULTIMEDIA ENABLE_VIDEO)

WEBKIT_OPTION_END()

# FTL JIT and IndexedDB support require GCC 4.9
# TODO: Patch code to avoid variadic lambdas
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (ENABLE_FTL_JIT OR ENABLE_INDEXED_DATABASE)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")
            message(FATAL_ERROR "GCC 4.9.0 is required to build QtWebKit with FTL JIT and Indexed Database, use a newer GCC version or clang, or disable these features")
        endif ()
    else ()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.0")
            message(FATAL_ERROR "GCC 4.8.0 is required to build QtWebKit, use a newer GCC version or clang")
        endif ()
    endif ()
endif ()

set(ENABLE_WEBKIT ON)
set(ENABLE_WEBKIT2 OFF)
set(WTF_USE_UDIS86 1)

if (SHARED_CORE)
    set(WebCoreTestSupport_LIBRARY_TYPE SHARED)
else ()
    set(JavaScriptCore_LIBRARY_TYPE STATIC)
    set(WebCoreTestSupport_LIBRARY_TYPE STATIC)
endif ()

SET_AND_EXPOSE_TO_BUILD(USE_TEXTURE_MAPPER TRUE)

if (WIN32)
    # bmalloc is not ported to Windows yet
    set(USE_SYSTEM_MALLOC 1)
    if (NOT WEBKIT_LIBRARIES_DIR)
        if (DEFINED ENV{WEBKIT_LIBRARIES})
            set(WEBKIT_LIBRARIES_DIR "$ENV{WEBKIT_LIBRARIES}")
        else ()
            set(WEBKIT_LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/WebKitLibraries/win")
        endif ()
    endif ()

    include_directories("${CMAKE_BINARY_DIR}/DerivedSources/ForwardingHeaders" "${CMAKE_BINARY_DIR}/DerivedSources" "${WEBKIT_LIBRARIES_DIR}/include")
    set(CMAKE_INCLUDE_PATH "${WEBKIT_LIBRARIES_DIR}/include")
    # bundled FindZlib is strange
    set(ZLIB_ROOT "${WEBKIT_LIBRARIES_DIR}/include")
    if (${MSVC_CXX_ARCHITECTURE_ID} STREQUAL "X86")
        link_directories("${CMAKE_BINARY_DIR}/lib32" "${WEBKIT_LIBRARIES_DIR}/lib32")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib32)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib32)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin32)
        set(CMAKE_LIBRARY_PATH "${WEBKIT_LIBRARIES_DIR}/lib32")
    else ()
        link_directories("${CMAKE_BINARY_DIR}/lib64" "${WEBKIT_LIBRARIES_DIR}/lib64")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin64)
        set(CMAKE_LIBRARY_PATH "${WEBKIT_LIBRARIES_DIR}/lib64")
    endif ()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif ()

find_package(LibXml2 2.8.0 REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(Sqlite REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if (NOT APPLE)
    find_package(ICU REQUIRED)
else ()
    set(ICU_INCLUDE_DIRS
        "${WEBCORE_DIR}/icu"
        "${JAVASCRIPTCORE_DIR}/icu"
        "${WTF_DIR}/icu"
    )
    set(ICU_LIBRARIES libicucore.dylib)

    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif ()

if (ENABLE_XSLT)
    find_package(LibXslt 1.1.7 REQUIRED)
endif ()

find_package(Fontconfig)

if (FONTCONFIG_FOUND)
    SET_AND_EXPOSE_TO_BUILD(HAVE_FONTCONFIG 1)
endif ()

find_package(WebP)

if (WEBP_FOUND)
    SET_AND_EXPOSE_TO_BUILD(USE_WEBP 1)
endif ()

set(REQUIRED_QT_VERSION 5.2.0)

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Network Sql)

# FIXME: Allow building w/o these components
find_package(Qt5OpenGL ${REQUIRED_QT_VERSION})
find_package(Qt5Test ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Widgets ${REQUIRED_QT_VERSION} REQUIRED)

if (COMPILER_IS_GCC_OR_CLANG AND UNIX)
    if (APPLE OR CMAKE_SYSTEM_NAME MATCHES "Android" OR ${Qt5_VERSION} VERSION_LESS 5.6)
        set(USE_LINKER_VERSION_SCRIPT_DEFAULT OFF)
    else ()
        set(USE_LINKER_VERSION_SCRIPT_DEFAULT ON)
    endif ()
else ()
    set(USE_LINKER_VERSION_SCRIPT_DEFAULT OFF)
endif ()

option(USE_LINKER_VERSION_SCRIPT "Use linker script for ABI compatibility with Qt libraries" ${USE_LINKER_VERSION_SCRIPT_DEFAULT})

if (ENABLE_GEOLOCATION)
    find_package(Qt5Positioning ${REQUIRED_QT_VERSION} REQUIRED)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QTPOSITIONING 1)
endif ()

if (ENABLE_DEVICE_ORIENTATION)
    find_package(Qt5Sensors ${REQUIRED_QT_VERSION} REQUIRED)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QTSENSORS 1)
endif ()

if (ENABLE_PRINT_SUPPORT)
    find_package(Qt5PrintSupport ${REQUIRED_QT_VERSION} REQUIRED)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QTPRINTSUPPORT 1)
endif ()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# TODO: figure out if we can run automoc only on Qt sources

# From OptionsEfl.cmake
# Optimize binary size for release builds by removing dead sections on unix/gcc.
if (COMPILER_IS_GCC_OR_CLANG AND UNIX AND NOT APPLE)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffunction-sections -fdata-sections -fno-rtti")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")

    if (NOT SHARED_CORE)
        set(CMAKE_C_FLAGS "-fvisibility=hidden ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS}")
    endif ()
endif ()


if (USE_GSTREAMER)
    SET_AND_EXPOSE_TO_BUILD(USE_GLIB 1)
    find_package(GLIB 2.36 REQUIRED COMPONENTS gio gobject)
endif ()

if (USE_QT_MULTIMEDIA)
    find_package(Qt5Multimedia ${REQUIRED_QT_VERSION} REQUIRED)
    # FIXME: Allow building w/o widgets
    find_package(Qt5MultimediaWidgets ${REQUIRED_QT_VERSION} REQUIRED)
endif ()

# From OptionsGTK.cmake
# FIXME: Refactor to avoid duplication
if (USE_GSTREAMER AND (ENABLE_VIDEO OR ENABLE_WEB_AUDIO))
    set(GSTREAMER_COMPONENTS app pbutils)

    if (ENABLE_VIDEO)
        list(APPEND GSTREAMER_COMPONENTS video mpegts tag gl)
    endif ()

    if (ENABLE_WEB_AUDIO)
        list(APPEND GSTREAMER_COMPONENTS audio fft)
    endif ()

    find_package(GStreamer 1.0.3 REQUIRED COMPONENTS ${GSTREAMER_COMPONENTS})

    if (ENABLE_WEB_AUDIO)
        if (NOT PC_GSTREAMER_AUDIO_FOUND OR NOT PC_GSTREAMER_FFT_FOUND)
            message(FATAL_ERROR "WebAudio requires the audio and fft GStreamer libraries. Please check your gst-plugins-base installation.")
        else ()
            SET_AND_EXPOSE_TO_BUILD(USE_WEBAUDIO_GSTREAMER TRUE)
        endif ()
    endif ()

    if (ENABLE_VIDEO)
        if (NOT PC_GSTREAMER_APP_FOUND OR NOT PC_GSTREAMER_PBUTILS_FOUND OR NOT PC_GSTREAMER_TAG_FOUND OR NOT PC_GSTREAMER_VIDEO_FOUND)
            message(FATAL_ERROR "Video playback requires the following GStreamer libraries: app, pbutils, tag, video. Please check your gst-plugins-base installation.")
        endif ()
    endif ()

    if (USE_GSTREAMER_MPEGTS)
        if (NOT PC_GSTREAMER_MPEGTS_FOUND)
            message(FATAL_ERROR "GStreamer MPEG-TS is needed for USE_GSTREAMER_MPEGTS.")
        endif ()
    endif ()

    if (USE_GSTREAMER_GL)
        if (NOT PC_GSTREAMER_GL_FOUND)
            message(FATAL_ERROR "GStreamerGL is needed for USE_GSTREAMER_GL.")
        endif ()
    endif ()
endif ()

if (USE_LIBHYPHEN)
    find_package(Hyphen REQUIRED)
    if (NOT HYPHEN_FOUND)
       message(FATAL_ERROR "libhyphen is needed for USE_LIBHYPHEN.")
    endif ()
endif ()

# From OptionsGTK.cmake
if (CMAKE_MAJOR_VERSION LESS 3)
    # Before CMake 3 it was necessary to use a build script instead of using cmake --build directly
    # to preserve colors and pretty-printing.

    build_command(COMMAND_LINE_TO_BUILD)
    # build_command unconditionally adds -i (ignore errors) for make, and there's
    # no reasonable way to turn that off, so we just replace it with -k, which has
    # the same effect, except that the return code will indicate that an error occurred.
    # See: http://www.cmake.org/cmake/help/v3.0/command/build_command.html
    string(REPLACE " -i" " -k" COMMAND_LINE_TO_BUILD ${COMMAND_LINE_TO_BUILD})
    file(WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/build.sh
        "#!/bin/sh\n"
        "${COMMAND_LINE_TO_BUILD} $@"
    )
    file(COPY ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/build.sh
        DESTINATION ${CMAKE_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    )
endif ()

# You can build JavaScriptCore as a static library if you specify it as STATIC
# set(JavaScriptCore_LIBRARY_TYPE STATIC)

# From OptionsWin.cmake
if (MSVC)
    add_definitions(-DNOMINMAX -DUNICODE -D_UNICODE -D_WINDOWS -DWINVER=0x601)

    add_definitions(
        /wd4018 /wd4068 /wd4099 /wd4100 /wd4127 /wd4138 /wd4146 /wd4180 /wd4189
        /wd4201 /wd4244 /wd4251 /wd4267 /wd4275 /wd4288 /wd4291 /wd4305 /wd4309
        /wd4344 /wd4355 /wd4389 /wd4396 /wd4456 /wd4457 /wd4458 /wd4459 /wd4481
        /wd4503 /wd4505 /wd4510 /wd4512 /wd4530 /wd4577 /wd4610 /wd4611 /wd4702
        /wd4706 /wd4800 /wd4819 /wd4951 /wd4952 /wd4996 /wd6011 /wd6031 /wd6211
        /wd6246 /wd6255 /wd6387
    )

    # Create pdb files for debugging purposes, also for Release builds
    add_compile_options(/Zi /GS)

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")

    # We do not use exceptions
    add_definitions(-D_HAS_EXCEPTIONS=0)
    add_compile_options(/EHa- /EHc- /EHs- /fp:except-)

    # We have some very large object files that have to be linked
    add_compile_options(/analyze- /bigobj)

    # Use CRT security features
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)

    # Turn off certain link features
    add_compile_options(/Gy- /openmp- /GF-)

    # Turn off some linker warnings
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4049 /ignore:4217")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")

    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:NOREF /OPT:NOICF")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:NOREF /OPT:NOICF")

        # To debug linking time issues, uncomment the following three lines:
        #add_compile_options(/Bv)
        #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /VERBOSE /VERBOSE:INCR /TIME")
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /VERBOSE /VERBOSE:INCR /TIME")

        # enable fast link for >= MSVC2015
        if ((MSVC_VERSION GREATER 1900) OR (MSVC_VERSION EQUAL 1900))
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG:FASTLINK")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG:FASTLINK")
        endif ()

    elseif (${CMAKE_BUILD_TYPE} MATCHES "Release")
        add_compile_options(/Oy-)
    endif ()

    if (NOT ${CMAKE_GENERATOR} MATCHES "Ninja")
        link_directories("${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}")
        add_definitions(/MP)
    endif ()
    if (NOT ${CMAKE_CXX_FLAGS} STREQUAL "")
        string(REGEX REPLACE "(/EH[a-z]+) " "\\1- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable C++ exceptions
        string(REGEX REPLACE "/EHsc$" "/EHs- /EHc- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable C++ exceptions
        string(REGEX REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable RTTI
        string(REGEX REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Warnings are important
    endif ()

    foreach (flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        # Use the multithreaded static runtime library instead of the default DLL runtime.
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach ()
endif ()

if (NOT RUBY_FOUND AND RUBY_EXECUTABLE AND NOT RUBY_VERSION VERSION_LESS 1.9)
    get_property(_packages_found GLOBAL PROPERTY PACKAGES_FOUND)
    list(APPEND _packages_found Ruby)
    set_property(GLOBAL PROPERTY PACKAGES_FOUND ${_packages_found})

    get_property(_packages_not_found GLOBAL PROPERTY PACKAGES_NOT_FOUND)
    list(REMOVE_ITEM _packages_not_found Ruby)
    set_property(GLOBAL PROPERTY PACKAGES_NOT_FOUND ${_packages_not_found})
endif ()

set_package_properties(Ruby PROPERTIES TYPE REQUIRED)
set_package_properties(Qt5PrintSupport PROPERTIES PURPOSE "Required for ENABLE_PRINT_SUPPORT=ON")
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
