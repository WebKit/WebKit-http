set(QtTestBrowser_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_BINARY_DIR}/include"
    "${CMAKE_BINARY_DIR}/include/QtWebKit"
    "${CMAKE_BINARY_DIR}/include/QtWebKitWidgets"
    "${WTF_FRAMEWORK_HEADERS_DIR}"
)

set(QtTestBrowser_SOURCES
    cookiejar.cpp
    fpstimer.cpp
    launcherwindow.cpp
    locationedit.cpp
    mainwindow.cpp
    qttestbrowser.cpp
    urlloader.cpp
    utils.cpp
    webpage.cpp
    webview.cpp
)

set(QtTestBrowser_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

set(QtTestBrowser_LIBRARIES
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5PrintSupport_LIBRARIES}
    WebKitWidgets
    ${STATIC_LIB_DEPENDENCIES}
)

qt_wrap_cpp(WebKit QtTestBrowser_SOURCES
    webinspector.h
)

if (Qt5OpenGL_FOUND)
    list(APPEND QtTestBrowser_SYSTEM_INCLUDE_DIRECTORIES
        ${Qt5OpenGL_INCLUDE_DIRS}
    )
    list(APPEND QtTestBrowser_LIBRARIES
        ${Qt5OpenGL_LIBRARIES}
    )
endif ()

if (ENABLE_TEST_SUPPORT)
    add_definitions(-DHAVE_QTTESTSUPPORT)
endif ()

include_directories(${QtTestBrowser_INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${QtTestBrowser_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(QtTestBrowser ${QtTestBrowser_SOURCES})
target_link_libraries(QtTestBrowser ${QtTestBrowser_LIBRARIES})
set_target_properties(QtTestBrowser PROPERTIES FOLDER "Tools" AUTOMOC ON)

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    set_target_properties(QtTestBrowser PROPERTIES WIN32_EXECUTABLE ON)
endif ()
