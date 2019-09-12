cmake_minimum_required(VERSION 3.10)

project(RDMnetBroker VERSION 0.1.0)

if (MSVC)
  add_compile_options(/W4)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-Wall -Wconversion)
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    add_compile_options(-Wno-sign-conversion)
  else()
    add_compile_options(-Wno-sign-compare)
  endif()
endif()

# Allow folder grouping in generated IDE projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/tools/cmake)
set(BROKER_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Add our RDMnet source dependency
include(BrokerAddDependency)
broker_add_dependency(RDMnet RDMNET_DIR)
set_target_properties(Broker PROPERTIES FOLDER dependencies)

# Add the JSON 3rd-party library
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_LIST_DIR}/external/json)
find_package(nlohmann_json 3.7.0 REQUIRED)

################################### Main app ##################################

add_subdirectory(src)

#################################### Tests ####################################

include(CTest)
enable_testing()
add_subdirectory(tests)
