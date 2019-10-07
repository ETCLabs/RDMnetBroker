
add_executable(RDMnetBrokerService
  broker_service.h
  broker_service.cpp
  service_utils.h
  service_utils.cpp
  main.cpp
)
target_link_libraries(RDMnetBrokerService PRIVATE RDMnetBrokerServiceCore)
target_compile_definitions(RDMnetBrokerService PRIVATE UNICODE _UNICODE)
