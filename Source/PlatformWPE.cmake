if(WPE_BACKEND)
  set(WPE_BACKEND_DIR "ThirdParty/WPE-${WPE_BACKEND}")
else ()
  set(WPE_BACKEND_DIR "ThirdParty/WPE-mesa")
endif ()

# FIXME: This should be moved into a standalone project.
add_subdirectory(ThirdParty/WPE)
add_subdirectory(${WPE_BACKEND_DIR})
