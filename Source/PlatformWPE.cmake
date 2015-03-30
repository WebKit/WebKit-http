# FIXME: This should be moved to Source/ThirdParty/. And finally into a standalone project.
add_subdirectory(WPE)
WEBKIT_SET_EXTRA_COMPILER_FLAGS(WPE ${ADDITIONAL_COMPILER_FLAGS})
