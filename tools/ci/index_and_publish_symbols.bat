python C:\tools\ci\Symbols\publish_symbols.py^
  -n "RDMnetBrokerService"^
  -v %NEW_BUILD_VERSION%^
  -a %COMPILER_ARCH%^
  -u https://artifactory.etcconnect.com:443^
  -r NET^
  -k %RDMNETBRKR_ARTIFACTORY_API_KEY%^
  --os windows^
  -d .\symbols^
  .\build\%CMAKE_INSTALL%\bin\RDMnetBrokerService.exe
