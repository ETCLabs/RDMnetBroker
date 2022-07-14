
add_library(RDMnetBrokerServiceCore
  broker_config.h
  broker_config.cpp
  broker_shell.h
  broker_shell.cpp
  broker_os_interface.h
  broker_version.h
)
set_target_properties(RDMnetBrokerServiceCore PROPERTIES CXX_STANDARD 17)
target_include_directories(RDMnetBrokerServiceCore PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(RDMnetBrokerServiceCore PUBLIC RDMnetBroker nlohmann_json)
