
add_executable(RDMnetBrokerService
  broker_service.h
  broker_service.cpp
  service_utils.h
  service_utils.cpp
  win_broker_log.h
  win_broker_log.cpp
  main.cpp
)
target_link_libraries(RDMnetBrokerService PRIVATE RDMnetBrokerServiceCore)
target_compile_definitions(RDMnetBrokerService PRIVATE UNICODE _UNICODE)
set_target_properties(RDMnetBrokerService PROPERTIES CXX_STANDARD 17)
