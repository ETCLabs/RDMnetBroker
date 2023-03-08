
add_library(RDMnetBrokerServiceCore
  broker_common.h
  broker_common.cpp
  broker_config.h
  broker_config.cpp
  broker_shell.h
  broker_shell.cpp
  broker_os_interface.h
  broker_version.h
)
set_target_properties(RDMnetBrokerServiceCore PROPERTIES CXX_STANDARD 17)
if(WIN32)
  set_target_properties(RDMnetBrokerServiceCore PROPERTIES COMPILE_PDB_NAME "RDMnetBrokerServiceCore")
  set_target_properties(RDMnetBrokerServiceCore PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
target_include_directories(RDMnetBrokerServiceCore PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(RDMnetBrokerServiceCore PUBLIC RDMnetBroker nlohmann_json)

if(WIN32)
  get_target_property(BROKER_SERVICE_CORE_PDB_OUTPUT_DIRECTORY RDMnetBrokerServiceCore COMPILE_PDB_OUTPUT_DIRECTORY)
  get_target_property(BROKER_SERVICE_CORE_PDB_NAME RDMnetBrokerServiceCore COMPILE_PDB_NAME)

  install(FILES "${BROKER_SERVICE_CORE_PDB_OUTPUT_DIRECTORY}/Debug/${BROKER_SERVICE_CORE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS Debug OPTIONAL)
  install(FILES "${BROKER_SERVICE_CORE_PDB_OUTPUT_DIRECTORY}/Release/${BROKER_SERVICE_CORE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS Release OPTIONAL)
  install(FILES "${BROKER_SERVICE_CORE_PDB_OUTPUT_DIRECTORY}/MinSizeRel/${BROKER_SERVICE_CORE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS MinSizeRel OPTIONAL)
  install(FILES "${BROKER_SERVICE_CORE_PDB_OUTPUT_DIRECTORY}/RelWithDebInfo/${BROKER_SERVICE_CORE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS RelWithDebInfo OPTIONAL)
endif()
