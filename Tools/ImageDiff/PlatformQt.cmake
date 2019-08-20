set(ImageDiff_SOURCES
    ${IMAGE_DIFF_DIR}/qt/ImageDiff.cpp
)

list(APPEND ImageDiff_SYSTEM_INCLUDE_DIRECTORIES
    ${Qt5Gui_INCLUDE_DIRS}
)

set(ImageDiff_LIBRARIES
    ${Qt5Gui_LIBRARIES}
)
