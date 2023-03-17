@rem Crazy gymnastics to get the current git hash into an environment variable:
echo|set /p="set PROJECT_HASH=">sethash.bat
call git log -1 --pretty=%%%%H >>sethash.bat
call sethash.bat
del sethash.bat

@rem execute github source indexing powershell script
@rem Need to assemble a URL like this:
@rem https://raw.githubusercontent.com/ETCLabs/RDMnetBroker/60a3a468842b119603717852b7b76bbc28ecc8e7/.clang-format

powershell C:\tools\ci\github-sourceindexer.ps1 ^
  -dbgToolsPath "%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\srcsrv" ^
  -gitHubUrl https://raw.githubusercontent.com ^
  -serverIsRaw ^
  -userId "ETCLabs" ^
  -repository "RDMnetBroker" ^
  -branch %PROJECT_HASH% ^
  -symbolsFolder "%cd%\symbols" ^
  -sourcesRoot "%cd%" ^
  -ignoreUnknown -verifyLocalRepo

python C:\tools\ci\Symbols\publish_symbols.py ^
  -n "RDMnetBrokerService" ^
  -v %NEW_BUILD_VERSION% ^
  -a %COMPILER_ARCH% ^
  -u https://artifactory.etcconnect.com:443 ^
  -r NET ^
  -k %RDMNETBRKR_ARTIFACTORY_API_KEY% ^
  --os windows ^
  -d .\symbols ^
  .\build\%CMAKE_INSTALL%\bin\RDMnetBrokerService.exe
