; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#include "version.h"

#define MyAppPublisher "mooe"
#define MyAppURL "https://www.hiscene.com/"
#define MyAppName "Զ�̼�ʻ2.0"
#define MyAppExeName "driver" + ".exe"
#define AppReleaseDir "..\out\x64\Release"
#define AppIcon "..\driver\res\images\favicon.ico"
#define PackageBaseName="RemoteDrive"

#define MyAppVersion AppVersion
#define MyAppAssocName MyAppName + " File"
#define MyAppAssocExt ".myp"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
;PrivilegesRequired=admin 
AppId={{5B378203-64A3-48A8-8FB2-8E1AC798BE9B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commonpf}\{#MyAppName}
;���Զ�ʶ��֮ǰ�İ�װĿ¼
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
; Remove the following line to run in administrative install mode (install for all users.)
;SetupIconFile= {#MyAppOutDir}\ico.ico
SetupIconFile= {#AppIcon}
;UninstallDisplayIcon={#MyAppOutDir}\ico.ico  
UninstallDisplayIcon={#AppIcon}
OutputDir=.
OutputBaseFilename={#PackageBaseName}_{#MyAppVersion}_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "zh"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
;Name: "en"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkablealone

[Files]
Source: "{#AppReleaseDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppReleaseDir}\*"; DestDir: "{app}"; Excludes: "*.lib,*.pdb,{#MyAppExeName}.manifest,vc_redist.x64.exe,download"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".myp"; ValueData: ""

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
;Filename: "{app}\vc_redist.x64.exe";

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
	DelTree(ExpandConstant('{app}'), True, True, True);
  end;
end;
