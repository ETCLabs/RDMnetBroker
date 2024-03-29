# The Broker Service is installed in one of two ways: from the Windows installer built in this
# repository, or by invoking the service executable directly with the argument "-install".
#
# The installer cannot install the service by invoking the executable, because the service
# installation must be part of the MSI table so that it can be reference-counted by dependent
# products and cannot be removed when products still exist on the system that need it.
#
# The service configuration options, which are set here in both service_config.wxi and in
# service_config.h, are defined below.

set(BROKER_SERVICE_NAME "ETC RDMnet Broker")
set(BROKER_SERVICE_DISPLAY_NAME "ETC RDMnet Broker")
set(BROKER_SERVICE_DESCRIPTION "Implements an RDMnet message broker as defined in ANSI E1.33")

# The following pairs must match; see https://wixtoolset.org/documentation/manual/v3/xsd/wix/serviceinstall.html
# and https://docs.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-createservicew
set(BROKER_SERVICE_C_START_TYPE SERVICE_AUTO_START)
set(BROKER_SERVICE_WIX_START_TYPE "auto")

set(BROKER_SERVICE_C_ERROR_CONTROL SERVICE_ERROR_NORMAL)
set(BROKER_SERVICE_WIX_ERROR_CONTROL "normal")

set(BROKER_SERVICE_C_FAILURE_ACTION_1 SC_ACTION_RESTART)
set(BROKER_SERVICE_WIX_FAILURE_ACTION_1 "restart")

set(BROKER_SERVICE_C_FAILURE_ACTION_1_DELAY 5000)
set(BROKER_SERVICE_WIX_FAILURE_ACTION_1_DELAY 5)

configure_file(${CMAKE_CURRENT_LIST_DIR}/service_config.h.in
  ${CMAKE_CURRENT_LIST_DIR}/GeneratedFiles/service_config.h
)
configure_file(${CMAKE_CURRENT_LIST_DIR}/service_config.wxi.in
  ${PROJECT_SOURCE_DIR}/tools/install/windows/merge/GeneratedFiles/service_config.wxi
)
configure_file(${PROJECT_SOURCE_DIR}/tools/cmake/mdnsmerge.wxi.in
  ${PROJECT_SOURCE_DIR}/tools/install/windows/standalone/GeneratedFiles/mdnsmerge.wxi
)

# The main service executable

add_executable(RDMnetBrokerService
  broker_service.h
  broker_service.cpp
  service_config.h
  service_utils.h
  service_utils.cpp
  win_broker_os_interface.h
  win_broker_os_interface.cpp

  Resource.aps
  resource.h
  Resource.rc

  main.cpp
)
set_target_properties(RDMnetBrokerService PROPERTIES CXX_STANDARD 17)
set_target_properties(RDMnetBrokerService PROPERTIES COMPILE_PDB_NAME "RDMnetBrokerService")
set_target_properties(RDMnetBrokerService PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(RDMnetBrokerService PRIVATE GeneratedFiles)
target_link_libraries(RDMnetBrokerService PRIVATE RDMnetBrokerServiceCore)
target_compile_definitions(RDMnetBrokerService PRIVATE UNICODE _UNICODE)

install(TARGETS RDMnetBrokerService
  RUNTIME DESTINATION bin
)

get_target_property(BROKER_SERVICE_PDB_OUTPUT_DIRECTORY RDMnetBrokerService COMPILE_PDB_OUTPUT_DIRECTORY)
get_target_property(BROKER_SERVICE_PDB_NAME RDMnetBrokerService COMPILE_PDB_NAME)

install(FILES "${BROKER_SERVICE_PDB_OUTPUT_DIRECTORY}/Debug/${BROKER_SERVICE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS Debug OPTIONAL)
install(FILES "${BROKER_SERVICE_PDB_OUTPUT_DIRECTORY}/Release/${BROKER_SERVICE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS Release OPTIONAL)
install(FILES "${BROKER_SERVICE_PDB_OUTPUT_DIRECTORY}/MinSizeRel/${BROKER_SERVICE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS MinSizeRel OPTIONAL)
install(FILES "${BROKER_SERVICE_PDB_OUTPUT_DIRECTORY}/RelWithDebInfo/${BROKER_SERVICE_PDB_NAME}.pdb" DESTINATION symbols CONFIGURATIONS RelWithDebInfo OPTIONAL)
