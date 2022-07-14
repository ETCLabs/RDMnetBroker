
add_executable(TestBrokerServiceCore
  test_broker_config.cpp
  test_broker_shell.cpp
)
set_target_properties(TestBrokerServiceCore PROPERTIES
  CXX_STANDARD 17
  FOLDER tests
)
target_link_libraries(TestBrokerServiceCore PRIVATE RDMnetBrokerServiceCore gmock_main)
gtest_discover_tests(TestBrokerServiceCore)
