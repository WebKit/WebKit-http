list(APPEND WTF_SOURCES
    qt/MainThreadQt.cpp
    qt/RunLoopQt.cpp
    qt/WorkQueueQt.cpp
)

list(APPEND WTF_INCLUDE_DIRECTORIES
    ${Qt5Core_INCLUDES}
)

list(APPEND WTF_LIBRARIES
    ${Qt5Core_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
