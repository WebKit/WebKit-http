include(FeatureSummary)
include(ECMEnableSanitizers)
include(ECMPackageConfigHelpers)

set(ECM_MODULE_DIR ${CMAKE_MODULE_PATH})

set(PROJECT_VERSION_MAJOR 5)
set(PROJECT_VERSION_MINOR 212)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(PROJECT_VERSION_STRING "${PROJECT_VERSION}")

set(QT_CONAN_DIR "" CACHE PATH "Directory containing conanbuildinfo.cmake and conanfile.txt")
if (QT_CONAN_DIR)
    include("${QT_CONAN_DIR}/conanbuildinfo.cmake")
    conan_basic_setup()

    install(CODE "
        set(_conan_imports_dest \${CMAKE_INSTALL_PREFIX})
        if (DEFINED ENV{DESTDIR})
            get_filename_component(_absolute_destdir \$ENV{DESTDIR} ABSOLUTE)
            string(REGEX REPLACE \"^[A-z]:\" \"\" _conan_imports_dest \${CMAKE_INSTALL_PREFIX})
            set(_conan_imports_dest \"\${_absolute_destdir}\${_conan_imports_dest}\")
        endif ()

        execute_process(
            COMMAND conan imports -f \"${QT_CONAN_DIR}/conanfile.txt\" --dest \${_conan_imports_dest}
            WORKING_DIRECTORY \"${QT_CONAN_DIR}\"
        )

        set(_conan_imports_manifest \"\${_conan_imports_dest}/conan_imports_manifest.txt\")
        if (EXISTS \${_conan_imports_manifest})
            file(REMOVE \${_conan_imports_manifest})
            message(\"Removed conan install manifest: \${_conan_imports_manifest}\")
        endif ()
    ")
endif ()

set(STATIC_DEPENDENCIES_CMAKE_FILE "${CMAKE_BINARY_DIR}/QtStaticDependencies.cmake")
if (EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
    file(REMOVE ${STATIC_DEPENDENCIES_CMAKE_FILE})
endif ()

macro(CONVERT_PRL_LIBS_TO_CMAKE _qt_component)
    if (TARGET Qt5::${_qt_component})
        get_target_property(_lib_location Qt5::${_qt_component} LOCATION)
        execute_process(COMMAND ${PERL_EXECUTABLE} ${TOOLS_DIR}/qt/convert-prl-libs-to-cmake.pl
            --lib ${_lib_location}
            --out ${STATIC_DEPENDENCIES_CMAKE_FILE}
            --component ${_qt_component}
            --compiler ${CMAKE_CXX_COMPILER_ID}
        )
    endif ()
endmacro()

macro(QT_ADD_EXTRA_WEBKIT_TARGET_EXPORT target)
    if (QT_STATIC_BUILD OR SHARED_CORE)
        install(TARGETS ${target} EXPORT WebKitTargets
            DESTINATION "${LIB_INSTALL_DIR}")
    endif ()
endmacro()

macro(QTWEBKIT_SKIP_AUTOMOC _target)
    foreach (_src ${${_target}_SOURCES})
        set_property(SOURCE ${_src} PROPERTY SKIP_AUTOMOC ON)
    endforeach ()
endmacro()

macro(QTWEBKIT_GENERATE_MOC_FILES_CPP _target)
    if (${ARGC} LESS 2)
        message(FATAL_ERROR "QTWEBKIT_GENERATE_MOC_FILES_CPP must be called with at least 2 arguments")
    endif ()
    foreach (_file ${ARGN})
        get_filename_component(_ext ${_file} EXT)
        if (NOT _ext STREQUAL ".cpp")
            message(FATAL_ERROR "QTWEBKIT_GENERATE_MOC_FILES_CPP must be used for .cpp files only")
        endif ()
        get_filename_component(_name_we ${_file} NAME_WE)
        set(_moc_name "${CMAKE_CURRENT_BINARY_DIR}/${_name_we}.moc")
        qt5_generate_moc(${_file} ${_moc_name} TARGET ${_target})
        ADD_SOURCE_DEPENDENCIES(${_file} ${_moc_name})
    endforeach ()
endmacro()

macro(QTWEBKIT_GENERATE_MOC_FILE_H _target _header _source)
    get_filename_component(_header_ext ${_header} EXT)
    get_filename_component(_source_ext ${_source} EXT)
    if ((NOT _header_ext STREQUAL ".h") OR (NOT _source_ext STREQUAL ".cpp"))
        message(FATAL_ERROR "QTWEBKIT_GENERATE_MOC_FILE_H must be called with arguments being .h and .cpp files")
    endif ()
    get_filename_component(_name_we ${_header} NAME_WE)
    set(_moc_name "${CMAKE_CURRENT_BINARY_DIR}/moc_${_name_we}.cpp")
    qt5_generate_moc(${_header} ${_moc_name} TARGET ${_target})
    ADD_SOURCE_DEPENDENCIES(${_source} ${_moc_name})
endmacro()

macro(QTWEBKIT_GENERATE_MOC_FILES_H _target)
    if (${ARGC} LESS 2)
        message(FATAL_ERROR "QTWEBKIT_GENERATE_MOC_FILES_H must be called with at least 2 arguments")
    endif ()
    foreach (_header ${ARGN})
        get_filename_component(_header_dir ${_header} DIRECTORY)
        get_filename_component(_name_we ${_header} NAME_WE)
        set(_source "${_header_dir}/${_name_we}.cpp")
        QTWEBKIT_GENERATE_MOC_FILE_H(${_target} ${_header} ${_source})
    endforeach ()
endmacro()

set(CMAKE_MACOSX_RPATH ON)

add_definitions(-DBUILDING_QT__=1)
add_definitions(-DQT_NO_EXCEPTIONS)
add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_NO_CAST_TO_ASCII -DQT_ASCII_CAST_WARNINGS)
add_definitions(-DQT_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x050000)

# We use -fno-rtti with GCC and Clang, see OptionsCommon.cmake
if (COMPILER_IS_GCC_OR_CLANG)
    add_definitions(-DQT_NO_DYNAMIC_CAST)
endif ()

if (WIN32)
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        set(CMAKE_DEBUG_POSTFIX d)
    endif ()

    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX "")
endif ()

WEBKIT_OPTION_BEGIN()

if (APPLE)
    option(MACOS_FORCE_SYSTEM_XML_LIBRARIES "Use system installation of libxml2 and libxslt on macOS" ON)
    set(MACOS_BUILD_FRAMEWORKS ON) # TODO: Make it an option
endif ()

if (WIN32 OR APPLE)
    set(USE_LIBHYPHEN_DEFAULT OFF)
    set(USE_GSTREAMER_DEFAULT OFF)
    set(USE_QT_MULTIMEDIA_DEFAULT ON)
else ()
    set(USE_LIBHYPHEN_DEFAULT ON)
    set(USE_GSTREAMER_DEFAULT ON)
    set(USE_QT_MULTIMEDIA_DEFAULT OFF)
endif ()

if (MSVC)
    set(USE_QT_MULTIMEDIA_DEFAULT OFF)
    set(USE_MEDIA_FOUNDATION_DEFAULT ON)
else ()
    set(USE_MEDIA_FOUNDATION_DEFAULT OFF)
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

# FIXME: Move Qt handling here
set(REQUIRED_QT_VERSION 5.2.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui QUIET)

get_target_property(QT_CORE_TYPE Qt5::Core TYPE)
if (QT_CORE_TYPE MATCHES STATIC)
    set(QT_STATIC_BUILD ON)
    set(SHARED_CORE OFF)
    set(MACOS_BUILD_FRAMEWORKS OFF)
endif ()

if (QT_STATIC_BUILD)
    set(ENABLE_WEBKIT2_DEFAULT OFF)
else ()
    set(ENABLE_WEBKIT2_DEFAULT ON)
endif ()

if (UNIX AND TARGET Qt5::QXcbIntegrationPlugin AND NOT APPLE)
    set(ENABLE_X11_TARGET_DEFAULT ON)
else ()
    set(ENABLE_X11_TARGET_DEFAULT OFF)
endif ()

if (WIN32 OR ENABLE_X11_TARGET_DEFAULT)
    set(ENABLE_NETSCAPE_PLUGIN_API_DEFAULT ON)
else ()
    set(ENABLE_NETSCAPE_PLUGIN_API_DEFAULT OFF)
endif ()

# Public options specific to the Qt port. Do not add any options here unless
# there is a strong reason we should support changing the value of the option,
# and the option is not relevant to any other WebKit ports.
WEBKIT_OPTION_DEFINE(USE_GSTREAMER "Use GStreamer implementation of MediaPlayer" PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFINE(USE_LIBHYPHEN "Use automatic hyphenation with LibHyphen" PUBLIC ${USE_LIBHYPHEN_DEFAULT})
WEBKIT_OPTION_DEFINE(USE_MEDIA_FOUNDATION "Use MediaFoundation implementation of MediaPlayer" PUBLIC ${USE_MEDIA_FOUNDATION_DEFAULT})
WEBKIT_OPTION_DEFINE(USE_QT_MULTIMEDIA "Use Qt Multimedia implementation of MediaPlayer" PUBLIC ${USE_QT_MULTIMEDIA_DEFAULT})
WEBKIT_OPTION_DEFINE(USE_WOFF2 "Include support of WOFF2 fonts format" PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_INSPECTOR_UI "Include Inspector UI into resources" PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_OPENGL "Whether to use OpenGL." PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_PRINT_SUPPORT "Enable support for printing web pages" PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_QT_GESTURE_EVENTS "Enable support for gesture events (required for mouse in WK2)" PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_QT_WEBCHANNEL "Enable support for Qt WebChannel" PUBLIC ON)
WEBKIT_OPTION_DEFINE(ENABLE_WEBKIT2 "Enable WebKit2 (QML API)" PUBLIC ${ENABLE_WEBKIT2_DEFAULT})
WEBKIT_OPTION_DEFINE(ENABLE_X11_TARGET "Whether to enable support for the X11 windowing target." PUBLIC ${ENABLE_X11_TARGET_DEFAULT})

option(GENERATE_DOCUMENTATION "Generate HTML and QCH documentation" OFF)
cmake_dependent_option(ENABLE_TEST_SUPPORT "Build tools for running layout tests and related library code" ON
                                           "DEVELOPER_MODE" OFF)
option(USE_STATIC_RUNTIME "Use static runtime (MSVC only)" OFF)

# Private options specific to the Qt port. Changing these options is
# completely unsupported. They are intended for use only by WebKit developers.
WEBKIT_OPTION_DEFINE(ENABLE_TOUCH_ADJUSTMENT "Whether to use touch adjustment" PRIVATE ON)


# Public options shared with other WebKit ports. There must be strong reason
# to support changing the value of the option.
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ACCELERATED_2D_CANVAS PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ALLINONE_BUILD PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_API_TESTS PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_GRID_LAYOUT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DATABASE_PROCESS PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DATALIST_ELEMENT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DEVICE_ORIENTATION PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FULLSCREEN_API PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GAMEPAD_DEPRECATED PUBLIC ${ENABLE_GAMEPAD_DEPRECATED_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INDEXED_DATABASE PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LEGACY_WEB_AUDIO PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LINK_PREFETCH PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MEDIA_SOURCE PUBLIC ${USE_GSTREAMER_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NETSCAPE_PLUGIN_API PUBLIC ${ENABLE_NETSCAPE_PLUGIN_API_DEFAULT})
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
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_3D_TRANSFORMS PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_IMAGE_SET PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_REGIONS PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_SHAPES PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_SELECTORS_LEVEL4 PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DOM4_EVENTS_CONSTRUCTOR PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DOWNLOAD_ATTRIBUTE PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTL_JIT PRIVATE ${ENABLE_FTL_DEFAULT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTPDIR PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_COLOR PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_COLOR_POPOVER PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MEDIA_CONTROLS_SCRIPT PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MHTML PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NOTIFICATIONS PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SMOOTH_SCROLLING PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_USERSELECT_ALL PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_VIDEO_TRACK PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_TIMING PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEBGL PRIVATE ON)

if (MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JIT PRIVATE OFF)
endif ()

WEBKIT_OPTION_CONFLICT(USE_GSTREAMER USE_QT_MULTIMEDIA)
WEBKIT_OPTION_CONFLICT(USE_GSTREAMER USE_MEDIA_FOUNDATION)
WEBKIT_OPTION_CONFLICT(USE_QT_MULTIMEDIA USE_MEDIA_FOUNDATION)

WEBKIT_OPTION_DEPEND(ENABLE_3D_TRANSFORMS ENABLE_OPENGL)
WEBKIT_OPTION_DEPEND(ENABLE_ACCELERATED_2D_CANVAS ENABLE_OPENGL)
WEBKIT_OPTION_DEPEND(ENABLE_WEBGL ENABLE_OPENGL)

# WebAudio and MediaSource are supported with GStreamer only
WEBKIT_OPTION_DEPEND(ENABLE_WEB_AUDIO USE_GSTREAMER)
WEBKIT_OPTION_DEPEND(ENABLE_LEGACY_WEB_AUDIO USE_GSTREAMER)
WEBKIT_OPTION_DEPEND(ENABLE_MEDIA_SOURCE USE_GSTREAMER)

WEBKIT_OPTION_DEPEND(ENABLE_QT_WEBCHANNEL ENABLE_WEBKIT2)

# While it's possible to have UI-less NPAPI plugins without X11, we don't support this case yet
if (UNIX AND NOT APPLE)
    WEBKIT_OPTION_DEPEND(ENABLE_NETSCAPE_PLUGIN_API ENABLE_X11_TARGET)
endif ()

WEBKIT_OPTION_END()

# FTL JIT and IndexedDB support require GCC 4.9
# TODO: Patch code to avoid variadic lambdas
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (ENABLE_FTL_JIT OR ENABLE_INDEXED_DATABASE OR (ENABLE_WEBKIT2 AND ENABLE_DATABASE_PROCESS))
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")
            message(FATAL_ERROR "GCC 4.9.0 is required to build QtWebKit with FTL JIT, Indexed Database, and Database Process (WebKit 2). Use a newer GCC version or clang, or disable these features")
        endif ()
    else ()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.0")
            message(FATAL_ERROR "GCC 4.8.0 is required to build QtWebKit, use a newer GCC version or clang")
        endif ()
    endif ()
elseif (MSVC AND MSVC_VERSION LESS 1900)
    message(FATAL_ERROR "MSVC 2015 is required to build QtWebKit, use a newer MSVC version")
endif ()

if (APPLE AND CMAKE_SYSTEM_VERSION VERSION_LESS 14.0.0)
    message(FATAL_ERROR "macOS 10.10 or higher is required to build and run QtWebKit")
endif ()

set(ENABLE_WEBKIT ON)
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
endif ()

if (MSVC)
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

if (DEFINED ENV{SQLITE3SRCDIR})
    get_filename_component(SQLITE3SRC_ABS_DIR $ENV{SQLITE3SRCDIR} ABSOLUTE)
    set(SQLITE3_SOURCE_DIR ${SQLITE3SRC_ABS_DIR} CACHE PATH "Path to SQLite sources to use instead of system library" FORCE)
endif ()

if (SQLITE3_SOURCE_DIR)
    set(SQLITE_INCLUDE_DIR ${SQLITE3_SOURCE_DIR})
    set(SQLITE_SOURCE_FILE ${SQLITE3_SOURCE_DIR}/sqlite3.c)
    if (NOT EXISTS ${SQLITE_SOURCE_FILE})
        message(FATAL_ERROR "${SQLITE_SOURCE_FILE} not found.")
    endif ()
    add_library(qtsqlite STATIC ${SQLITE_SOURCE_FILE})
    target_compile_definitions(qtsqlite PUBLIC -DSQLITE_CORE -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_OMIT_COMPLETE)
    WEBKIT_SET_EXTRA_COMPILER_FLAGS(qtsqlite)
    set(SQLITE_LIBRARIES qtsqlite)
    set(SQLITE_FOUND 1)
else ()
    find_package(Sqlite REQUIRED)
endif ()

find_package(Threads REQUIRED)

if (NOT QT_BUNDLED_JPEG)
    find_package(JPEG REQUIRED)
else ()
    set(JPEG_FOUND 1)
endif ()

if (NOT QT_BUNDLED_PNG)
    find_package(PNG REQUIRED)
else ()
    set(PNG_FOUND 1)
endif ()

if (NOT QT_BUNDLED_ZLIB)
    find_package(ZLIB REQUIRED)
else ()
    set(ZLIB_FOUND 1)
endif ()

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
    if (QT_STATIC_BUILD)
        find_library(CARBON_LIBRARY Carbon)
        find_library(COCOA_LIBRARY Cocoa)
        find_library(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration)
        find_library(CORESERVICES_LIBRARY CoreServices)
        find_library(SECURITY_LIBRARY Security)
    endif ()
endif ()

if (MACOS_FORCE_SYSTEM_XML_LIBRARIES)
    set(LIBXML2_INCLUDE_DIR "${CMAKE_OSX_SYSROOT}/usr/include/libxml2")
    set(LIBXML2_LIBRARIES xml2)
    if (ENABLE_XSLT)
        set(LIBXSLT_INCLUDE_DIR "${CMAKE_OSX_SYSROOT}/usr/include/libxslt")
        set(LIBXSLT_LIBRARIES xslt)
    endif ()
else ()
    find_package(LibXml2 2.8.0 REQUIRED)
    if (ENABLE_XSLT)
        find_package(LibXslt 1.1.7 REQUIRED)
    endif ()
endif ()

find_package(Fontconfig)

if (FONTCONFIG_FOUND)
    SET_AND_EXPOSE_TO_BUILD(HAVE_FONTCONFIG 1)
endif ()

find_package(WebP)

if (WEBP_FOUND)
    SET_AND_EXPOSE_TO_BUILD(USE_WEBP 1)
endif ()

set(QT_REQUIRED_COMPONENTS Core Gui Network)

# FIXME: Allow building w/o these components
list(APPEND QT_REQUIRED_COMPONENTS
    Widgets
)
set(QT_OPTIONAL_COMPONENTS OpenGL)

if (ENABLE_API_TESTS OR ENABLE_TEST_SUPPORT)
    list(APPEND QT_REQUIRED_COMPONENTS
        Test
    )
    if (ENABLE_WEBKIT2)
        list(APPEND QT_REQUIRED_COMPONENTS
            QuickTest
        )
    endif ()
endif ()

if (ENABLE_GEOLOCATION)
    list(APPEND QT_REQUIRED_COMPONENTS Positioning)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QTPOSITIONING 1)
endif ()

if (ENABLE_DEVICE_ORIENTATION)
    list(APPEND QT_REQUIRED_COMPONENTS Sensors)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QTSENSORS 1)
endif ()

if (ENABLE_PRINT_SUPPORT)
    list(APPEND QT_REQUIRED_COMPONENTS PrintSupport)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QTPRINTSUPPORT 1)
endif ()

if (ENABLE_WEBKIT2)
    list(APPEND QT_REQUIRED_COMPONENTS
        Quick
    )
    SET_AND_EXPOSE_TO_BUILD(USE_COORDINATED_GRAPHICS TRUE)
    SET_AND_EXPOSE_TO_BUILD(USE_COORDINATED_GRAPHICS_MULTIPROCESS TRUE)

    if (ENABLE_INDEXED_DATABASE AND NOT ENABLE_DATABASE_PROCESS)
        message(FATAL_ERROR "Using IndexedDB with WebKit2 requires ENABLE_DATABASE_PROCESS")
    endif ()
endif ()

# Mach ports and Unix sockets are currently used by WK2, but their USE() values
# affect building WorkQueue
if (APPLE)
    SET_AND_EXPOSE_TO_BUILD(USE_MACH_PORTS 1) # Qt-specific
elseif (UNIX)
    SET_AND_EXPOSE_TO_BUILD(USE_UNIX_DOMAIN_SOCKETS 1)
endif ()

if (ENABLE_QT_WEBCHANNEL)
    list(APPEND QT_REQUIRED_COMPONENTS
        WebChannel
    )
endif ()

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
if (QT_STATIC_BUILD)
    foreach (qt_module ${QT_REQUIRED_COMPONENTS})
        CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
    endforeach ()
    # HACK: We must explicitly add LIB path of the Qt installation
    # to correctly find qtpcre
    link_directories(${_qt5_install_prefix}/../)
endif ()

foreach (qt_module ${QT_OPTIONAL_COMPONENTS})
    find_package("Qt5${qt_module}" ${REQUIRED_QT_VERSION} PATHS ${_qt5_install_prefix} NO_DEFAULT_PATH)
    if (QT_STATIC_BUILD)
        CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
    endif ()
endforeach ()

if (QT_STATIC_BUILD)
    if (NOT EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
        message(FATAL_ERROR "Unable to find ${STATIC_DEPENDENCIES_CMAKE_FILE}")
    endif ()
    include(${STATIC_DEPENDENCIES_CMAKE_FILE})
    list(REMOVE_DUPLICATES STATIC_LIB_DEPENDENCIES)
endif ()

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

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# TODO: figure out if we can run automoc only on Qt sources

# From OptionsEfl.cmake
# Optimize binary size for release builds by removing dead sections on unix/gcc.
if (COMPILER_IS_GCC_OR_CLANG AND UNIX)
    if (NOT APPLE)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffunction-sections -fdata-sections")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")
    endif ()

    if (NOT SHARED_CORE)
        set(CMAKE_C_FLAGS "-fvisibility=hidden ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS}")
    endif ()
endif ()

if (WIN32 AND COMPILER_IS_GCC_OR_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-keep-inline-dllexport")
endif ()

if (APPLE)
    SET_AND_EXPOSE_TO_BUILD(HAVE_QOS_CLASSES 1)
endif ()

if (ENABLE_MATHML)
    SET_AND_EXPOSE_TO_BUILD(ENABLE_OPENTYPE_MATH 1)
endif ()

SET_AND_EXPOSE_TO_BUILD(WTF_PLATFORM_X11 ${ENABLE_X11_TARGET})

if (ENABLE_NETSCAPE_PLUGIN_API)
    # MOZ_X11 and XP_UNIX are required by npapi.h. Their value is not checked;
    # only their definedness is. They should only be defined in the true case.
    if (${ENABLE_X11_TARGET})
        SET_AND_EXPOSE_TO_BUILD(MOZ_X11 1)
        set(PLUGIN_BACKEND_XLIB 1)
    endif ()
    if (${WTF_OS_UNIX})
        SET_AND_EXPOSE_TO_BUILD(XP_UNIX 1)
        SET_AND_EXPOSE_TO_BUILD(ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE 1)
        SET_AND_EXPOSE_TO_BUILD(ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1)
    endif ()

    if (ENABLE_WEBKIT2)
        if (ENABLE_X11_TARGET)
            set(ENABLE_PLUGIN_PROCESS 1)
            SET_AND_EXPOSE_TO_BUILD(PLUGIN_ARCHITECTURE_X11 1)
            SET_AND_EXPOSE_TO_BUILD(PLUGIN_ARCHITECTURE_UNSUPPORTED 0)
        else ()
            SET_AND_EXPOSE_TO_BUILD(PLUGIN_ARCHITECTURE_X11 0)
            SET_AND_EXPOSE_TO_BUILD(PLUGIN_ARCHITECTURE_UNSUPPORTED 1)
        endif ()
    endif ()
endif ()

if (ENABLE_X11_TARGET)
    find_package(X11 REQUIRED)
    if (NOT X11_Xcomposite_FOUND)
        message(FATAL_ERROR "libXcomposite is required for ENABLE_X11_TARGET")
    elseif (NOT X11_Xrender_FOUND)
        message(FATAL_ERROR "libXrender is required for ENABLE_X11_TARGET")
    endif ()
endif ()

if (ENABLE_OPENGL)
    SET_AND_EXPOSE_TO_BUILD(USE_TEXTURE_MAPPER_GL TRUE)
    SET_AND_EXPOSE_TO_BUILD(ENABLE_GRAPHICS_CONTEXT_3D TRUE)

    # TODO: Add proper support of DynamicGL detection to Qt and use it
    if (WIN32 AND NOT QT_USES_GLES2_ONLY)
        set(Qt5Gui_OPENGL_IMPLEMENTATION GL)
    endif ()
endif ()

if (NOT ENABLE_VIDEO)
    set(USE_MEDIA_FOUNDATION OFF)
    set(USE_QT_MULTIMEDIA OFF)

    if (NOT ENABLE_WEB_AUDIO)
        set(USE_GSTREAMER OFF) # TODO: What about MEDIA_STREAM?
    endif ()
endif ()

if (USE_QT_MULTIMEDIA)
    find_package(Qt5Multimedia ${REQUIRED_QT_VERSION} REQUIRED PATHS ${_qt5_install_prefix} NO_DEFAULT_PATH)
    # FIXME: Allow building w/o widgets
    find_package(Qt5MultimediaWidgets ${REQUIRED_QT_VERSION} REQUIRED PATHS ${_qt5_install_prefix} NO_DEFAULT_PATH)
endif ()

# From OptionsGTK.cmake
# FIXME: Refactor to avoid duplication
if (USE_GSTREAMER)
    SET_AND_EXPOSE_TO_BUILD(USE_GLIB 1)
    find_package(GLIB 2.36 REQUIRED COMPONENTS gio gobject)

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
if (WIN32)
    add_definitions(-DNOMINMAX -DUNICODE -D_UNICODE -D_WINDOWS)
endif ()

if (MSVC)
    add_definitions(-DWINVER=0x601)

    add_definitions(
        /wd4018 /wd4068 /wd4099 /wd4100 /wd4127 /wd4138 /wd4146 /wd4180 /wd4189
        /wd4201 /wd4244 /wd4251 /wd4267 /wd4275 /wd4288 /wd4291 /wd4305 /wd4309
        /wd4344 /wd4355 /wd4389 /wd4396 /wd4456 /wd4457 /wd4458 /wd4459 /wd4481
        /wd4503 /wd4505 /wd4510 /wd4512 /wd4530 /wd4577 /wd4610 /wd4611 /wd4702
        /wd4706 /wd4800 /wd4819 /wd4951 /wd4952 /wd4996 /wd6011 /wd6031 /wd6211
        /wd6246 /wd6255 /wd6387
    )

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        # Create pdb files for debugging purposes, also for Release builds
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    endif ()

    add_compile_options(/GS)

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

    # Make sure incremental linking is turned off, as it creates unacceptably long link times.
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS "${replace_CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL:NO")
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
    set(CMAKE_EXE_LINKER_FLAGS "${replace_CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO")

    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_SHARED_LINKER_FLAGS_DEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${replace_CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${replace_CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")

    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO})
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${replace_CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL:NO")
    string(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replace_CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${replace_CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL:NO")

    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:NOREF /OPT:NOICF")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:NOREF /OPT:NOICF")

        # To debug linking time issues, uncomment the following three lines:
        #add_compile_options(/Bv)
        #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /VERBOSE /VERBOSE:INCR /TIME")
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /VERBOSE /VERBOSE:INCR /TIME")

        # enable fast link
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG:FASTLINK")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG:FASTLINK")
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
        string(REGEX REPLACE "/EHsc- " "/EHs- /EHc- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable C++ exceptions
        string(REGEX REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Warnings are important
    endif ()

    if (USE_STATIC_RUNTIME)
        foreach (flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            # Use the multithreaded static runtime library instead of the default DLL runtime.
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endforeach ()
    endif ()

    if (NOT QT_CONAN_DIR)
        set(ICU_LIBRARIES icuuc${CMAKE_DEBUG_POSTFIX} icuin${CMAKE_DEBUG_POSTFIX} icudt${CMAKE_DEBUG_POSTFIX})
    endif ()
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


include(ECMQueryQmake)

query_qmake(qt_install_prefix_dir QT_INSTALL_PREFIX)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${qt_install_prefix_dir}" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
endif ()

include(KDEInstallDirs)

if (NOT qt_install_prefix_dir STREQUAL "${CMAKE_INSTALL_PREFIX}")
    set(KDE_INSTALL_USE_QT_SYS_PATHS OFF)
endif ()

# We split all installed files into 2 components: Code and Data. This is different from
# traditional approach with Runtime and Devel, but we need it to fix concurrent installation of
# debug and release builds in qmake-based build
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "Code")
