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
; !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
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
  !insertmacro MUI_HEADER_TEXT "Database Configuration" "Configure FoxBridgeAgent Settings"
  
  nsDialogs::Create 1018
  Pop $0
  
  ${If} $0 == error
    Abort
  ${EndIf}
  
  ; Set default API Key if empty
  ${If} $ApiKey == ""
    StrCpy $ApiKey "quCtcMFsFNw3zwOFxOAJxFKaOdpbuwftKzMelJCVvks="
  ${EndIf}
  
  ; Set default port if empty
  ${If} $HttpPort == ""
    StrCpy $HttpPort "8080"
  ${EndIf}
  
  ; Info text
  ${NSD_CreateLabel} 0 0u 100% 12u "Please specify the folder containing your .DBF files:"
  Pop $0
  
  ; Database Path
  ${NSD_CreateLabel} 0 15u 100% 12u "Database Path (folder with .DBF files):"
  Pop $0
  ${NSD_CreateDirRequest} 0 29u 85% 12u "$DatabasePath"
  Pop $DatabasePath
  ${NSD_CreateBrowseButton} 87% 29u 13% 12u "Browse..."
  Pop $1
  ${NSD_OnClick} $1 BrowseDatabase
  
  ; HTTP Port
  ${NSD_CreateLabel} 0 50u 50% 12u "HTTP Port:"
  Pop $0
  ${NSD_CreateNumber} 0 64u 30% 12u "$HttpPort"
  Pop $HttpPort
  
  ; Port hint
  ${NSD_CreateLabel} 35% 66u 65% 10u "(Default: 8080)"
  Pop $0
  
  ; API Key
  ${NSD_CreateLabel} 0 85u 100% 12u "API Key (auto-generated, keep it safe):"
  Pop $0
  ${NSD_CreateText} 0 99u 100% 12u "$ApiKey"
  Pop $ApiKey
  
  ; API Key hint
  ${NSD_CreateLabel} 0 113u 100% 10u "This key is required for API authentication"
  Pop $0
  
  ; Validate Database Path
  ${If} $DatabasePath == ""
    MessageBox MB_ICONEXCLAMATION "Please specify the Database Path!$\n$\nThis is the folder containing your .DBF files."
    Abort
  ${EndIf}
  
  ; Check if path exists
  IfFileExists "$DatabasePath\*.*" PathExists PathNotExists
  PathNotExists:
    MessageBox MB_ICONQUESTION|MB_YESNO "The specified folder does not exist:$\n$\n$DatabasePath$\n$\nDo you want to continue anyway?" IDYES PathExists
    Abort
  PathExists:
  
  ; Validate API Key
  ${If} $ApiKey == ""
    MessageBox MB_ICONEXCLAMATION "API Key cannot be empty!"
    Abort
  ${EndIf}
  
  ; Validate Port
  ${If} $HttpPort == ""
    StrCpy $HttpPort "8080"nd

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
 with proper format
  DetailPrint "Creating configuration file..."
  
  ; Escape backslashes for JSON
  Push "$DatabasePath"
  Call EscapeBackslashes
  Pop $0
  
  FileOpen $1 "$APPDATA\FoxBridgeAgent\config.json" w
  FileWrite $1 '{$\r$\n'
  FileWrite $1 '  "database": {$\r$\n'
  FileWrite $1 '    "type": "vfp_odbc",$\r$\n'
  FileWrite $1 '    "dsn": "VFP_ExpressD",$\r$\n'
  FileWrite $1 '    "connection_string": "Driver={Microsoft Visual FoxPro Driver};SourceType=DBF;SourceDB=$0;Exclusive=No;",$\r$\n'
  FileWrite $1 '    "dbf_path": "$0"$\r$\n'
  FileWrite $1 '  },$\r$\n'
  FileWrite $1 '  "server": {$\r$\n'
  FileWrite $1 '    "host": "0.0.0.0",$\r$\n'
  FileWrite $1 '    "port": $HttpPort,$\r$\n'
  FileWrite $1 '    "api_key": "$ApiKey"$\r$\n'
  FileWrite $1 '  },$\r$\n'
  FileWrite $1 '  "cloudflare": {$\r$\n'
  FileWrite $1 '    "enabled": false$\r$\n'
  FileWrite $1 '  },$\r$\n'
  FileWrite $1 '  "logging": {$\r$\n'
  FileWrite $1 '    "level": "info",$\r$\n'
  FileWrite $1 '    "path": "C:\\ProgramData\\FoxBridgeAgent\\logs"$\r$\n'
  FileWrite $1 '  },$\r$\n'
  FileWrite $1 '  "maintenance": {$\r$\n'
  FileWrite $1 '    "auto_reindex": true,$\r$\n'
  FileWrite $1 '    "check_interval_minutes": 60,$\r$\n'
  FileWrite $1 '    "backup_before_pack": true$\r$\n'
  FileWrite $1 '  }$\r$\n'
  FileWrite $1 '}$\r$\n'
  FileClose $1
  
  DetailPrint "Configuration saved to: $APPDATA\FoxBridgeAgent\config.json"

; Function to escape backslashes for JSON
Function EscapeBackslashes
  Exch $0
  Push $1
  Push $2
  
  StrCpy $1 ""
  StrLen $2 $0
  
  ${While} $2 > 0
    StrCpy $1 "$0" 1
    ${If} $1 == "\"
      StrCpy $1 "\\"
    ${EndIf}
    StrCpy $0 "$0" "" 1
    IntOp $2 $2 - 1
  ${EndWhile}
  
  Pop $2
  Pop $1
  Exch $0
FunctionEnd '{$\r$\n'
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
