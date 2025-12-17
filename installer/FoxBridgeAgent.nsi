; FoxBridgeAgent NSIS Installer Script
; Alternative to WiX for simpler installation

!include "MUI2.nsh"
!include "LogicLib.nsh"

; General
Name "FoxBridgeAgent"
OutFile "FoxBridgeAgent-Setup.exe"
InstallDir "$PROGRAMFILES64\FoxBridgeAgent"
InstallDirRegKey HKLM "Software\FoxBridgeAgent" "Install_Dir"
RequestExecutionLevel admin

; Version Info
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "FoxBridgeAgent"
VIAddVersionKey "CompanyName" "YourCompany"
VIAddVersionKey "FileDescription" "HTTP API Server for Visual FoxPro / ExpressD"
VIAddVersionKey "FileVersion" "1.0.0.0"

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
Page custom ConfigPage ConfigPageLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

; Variables
Var DatabasePath
Var ApiKey
Var HttpPort
Var CloudflareToken

; Custom Page for Configuration
Function ConfigPage
  !insertmacro MUI_HEADER_TEXT "Configuration" "Enter FoxBridgeAgent configuration"
  
  nsDialogs::Create 1018
  Pop $0
  
  ${If} $0 == error
    Abort
  ${EndIf}
  
  ; Database Path
  ${NSD_CreateLabel} 0 10u 100% 12u "Database Path (ExpressD DBF folder):"
  Pop $0
  ${NSD_CreateDirRequest} 0 24u 85% 12u "$DatabasePath"
  Pop $DatabasePath
  ${NSD_CreateBrowseButton} 87% 24u 13% 12u "..."
  Pop $1
  ${NSD_OnClick} $1 BrowseDatabase
  
  ; API Key
  ${NSD_CreateLabel} 0 45u 100% 12u "API Key (for authentication):"
  Pop $0
  ${NSD_CreatePassword} 0 59u 100% 12u "$ApiKey"
  Pop $ApiKey
  
  ; HTTP Port
  ${NSD_CreateLabel} 0 80u 100% 12u "HTTP Port:"
  Pop $0
  ${NSD_CreateNumber} 0 94u 50% 12u "8787"
  Pop $HttpPort
  
  ; Cloudflare Token
  ${NSD_CreateLabel} 0 115u 100% 12u "Cloudflare Tunnel Token (optional):"
  Pop $0
  ${NSD_CreateText} 0 129u 100% 12u "$CloudflareToken"
  Pop $CloudflareToken
  
  nsDialogs::Show
FunctionEnd

Function BrowseDatabase
  nsDialogs::SelectFolderDialog "Select ExpressD Database Folder" "$DatabasePath"
  Pop $0
  ${If} $0 != error
    StrCpy $DatabasePath $0
  ${EndIf}
FunctionEnd

Function ConfigPageLeave
  ${NSD_GetText} $DatabasePath $DatabasePath
  ${NSD_GetText} $ApiKey $ApiKey
  ${NSD_GetText} $HttpPort $HttpPort
  ${NSD_GetText} $CloudflareToken $CloudflareToken
  
  ; Validate
  ${If} $DatabasePath == ""
    MessageBox MB_ICONEXCLAMATION "Database Path is required!"
    Abort
  ${EndIf}
  
  ${If} $ApiKey == ""
    MessageBox MB_ICONEXCLAMATION "API Key is required!"
    Abort
  ${EndIf}
FunctionEnd

