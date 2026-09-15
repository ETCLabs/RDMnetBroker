# uninstall.nsi
#
# Do not edit uninstall.nsi, edit uninstall.nsi.in
#
# Create the Windows uninstaller for RDMnet Broker
#
# Usage: "C:\Program Files (x86)\NSIS\makensis.exe" uninstall.nsi

!include "WinVer.nsh"
!include "WordFunc.nsh"
!include "Registry.nsh"

# ; MUI 1.67 compatible ------
!include "MUI.nsh"

# ; MUI Settings
!define MUI_ABORTWARNING
!define MUI_UNICON "ETCIconDark_NSIS.ico"
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"
# MUI end ------

!if "$%ARTIFACT_TYPE%" == "${U+24}%ARTIFACT_TYPE%"
  !error "Error environment variable ARTIFACT_TYPE not defined"
!endif

!define PRODUCT_NAME "ETC RDMnet Broker"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\{9ABA3493-191A-497A-8C9C-D676AD9DDAFD}"
!define PRODUCT_VERSION "1.0.0.12"

# the name of the uninstaller
Outfile "fake_install.exe"

# Request application privileges for Windows Vista
RequestExecutionLevel admin

# define the friendly name displayed in the installer pages
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"

# Show install details
ShowInstDetails show

# default section
Section Uninstall
  # this will affect all users
  SetShellVarContext all

  DetailPrint "Stopping ${PRODUCT_NAME}"
  DetailPrint "$SYSDIR\sc.exe stop ${PRODUCT_NAME}"
  ExecWait '$SYSDIR\sc.exe stop "${PRODUCT_NAME}"'

  DetailPrint "Deleting ${PRODUCT_NAME}"
  DetailPrint "$SYSDIR\sc.exe delete ${PRODUCT_NAME}"
  ExecWait '$SYSDIR\sc.exe delete "${PRODUCT_NAME}"'

  IfFileExists $PROGRAMFILES64\ETC\RDMnetBroker\RDMnetBrokerService.exe 0 skip_remove
    # remove RDMnet Broker service
    DetailPrint "$PROGRAMFILES64\ETC\RDMnetBroker\RDMnetBrokerService.exe -remove"
    ExecWait '"$PROGRAMFILES64\ETC\RDMnetBroker\RDMnetBrokerService.exe" -remove'
skip_remove:

  DetailPrint "Removing Firewall Exception"
  ExecWait 'netsh advfirewall firewall delete rule name="ETC RDMnet Broker (TCP-in)"'
  ExecWait 'netsh advfirewall firewall delete rule name="ETC RDMnet Broker (UDP-in)"'

  !if "$%ARTIFACT_TYPE%" == "x64"  
    SetRegView 64
  !endif

  # delete keys used by Control Panel "Uninstall" screen
  DeleteRegKey "HKLM" "${UNINSTALL_KEY}\"

  RMDir /r "$PROGRAMFILES64\ETC\RDMnetBroker"

  # finally, delete the uninstaller itself
  Delete "$PROGRAMFILES64\ETC\RDMnetBroker\ETC_RDMnetBroker_Uninstall.exe"

  SetAutoClose true
SectionEnd

Section "Application"
  # only "install" the uninstaller
  SetOutPath "$PROGRAMFILES64\ETC\RDMnetBroker"
  WriteUninstaller "$EXEDIR\ETC_RDMnetBroker_Uninstall.exe"
SectionEnd
