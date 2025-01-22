cmake_minimum_required(VERSION 3.15)

project(RDMnetBroker VERSION 0.1.0)

###################### Compile Options and Configuration ######################

if(WIN32)
  add_compile_definitions(NOMINMAX)
endif()

option(RDMNETBROKER_BUILD_TESTS "Build the RDMnet Broker unit tests" OFF)
option(RDMNETBROKER_INCLUDE_SIGN_TOOLS "Include scripts for signing built artifacts (used in development only)" OFF)

set(RDMNETBROKER_CMAKE ${CMAKE_CURRENT_LIST_DIR}/tools/cmake)
set(RDMNETBROKER_ROOT ${CMAKE_CURRENT_LIST_DIR})

################################# Dependencies ################################

include(${RDMNETBROKER_CMAKE}/OssDependencyTools.cmake)
determine_compile_environment()
if(NOT COMPILING_AS_OSS)
  include(${RDMNETBROKER_CMAKE}/AddCMakeTools.cmake)
endif()
include(${RDMNETBROKER_CMAKE}/ResolveDependencies.cmake)

if(RDMNETBROKER_INCLUDE_SIGN_TOOLS)
  add_project_dependency(ETC_Sign)
  add_project_dependency(ETC_Mac_Notarize)
endif()

############################## Standalone support #############################

if(NOT COMPILING_AS_OSS)
  include(${CMAKE_TOOLS_MODULES}/QualityGateHelpers.cmake)

  setup_standalone_compile()
  setup_clang_format()
  # TODO: setup_clang_tidy()
  setup_address_sanitizer()
endif()

################################### Main app ##################################

add_subdirectory(src)

#################################### Tests ####################################

if(RDMNETBROKER_BUILD_TESTS)
  include(GoogleTest)
  enable_testing()
  add_subdirectory(tests)
endif()
