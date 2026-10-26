[Setup]
AppName=TARAREO
AppVersion=0.3.0
AppPublisher=ORYMY And YMY Records
DefaultDirName={autopf}\TARAREO
DefaultGroupName=TARAREO
OutputDir=Output
OutputBaseFilename=TARAREO-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes

[Files]
; Plugin VST3 — FL Studio escanea la carpeta estándar.
Source: "..\build\Tarareo_artefacts\Release\VST3\TARAREO.vst3\*"; DestDir: "{commoncf64}\VST3\TARAREO.vst3"; Flags: recursesubdirs ignoreversion
Source: "..\build\Tarareo_artefacts\Release\Standalone\TARAREO.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\TARAREO"; Filename: "{app}\TARAREO.exe"

[Run]
Filename: "{app}\TARAREO.exe"; Description: "Abrir TARAREO"; Flags: nowait postinstall skipifsilent unchecked
