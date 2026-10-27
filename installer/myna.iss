; Inno Setup script for the Myna Windows installer.
;
; The release workflow builds it with:
;   iscc /DAppVersion=0.1.1 /DSourceDir=<package folder> installer\myna.iss
; where the package folder holds bin\myna.exe, data\, README.md and LICENSE.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\build\package"
#endif

[Setup]
; AppId identifies Myna to Windows across upgrades. Never change it.
AppId={{6F3B2C1A-8E4D-4B7A-9C2E-5D1F0A7B3E91}
AppName=Myna
AppVersion={#AppVersion}
AppVerName=Myna {#AppVersion}
AppPublisher=Myna contributors
AppPublisherURL=https://github.com/Afueth/Myna
AppSupportURL=https://github.com/Afueth/Myna/issues
AppUpdatesURL=https://github.com/Afueth/Myna/releases
DefaultDirName={autopf}\Myna
DefaultGroupName=Myna
DisableProgramGroupPage=yes
LicenseFile={#SourceDir}\LICENSE
; Installs for the current user without admin rights; users can choose
; "all users" in the first dialog.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ChangesEnvironment=yes
; Myna often runs in the background as an MCP server for AI tools. Those
; processes have no window, so they cannot be asked to close: end them, and
; leave restarting to the AI tool, which starts Myna again when it reconnects.
CloseApplications=force
RestartApplications=no
SetupIconFile=..\assets\myna.ico
UninstallDisplayIcon={app}\bin\myna.exe
UninstallDisplayName=Myna
OutputDir=..\dist
OutputBaseFilename=Myna-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"
Name: "addtopath"; Description: "Add &myna to PATH (use it from any terminal)"; GroupDescription: "Command line:"

[Files]
Source: "{#SourceDir}\bin\myna.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#SourceDir}\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs
Source: "{#SourceDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Myna"; Filename: "{app}\bin\myna.exe"; WorkingDir: "{userdocs}"; Comment: "Myna - Developer Intent Engine"
Name: "{group}\Uninstall Myna"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Myna"; Filename: "{app}\bin\myna.exe"; WorkingDir: "{userdocs}"; Comment: "Myna - Developer Intent Engine"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\myna.exe"; Description: "Start Myna now"; WorkingDir: "{userdocs}"; Flags: postinstall nowait skipifsilent

[Code]
const
  UserEnvKey = 'Environment';
  SystemEnvKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

function BinDir: string;
begin
  Result := ExpandConstant('{app}\bin');
end;

{ The PATH of the user, or of the machine for an all-users install. }
function PathRoot: Integer;
begin
  if IsAdminInstallMode then
    Result := HKEY_LOCAL_MACHINE
  else
    Result := HKEY_CURRENT_USER;
end;

function PathKey: string;
begin
  if IsAdminInstallMode then
    Result := SystemEnvKey
  else
    Result := UserEnvKey;
end;

function PathContains(Paths, Dir: string): Boolean;
begin
  Result := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';') > 0;
end;

procedure AddToPath;
var
  Paths: string;
begin
  if not RegQueryStringValue(PathRoot, PathKey, 'Path', Paths) then
    Paths := '';
  if PathContains(Paths, BinDir) then
    exit;
  if (Paths <> '') and (Copy(Paths, Length(Paths), 1) <> ';') then
    Paths := Paths + ';';
  RegWriteExpandStringValue(PathRoot, PathKey, 'Path', Paths + BinDir);
end;

procedure RemoveFromPath;
var
  Paths: string;
  P: Integer;
begin
  if not RegQueryStringValue(PathRoot, PathKey, 'Path', Paths) then
    exit;
  Paths := ';' + Paths + ';';
  P := Pos(';' + Uppercase(BinDir) + ';', Uppercase(Paths));
  if P = 0 then
    exit;
  Delete(Paths, P, Length(BinDir) + 1);
  Paths := Copy(Paths, 2, Length(Paths) - 2);
  RegWriteExpandStringValue(PathRoot, PathKey, 'Path', Paths);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addtopath') then
    AddToPath;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemoveFromPath;
end;
