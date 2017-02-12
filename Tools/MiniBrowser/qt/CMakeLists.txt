set(MiniBrowser_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/Source"
    "${FORWARDING_HEADERS_DIR}/QtWebKit"
    "${WEBKIT_DIR}/qt/Api"
    "${WEBKIT_DIR}/qt/WebCoreSupport"
    "${WEBKIT2_DIR}/UIProcess/API/qt"
    ${WTF_DIR}
)

set(MiniBrowser_SOURCES
    BrowserWindow.cpp
    MiniBrowserApplication.cpp
    UrlLoader.cpp
    main.cpp
    utils.cpp
)

qt5_add_resources(MiniBrowser_SOURCES
    MiniBrowser.qrc
)

set(MiniBrowser_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
)

set(MiniBrowser_LIBRARIES
    WebKit
    WebKit2
    ${Qt5Quick_LIBRARIES}
)

if (ENABLE_TEST_SUPPORT)
    add_definitions(-DHAVE_QTTESTSUPPORT)
endif ()

include_directories(${MiniBrowser_INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${MiniBrowser_SYSTEM_INCLUDE_DIRECTORIES})
add_executable(MiniBrowser ${MiniBrowser_SOURCES})
target_link_libraries(MiniBrowser ${MiniBrowser_LIBRARIES})
set_target_properties(MiniBrowser PROPERTIES FOLDER "Tools")
