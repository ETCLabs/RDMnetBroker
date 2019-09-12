
add_subdirectory(core)

if(WIN32)
  # add_subdirectory(windows)
elseif(APPLE)
  # add_subdirectory(macos)
elseif(UNIX)
  # add_subdirectory(linux)
else()
  message(FATAL_ERROR "Cannot build the RDMnetBroker project on this system.")
endif()