; Installation Section
Section "Install"
  SetOutPath "$INSTDIR\bin"
  
  ; Copy executable
  File "..\build\bin\Release\FoxBridgeAgent.exe"
  
  ; Create ProgramData directory
  CreateDirectory "$APPDATA\FoxBridgeAgent"
  CreateDirectory "$APPDATA\FoxBridgeAgent\logs"
  
  ; Create config.json
  FileOpen $0 "$APPDATA\FoxBridgeAgent\config.json" w
  FileWrite $0 '{$\r$\n'
  FileWrite $0 '  "database_path": "$DatabasePath",$\r$\n'
  FileWrite $0 '  "api_key": "$ApiKey",$\r$\n'
  FileWrite $0 '  "port": $HttpPort,$\r$\n'
  FileWrite $0 '  "cloudflare_token": "$CloudflareToken",$\r$\n'
  FileWrite $0 '  "log_level": "info",$\r$\n'
  FileWrite $0 '  "log_path": "C:\\ProgramData\\FoxBridgeAgent\\logs",$\r$\n'
  FileWrite $0 '  "index_policy": "auto",$\r$\n'
  FileWrite $0 '  "maintenance_window": "02:00-04:00",$\r$\n'
  FileWrite $0 '  "max_retry_attempts": 3,$\r$\n'
  FileWrite $0 '  "connection_timeout": 30$\r$\n'
  FileWrite $0 '}$\r$\n'
  FileClose $0
  
  ; Write registry
  WriteRegStr HKLM "Software\FoxBridgeAgent" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FoxBridgeAgent" "DisplayName" "FoxBridgeAgent"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FoxBridgeAgent" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FoxBridgeAgent" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FoxBridgeAgent" "NoRepair" 1
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Install Windows Service
  DetailPrint "Installing Windows Service..."
  ExecWait '"$INSTDIR\bin\FoxBridgeAgent.exe" --install' $0
  
  ${If} $0 == 0
    DetailPrint "Configuring service auto-start and recovery..."
    
    ; Configure service recovery (restart on failure)
    ; Restart after 60 seconds on first failure
    ; Restart after 120 seconds on second failure  
    ; Restart after 300 seconds on subsequent failures
    ; Reset failure count after 24 hours (86400 seconds)
    ExecWait 'sc.exe failure FoxBridgeAgent reset= 86400 actions= restart/60000/restart/120000/restart/300000' $2
    
    ${If} $2 == 0
      DetailPrint "Service recovery configured successfully"
    ${Else}
      DetailPrint "Warning: Failed to configure service recovery"
    ${EndIf}
    
    ; Configure service description
    ExecWait 'sc.exe description FoxBridgeAgent "HTTP API Server for Visual FoxPro ExpressD database. Provides REST API access to DBF files with multi-user safety. Automatically starts on system boot."' $2
    
    ; Configure delayed auto-start (starts after critical services)
    ExecWait 'sc.exe config FoxBridgeAgent start= delayed-auto' $2
    
    DetailPrint "Starting Windows Service..."
    ExecWait '"$INSTDIR\bin\FoxBridgeAgent.exe" --start' $1
    
    ${If} $1 == 0
      MessageBox MB_ICONINFORMATION "FoxBridgeAgent installed and started successfully!$\n$\nService will automatically start on system boot.$\nIf the service crashes, it will restart automatically."
    ${Else}
      MessageBox MB_ICONWARNING "Service installed but failed to start. Check logs.$\n$\nNote: Service is configured to start on next boot."
    ${EndIf}
  ${Else}
    MessageBox MB_ICONWARNING "Failed to install Windows Service. Check permissions."
  ${EndIf}
  
SectionEnd

; Uninstall Section
Section "Uninstall"
  ; Stop and uninstall service
  ExecWait '"$INSTDIR\bin\FoxBridgeAgent.exe" --stop'
  ExecWait '"$INSTDIR\bin\FoxBridgeAgent.exe" --uninstall'
  
  ; Remove files
  Delete "$INSTDIR\bin\FoxBridgeAgent.exe"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR"
  
  ; Remove config (ask user)
  MessageBox MB_YESNO "Remove configuration and logs?" IDYES RemoveConfig IDNO SkipRemoveConfig
  RemoveConfig:
    RMDir /r "$APPDATA\FoxBridgeAgent"
  SkipRemoveConfig:
  
  ; Remove registry
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FoxBridgeAgent"
  DeleteRegKey HKLM "Software\FoxBridgeAgent"
  
SectionEnd
