if(COMPILING_AS_OSS)
  get_cpm()

  add_oss_dependency(RDMnet GIT_REPOSITORY https://github.com/ETCLabs/RDMnet.git)
  add_oss_dependency(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git)

  if(RDMNETBROKER_BUILD_TESTS)
    add_oss_dependency(googletest GIT_REPOSITORY https://github.com/google/googletest.git)
  endif()
else()
  include(${CMAKE_TOOLS_MODULES}/DependencyManagement.cmake)
  add_project_dependencies()

  if(RDMNETBROKER_BUILD_TESTS)
    add_project_dependency(googletest)
  endif()
endif()
