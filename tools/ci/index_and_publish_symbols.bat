@rem Crazy gymnastics to get the current git hash into an environment variable:
echo|set /p="set PROJECT_HASH=">sethash.bat
call git log -1 --pretty=%%%%H >>sethash.bat
call sethash.bat
del sethash.bat

:: Copy srcsrv into temp to ensure script cannot break it
mkdir %TEMP%\srcsrv
robocopy "%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\srcsrv" "%TEMP%\srcsrv" /np /mir

@rem execute github source indexing powershell script
@rem Need to assemble a URL like this:
@rem https://raw.githubusercontent.com/ETCLabs/RDMnetBroker/60a3a468842b119603717852b7b76bbc28ecc8e7/.clang-format

powershell C:\tools\ci\github-sourceindexer.ps1 -dbgToolsPath "%TEMP%\srcsrv" -gitHubUrl https://raw.githubusercontent.com -serverIsRaw ^
  -userId "ETCLabs" ^
  -repository "RDMnetBroker" ^
  -branch %PROJECT_HASH% ^
  -symbolsFolder "%cd%\symbols" ^
  -sourcesRoot "%cd%" ^
  -ignoreUnknown -verifyLocalRepo

curl -o %TEMP%\cacert.pem http://etc.gitlab-pages.etcconnect.com/cacert/wildcard-cert-chain.pem

python C:\tools\ci\Symbols\publish_symbols.py ^
  -n "RDMnetBrokerService" ^
  -v %NEW_BUILD_VERSION% ^
  -a %COMPILER_ARCH% ^
  -u %ETC_COMMON_TECH_ARTIFACTORY_URL% ^
  -r NET ^
  -k %RDMNETBRKR_ARTIFACTORY_API_KEY% ^
  --os windows ^
  -d .\symbols ^
  --cert %TEMP%\cacert.pem
  .\build\%CMAKE_INSTALL%\bin\RDMnetBrokerService.exe
