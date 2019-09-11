
add_library(RDMnetBrokerCore
  broker_config.h
  broker_config.cpp
)
target_include_directories(RDMnetBrokerCore PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(RDMnetBrokerCore PUBLIC Broker nlohmann_json::nlohmann_json)
