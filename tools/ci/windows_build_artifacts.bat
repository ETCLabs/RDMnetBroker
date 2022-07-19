call call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" %VCVARSALL_PLATFORM% %VCVARSALL_PLATFORM%

MSBuild.exe -property:Configuration=Release -property:Platform=%ARTIFACT_TYPE% .\tools\install\windows\merge\BrokerMerge_%ARTIFACT_TYPE%.wixproj
IF %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )
MSBuild.exe -property:Configuration=Release -property:Platform=%ARTIFACT_TYPE% .\tools\install\windows\standalone\BrokerStandalone_%ARTIFACT_TYPE%.wixproj
IF %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

copy tools\install\windows\merge\bin\Release\RDMnetBroker_%ARTIFACT_TYPE%.msm .\RDMnetBroker_%ARTIFACT_TYPE%.msm
IF %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )
copy tools\install\windows\standalone\bin\Release\RDMnetBroker_%ARTIFACT_TYPE%.msi .\RDMnetBroker_%ARTIFACT_TYPE%.msi
IF %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )

signtool.exe sign /v /a /tr "http://timestamp.digicert.com" /td sha256 /fd sha256 /f "C:\certs\ETCCert2021DigicertCodeSigningSHA256.pfx" /p %RDMNETBRKR_CODESIGN_CERT_SECRET% RDMnetBroker_%ARTIFACT_TYPE%.msi > NUL
IF %ERRORLEVEL% NEQ 0 ( EXIT /B %ERRORLEVEL% )
