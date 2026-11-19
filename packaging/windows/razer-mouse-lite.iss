#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef SourceDirectory
  #error SourceDirectory is required
#endif
#ifndef OutputDirectory
  #error OutputDirectory is required
#endif
#ifndef InstallerAppId
  #define InstallerAppId "RazerMouseLite.Independent"
#endif
#ifndef InstallerName
  #define InstallerName "Razer Mouse Lite"
#endif
#ifndef StartupKey
  #define StartupKey "Software\Microsoft\Windows\CurrentVersion\Run"
#endif

[Setup]
AppId={#InstallerAppId}
AppName={#InstallerName}
AppVersion={#AppVersion}
AppPublisher=Razer Mouse Lite contributors
DefaultDirName={autopf}\Razer Mouse Lite
DefaultGroupName={#InstallerName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDirectory}
OutputBaseFilename=razer-mouse-lite-{#AppVersion}-windows-x64-setup
SetupIconFile=..\..\assets\razer-synapse.ico
UninstallDisplayIcon={app}\razer-mouse.exe
UninstallDisplayName={#InstallerName}
LicenseFile=..\..\LICENSE
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "swedish"; MessagesFile: "compiler:Languages\Swedish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDirectory}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#InstallerName}"; Filename: "{app}\razer-mouse.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\{#InstallerName}"; Filename: "{app}\razer-mouse.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\razer-mouse.exe"; Description: "{cm:LaunchProgram,{#InstallerName}}"; Flags: nowait postinstall skipifsilent

[Code]
// The app owns the startup checkbox. Remove only an entry pointing to this copy.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Current, Expected: String;
begin
  if CurUninstallStep = usUninstall then begin
    Expected := '"' + ExpandConstant('{app}\razer-mouse.exe') + '" --tray';
    if RegQueryStringValue(HKCU, '{#StartupKey}', 'RazerMouseLite', Current) then
      if CompareText(Current, Expected) = 0 then
        RegDeleteValue(HKCU, '{#StartupKey}', 'RazerMouseLite');
  end;
end;
