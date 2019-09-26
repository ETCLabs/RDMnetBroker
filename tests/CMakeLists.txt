
message(STATUS "Adding GoogleTest dependency...")
include(AddGoogleTest)

include(GoogleTest)

add_executable(test_rdmnet_broker
  test_broker_config.cpp
)
target_link_libraries(test_rdmnet_broker PRIVATE RDMnetBrokerCore gtest_main)
set_target_properties(test_rdmnet_broker PROPERTIES
  CXX_STANDARD 17
  FOLDER tests
)
gtest_discover_tests(test_rdmnet_broker)
