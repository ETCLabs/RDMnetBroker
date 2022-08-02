
set(TEST_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_executable(TestBrokerServiceCore
  test_broker_config.cpp
  test_broker_shell.cpp
)
set_target_properties(TestBrokerServiceCore PROPERTIES
  CXX_STANDARD 17
  FOLDER tests
)
target_link_libraries(TestBrokerServiceCore PRIVATE RDMnetBrokerServiceCore gmock_main)
gtest_discover_tests(TestBrokerServiceCore NO_PRETTY_VALUES EXTRA_ARGS "--gtest_output=xml:${TEST_BIN_DIR}/test-results/")
