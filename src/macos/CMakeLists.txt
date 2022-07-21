add_executable(RDMnetBrokerService
  broker_service.h
  broker_service.cpp
  mac_broker_os_interface.h
  mac_broker_os_interface.cpp

  main.cpp
)
set_target_properties(RDMnetBrokerService PROPERTIES CXX_STANDARD 17)
target_link_libraries(RDMnetBrokerService PRIVATE RDMnetBrokerServiceCore)

install(TARGETS RDMnetBrokerService
  RUNTIME DESTINATION libexec  # Apple docs recommend /usr/local/libexec
)

