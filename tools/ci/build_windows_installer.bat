echo OFF

REM Build the RDMnetBroker Windows installer

REM Arguments: none
REM
REM Directory structure
REM   rdmnetbroker
REM     build
REM       install_x86
REM         bin
REM       install_x64
REM         bin
REM     tools
REM       ci
REM       install
REM         windows
REM Requires:
REM - python
REM - batch file called from rdmnetbroker\tools\ci directory
REM - %ProgramFiles(x86)%\NSIS\makensis.exe
REM - environment variable ARTIFACT_TYPE = "x86" or "x64"
REM - rdmnetbroker\build\CMakeCache.txt
REM - rdmnetbroker\build\_deps\rdmnet-src\cmake\dnssd\windows.cmake
REM - rdmnetbroker\tools\ci\copy_vc_redist_to_install.py
REM - rdmnetbroker\tools\install\windows\uninstall.nsi
REM - rdmnetbroker\tools\install\windows\install.nsi
REM
REM Output:
REM - signed RDMnetBroker_%ARTIFACT_TYPE%.exe
REM
REM Assumptions:
REM - this batch file is called from the rdmnetbroker\tools\ci directory
REM - previous pipeline stage has created (required by install.nsi)
REM     C:\git\rdmnetbroker\build\install_x86\bin\dnssd.dll
REM     C:\git\rdmnetbroker\build\install_x86\bin\RDMnetBrokerService.exe
REM
REM       and
REM
REM     C:\git\rdmnetbroker\build\install_x64\bin\dnssd.dll
REM     C:\git\rdmnetbroker\build\install_x64\bin\RDMnetBrokerService.exe
REM
REM Example:
REM   C:\git>cd rdmnetbroker\tools\ci
REM   C:\git\rdmnetbroker\tools\ci>build_windows_installer.bat

ECHO cd ..\..
cd ..\..

REM we are now in the rdmnetbroker directory

REM run RDMnet script to download mdnswindows_install\ETC_mDNSInstall.exe
ECHO cmake -P build\_deps\rdmnet-src\cmake\dnssd\windows.cmake
cmake -P build\_deps\rdmnet-src\cmake\dnssd\windows.cmake
REM do NOT check error level!

REM rdmnetbroker\mdnswindows_install\ETC_mDNSInstall.exe now exists

ECHO cd tools\ci
cd tools\ci

REM run copy_vc_redist_to_install.py
ECHO python copy_vc_redist_to_install.py ..\..\build\CMakeCache.txt ..\..\tools\install\windows
python copy_vc_redist_to_install.py ..\..\build\CMakeCache.txt ..\..\tools\install\windows

REM vc_redist.x86.exe or vc_redist.x64.exe now exists in tools\install\windows

ECHO cd ..\..
cd ..\..

ECHO Initializing signing tool
set ETC_SIGN_CREDS=%NET_CODESIGN_USER%:%NET_CODESIGN_PASS%
ECHO call build\_deps\etc_sign-src\etc_sign.bat --initcert
call build\_deps\etc_sign-src\etc_sign.bat --initcert

ECHO Signing ETC_mDNSInstall.exe
ECHO call build\_deps\etc_sign-src\etc_sign.bat mdnswindows_install\ETC_mDNSInstall.exe
call build\_deps\etc_sign-src\etc_sign.bat mdnswindows_install\ETC_mDNSInstall.exe

ECHO cd tools\install\windows
cd tools\install\windows

ECHO Build the RDMnetBroker installer

REM During "normal" use NSIS 3.10 does not allow the uninstaller to be signed. It creates the uninstaller
REM in a temporary directory, adds it to the installer, and delete the uninstaller. The next few steps involve
REM two separate NSIS script. The first script generates the uninstaller, wrapped in a "fake" installer. The
REM fake installer only contains the uninstaller, and installs it in the same directory as the installer.
REM
REM The fake installer is run, and then deleted. The uninstaller is then signed and is available for the
REM second NSIS script which generates the "real" RDMnetBroker installer.

REM create a fake RDMnetBroker installer fake_install.exe
ECHO "%ProgramFiles(x86)%\NSIS\makensis.exe" uninstall.nsi
"%ProgramFiles(x86)%\NSIS\makensis.exe" uninstall.nsi
if %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

REM wait a few seconds for the fake installer to become available
ECHO ping 127.0.0.1 -n 6 >nul
ping 127.0.0.1 -n 6 >nul

REM run the fake installer in silent mode to "install" the uninstaller
ECHO call fake_install.exe /S
call fake_install.exe /S

REM the fake installer does not get checked in to Git
ECHO del fake_install.exe
del fake_install.exe

REM wait a few seconds for the uninstaller to become available
ECHO ping 127.0.0.1 -n 6 >nul
ping 127.0.0.1 -n 6 >nul

if %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

ECHO Signing RDMnetBroker uninstaller
REM The unstaller file name does NOT include "_x86" / "_x64"
ECHO call ..\..\..\build\_deps\etc_sign-src\etc_sign.bat ETC_RDMnetBroker_Uninstall.exe
call ..\..\..\build\_deps\etc_sign-src\etc_sign.bat ETC_RDMnetBroker_Uninstall.exe
REM if %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

REM copy mDNSWindows installer
ECHO copy ..\..\..\build\mdnswindows_install\ETC_mDNSInstall.exe .
copy ..\..\..\build\mdnswindows_install\ETC_mDNSInstall.exe .

REM copy RDMnet Broker Service file to current directory
ECHO copy ..\..\..\build\install_%ARTIFACT_TYPE%\bin\dnssd.dll .
copy ..\..\..\build\install_%ARTIFACT_TYPE%\bin\dnssd.dll .
ECHO copy ..\..\..\build\install_%ARTIFACT_TYPE%\bin\RDMnetBrokerService.exe .
copy ..\..\..\build\install_%ARTIFACT_TYPE%\bin\RDMnetBrokerService.exe .

REM create the installer
ECHO "%ProgramFiles(x86)%\NSIS\makensis.exe" install.nsi
"%ProgramFiles(x86)%\NSIS\makensis.exe" install.nsi
if %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

ECHO Signing RDMnetBroker installer
ECHO call ..\..\..\build\_deps\etc_sign-src\etc_sign.bat RDMnetBroker_%ARTIFACT_TYPE%.exe
call ..\..\..\build\_deps\etc_sign-src\etc_sign.bat RDMnetBroker_%ARTIFACT_TYPE%.exe
REM if %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

REM delete the uninstaller
ECHO del ETC_RDMnetBroker_Uninstall.exe
del ETC_RDMnetBroker_Uninstall.exe

REM display files
ECHO dir
dir

REM copy installer to root directory
ECHO copy RDMnetBroker_%ARTIFACT_TYPE%.exe ..\..\..
copy RDMnetBroker_%ARTIFACT_TYPE%.exe ..\..\..

REM verify copy
ECHO cd ..\..\..
cd ..\..\..
ECHO dir
dir

REM return to the tools\ci directory where we started
ECHO cd tools\ci
cd tools\ci

