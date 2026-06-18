; Inno Setup 6 script for Learn to Play PoE1
; Prerequisites: Inno Setup 6 (https://jrsoftware.org/isinfo.php)
; Build:  ISCC installer\windows.iss   (or: just installer)
; Output: installer\dist\l2p-poe1-0.1.0-setup.exe

#define AppName      "Learn to Play PoE1"
#define AppVersion   "0.1.0"
#define AppPublisher "MovingCairn"
#define AppExeName   "l2p-poe1.exe"
#define DistDir      "..\dist\windows-mingw"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=dist
OutputBaseFilename=l2p-poe1-{#AppVersion}-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"
Name: "startupentry"; Description: "Start with Windows (runs in system tray)"; GroupDescription: "Startup:"; Flags: unchecked

[Files]
Source: "{#DistDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}";           Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";     Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; \
    ValueType: string; ValueName: "{#AppName}"; \
    ValueData: """{app}\{#AppExeName}"""; \
    Flags: uninsdeletevalue; Tasks: startupentry
