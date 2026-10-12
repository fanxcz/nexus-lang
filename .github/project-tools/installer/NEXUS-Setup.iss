#define MyAppName "NEXUS"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "NEXUS Project"
#ifndef SourceDir
  #define SourceDir "..\\..\\..\\dist\\NEXUS_Windows_Portable"
#endif

[Setup]
AppId={{B41A4B2E-5DF0-4BDE-9B1B-4D2E90B5B1A1}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/fanxcz/nexus-lang
AppSupportURL=https://github.com/fanxcz/nexus-lang/issues
AppUpdatesURL=https://fanxcz.github.io/nexus-lang/
DefaultDirName={localappdata}\\Programs\\NEXUS
DefaultGroupName=NEXUS
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
WizardStyle=modern
LanguageDetectionMethod=uilanguage
UninstallDisplayIcon={app}\\bin\\nexus-core.exe
OutputBaseFilename=NEXUS-Setup-1.1.0
Compression=lzma2/ultra64
SolidCompression=yes
ChangesEnvironment=yes
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
VersionInfoVersion=1.1.0.0
VersionInfoProductName=NEXUS
VersionInfoProductVersion=1.1.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=NEXUS native programming language installer
UninstallDisplayName=NEXUS 1.1.0

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык NEXUS на рабочем столе"; GroupDescription: "Ярлыки:"; Flags: checkedonce
Name: "startmenuicon"; Description: "Создать ярлыки в меню «Пуск»"; GroupDescription: "Ярлыки:"; Flags: checkedonce
Name: "addtopath"; Description: "Добавить команду nexus в PATH пользователя"; GroupDescription: "Интеграция:"; Flags: checkedonce
Name: "associate_nx"; Description: "Добавить пункт «Запустить с NEXUS» для файлов .nx"; GroupDescription: "Интеграция:"; Flags: unchecked

[Files]
Source: "{#SourceDir}\\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autodesktop}\\NEXUS Console"; Filename: "{app}\\NEXUS-Console.bat"; WorkingDir: "{app}"; IconFilename: "{sys}\\cmd.exe"; Tasks: desktopicon
Name: "{autoprograms}\\NEXUS\\NEXUS Console"; Filename: "{app}\\NEXUS-Console.bat"; WorkingDir: "{app}"; IconFilename: "{sys}\\cmd.exe"; Tasks: startmenuicon
Name: "{autoprograms}\\NEXUS\\Run hello.nx example"; Filename: "{app}\\Run-NEXUS.bat"; WorkingDir: "{app}"; IconFilename: "{sys}\\cmd.exe"; Tasks: startmenuicon

[UninstallDelete]
Type: filesandordirs; Name: "{app}\\_setup_temp"

[Code]
const
  UserEnvironmentSubKey = 'Environment';
  NexusContextMenuKey = 'Software\\Classes\\SystemFileAssociations\\.nx\\shell\\NEXUS';

function NormalizePathEntry(const Value: string): string;
var
  S: string;
begin
  S := Trim(Value);
  while (Length(S) > 0) and (S[1] = '"') do
    Delete(S, 1, 1);
  while (Length(S) > 0) and (S[Length(S)] = '"') do
    Delete(S, Length(S), 1);
  S := Trim(S);
  if S = '' then
  begin
    Result := '';
    Exit;
  end;
  Result := RemoveBackslashUnlessRoot(ExpandFileName(S));
end;

function PathContains(const PathValue, Target: string): Boolean;
var
  StartAt: Integer;
  EndAt: Integer;
  Entry: string;
  NormalizedTarget: string;
begin
  Result := False;
  NormalizedTarget := NormalizePathEntry(Target);
  StartAt := 1;
  while StartAt <= Length(PathValue) do
  begin
    EndAt := StartAt;
    while (EndAt <= Length(PathValue)) and (PathValue[EndAt] <> ';') do
      EndAt := EndAt + 1;
    Entry := NormalizePathEntry(Copy(PathValue, StartAt, EndAt - StartAt));
    if (Entry <> '') and (CompareText(Entry, NormalizedTarget) = 0) then
    begin
      Result := True;
      Exit;
    end;
    StartAt := EndAt + 1;
  end;
end;

procedure AddNexusToUserPath;
var
  ExistingPath: string;
  AppPath: string;
  NewPath: string;
begin
  AppPath := RemoveBackslashUnlessRoot(ExpandConstant('{app}'));
  ExistingPath := '';
  RegQueryStringValue(HKEY_CURRENT_USER, UserEnvironmentSubKey, 'Path', ExistingPath);
  if not PathContains(ExistingPath, AppPath) then
  begin
    if Trim(ExistingPath) = '' then
      NewPath := AppPath
    else
      NewPath := ExistingPath + ';' + AppPath;
    if not RegWriteStringValue(HKEY_CURRENT_USER, UserEnvironmentSubKey, 'Path', NewPath) then
      MsgBox('Не удалось изменить PATH пользователя. NEXUS всё равно установлен. Добавьте эту папку в PATH вручную:' + #13#10 + AppPath, mbError, MB_OK);
  end;
end;

procedure RemoveNexusFromUserPath;
var
  ExistingPath: string;
  NewPath: string;
  StartAt: Integer;
  EndAt: Integer;
  AppPath: string;
  Entry: string;
begin
  ExistingPath := '';
  if not RegQueryStringValue(HKEY_CURRENT_USER, UserEnvironmentSubKey, 'Path', ExistingPath) then
    Exit;

  AppPath := NormalizePathEntry(ExpandConstant('{app}'));
  NewPath := '';
  StartAt := 1;
  while StartAt <= Length(ExistingPath) do
  begin
    EndAt := StartAt;
    while (EndAt <= Length(ExistingPath)) and (ExistingPath[EndAt] <> ';') do
      EndAt := EndAt + 1;
    Entry := Trim(Copy(ExistingPath, StartAt, EndAt - StartAt));
    if (Entry <> '') and (CompareText(NormalizePathEntry(Entry), AppPath) <> 0) then
    begin
      if NewPath <> '' then
        NewPath := NewPath + ';';
      NewPath := NewPath + Entry;
    end;
    StartAt := EndAt + 1;
  end;

  if NewPath <> ExistingPath then
  begin
    if NewPath = '' then
      RegDeleteValue(HKEY_CURRENT_USER, UserEnvironmentSubKey, 'Path')
    else
      RegWriteStringValue(HKEY_CURRENT_USER, UserEnvironmentSubKey, 'Path', NewPath);
  end;
end;

procedure CreateNexusContextMenu;
var
  CommandKey: string;
  CommandLine: string;
begin
  RegWriteStringValue(HKEY_CURRENT_USER, NexusContextMenuKey, '', 'Запустить с NEXUS');
  RegWriteStringValue(HKEY_CURRENT_USER, NexusContextMenuKey, 'Icon', ExpandConstant('{app}\\bin\\nexus-core.exe'));
  CommandKey := NexusContextMenuKey + '\\command';
  CommandLine := '"' + ExpandConstant('{sys}\\WindowsPowerShell\\v1.0\\powershell.exe') +
    '" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "' +
    ExpandConstant('{app}\\NEXUS-Windows.ps1') + '" -Mode Run -SourcePath "%1"';
  RegWriteStringValue(HKEY_CURRENT_USER, CommandKey, '', CommandLine);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  PowerShell: string;
  Parameters: string;
begin
  if CurStep = ssPostInstall then
  begin
    PowerShell := ExpandConstant('{sys}\\WindowsPowerShell\\v1.0\\powershell.exe');
    Parameters := '-NoLogo -NoProfile -ExecutionPolicy Bypass -File "' +
      ExpandConstant('{app}\\NEXUS-Windows.ps1') + '" -Mode Check';

    WizardForm.StatusLabel.Caption := 'Проверка встроенного компилятора NEXUS...';
    WizardForm.Refresh;
    if Exec(PowerShell, Parameters, ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    begin
      if ResultCode <> 0 then
        MsgBox('NEXUS установлен, но проверка компилятора завершилась с ошибкой.' + #13#10 +
          'Откройте NEXUS Console и запустите: nexus doctor' + #13#10 +
          'Если проблема останется, переустановите программу из официального установщика.',
          mbError, MB_OK);
    end
    else
      MsgBox('Не удалось запустить PowerShell для проверки NEXUS. Файлы установлены, но проверьте запуск вручную командой nexus doctor.',
        mbError, MB_OK);

    if IsTaskSelected('addtopath') then
      AddNexusToUserPath;
    if IsTaskSelected('associate_nx') then
      CreateNexusContextMenu;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    RemoveNexusFromUserPath;
    if RegKeyExists(HKEY_CURRENT_USER, NexusContextMenuKey) then
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, NexusContextMenuKey);
  end;
end;
