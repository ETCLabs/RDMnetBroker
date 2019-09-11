cmake_minimum_required(VERSION 3.3)

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

# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/tools/cmake)

add_subdirectory(external/RDMnet)

################################### Main app ##################################

add_subdirectory(src)

#################################### Tests ####################################
