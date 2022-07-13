
add_executable(TestBrokerServiceCore
  test_broker_config.cpp
  test_broker_shell.cpp
)
target_link_libraries(TestBrokerServiceCore PRIVATE RDMnetBrokerServiceCore gmock_main)
set_target_properties(TestBrokerServiceCore PROPERTIES
  CXX_STANDARD 17
  FOLDER tests
)
gtest_discover_tests(TestBrokerServiceCore)
