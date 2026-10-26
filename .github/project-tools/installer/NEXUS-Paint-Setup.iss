#define MyAppName "NEXUS Paint"
#define MyAppVersion "2.1.0"
#define MyAppPublisher "NEXUS Project"
#ifndef SourceDir
  #define SourceDir "..\..\..\dist\NEXUS-Paint-Windows-x64"
#endif

[Setup]
AppId={{CEFE9C5C-8344-4D16-9D1C-77FA8C2AB83D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/fanxcz/nexus-lang
AppSupportURL=https://github.com/fanxcz/nexus-lang/issues
AppUpdatesURL=https://fanxcz.github.io/nexus-lang/downloads/
DefaultDirName={localappdata}\Programs\NEXUS Paint
DefaultGroupName=NEXUS Paint
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
WizardStyle=modern
LanguageDetectionMethod=uilanguage
UninstallDisplayIcon={app}\NEXUS-Paint.exe
OutputBaseFilename=NEXUS-Paint-Windows-x64
Compression=lzma2/fast
SolidCompression=yes
ChangesEnvironment=no
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
VersionInfoVersion=2.1.0.0
VersionInfoProductName=NEXUS Paint
VersionInfoProductVersion=2.1.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=NEXUS Paint raster editor installer
UninstallDisplayName=NEXUS Paint 2.0.0

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: checkedonce
Name: "startmenuicon"; Description: "Create a Start menu shortcut"; GroupDescription: "Shortcuts:"; Flags: checkedonce

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autodesktop}\NEXUS Paint"; Filename: "{app}\NEXUS-Paint.exe"; WorkingDir: "{app}"; Tasks: desktopicon
Name: "{autoprograms}\NEXUS Paint\NEXUS Paint"; Filename: "{app}\NEXUS-Paint.exe"; WorkingDir: "{app}"; Tasks: startmenuicon
Name: "{autoprograms}\NEXUS Paint\NEXUS Paint Source"; Filename: "{app}\NEXUS-Paint.nx"; Tasks: startmenuicon
