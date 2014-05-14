; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "SleepyHead"
#define MyAppVersion "0.9.6"
#define MyAppPublisher "Jedimark"
#define MyAppURL "http://sleepyhead.sourceforge.net"
#define MyAppExeName "SleepyHead.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{AF23DDE0-E745-456E-AA06-014BB6DEB63F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile=C:\Users\Mark\Documents\src\COPYING.txt
OutputDir=C:\Users\Mark\Documents\src\sleepyhead-code\innosetup
OutputBaseFilename=SleepyHead-{#MyAppVersion}-OpenGL-Setup-32bit
SetupIconFile=C:\Users\Mark\Documents\src\sleepyhead-code\sleepyhead\icons\bob-v3.0.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "C:\Users\Mark\Desktop\SleepyHead-0.9.6-testing-Qt5.2.1-MingW-OpenGL-32bit\SleepyHead.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Mark\Desktop\SleepyHead-0.9.6-testing-Qt5.2.1-MingW-OpenGL-32bit\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\Mark\Desktop\SleepyHead-0.9.6-testing-Qt5.2.1-MingW-OpenGL-32bit\Translations\*.qm"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

