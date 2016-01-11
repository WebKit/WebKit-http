add_definitions(-DSTATICALLY_LINKED_WITH_WTF)

list(APPEND JavaScriptCore_INCLUDE_DIRECTORIES
    ${Qt5Core_INCLUDES}
    ${WTF_DIR}
)

list(APPEND JavaScriptCore_LIBRARIES
    ${Qt5Core_LIBRARIES}
)
