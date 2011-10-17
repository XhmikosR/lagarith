#define LAG_VERSION       "1.3.26"
#define LAG_VERSION_SHORT "1326"
#define PUBLISHER         "Ben Greenwood"
#define WEBPAGE           "http://lags.leetcode.net/codec.html"

#if VER < 0x05040200
  #error Update your Inno Setup version
#endif


[Setup]
AppID={{F59AC46C-10C3-4023-882C-4212A92283B3}
AppName=Lagarith Lossless Codec
AppVerName=Lagarith Lossless Codec {#LAG_VERSION}
AppVersion={#LAG_VERSION}
AppContact={#WEBPAGE}
AppCopyright={#PUBLISHER}
AppPublisher={#PUBLISHER}
AppPublisherURL={#WEBPAGE}
AppSupportURL={#WEBPAGE}
AppUpdatesURL={#WEBPAGE}
VersionInfoCompany={#PUBLISHER}
VersionInfoCopyright={#PUBLISHER}
VersionInfoDescription=Lagarith Lossless Codec {#LAG_VERSION} Setup
VersionInfoTextVersion={#LAG_VERSION}
VersionInfoVersion={#LAG_VERSION}
VersionInfoProductName=Lagarith Lossless Codec
VersionInfoProductVersion={#LAG_VERSION}
VersionInfoProductTextVersion={#LAG_VERSION}
DefaultDirName={pf}\Lagarith Lossless Codec
AppReadmeFile={app}\ReadMe.txt
InfoBeforeFile=copying.txt
AlwaysShowComponentsList=false
ShowComponentSizes=false
FlatComponentsList=false
OutputDir=.
OutputBaseFilename=LagarithSetup_{#LAG_VERSION_SHORT}
UninstallDisplayName=Lagarith Lossless Codec ({#LAG_VERSION})
UninstallFilesDir={app}
MinVersion=0,5.1
SolidCompression=yes
Compression=lzma/ultra64
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
ArchitecturesInstallIn64BitMode=x64


[Files]
Source: ..\Release\Win32\lagarith.dll; DestDir: {sys}; Flags: ignoreversion restartreplace 32bit
Source: ..\Release\x64\lagarith.dll;   DestDir: {sys}; Flags: ignoreversion restartreplace 64bit; Check: Is64BitInstallMode()
Source: copying.txt;                   DestDir: {app}; Flags: ignoreversion restartreplace
Source: settings.txt;                  DestDir: {app}; Flags: ignoreversion restartreplace
Source: ReadMe.txt;                    DestDir: {app}; Flags: ignoreversion restartreplace


[Registry]
Root: HKCU;   Subkey: Software\Lagarith;                                             ValueType: string; ValueName: Multithread;  ValueData: 1
Root: HKLM;   Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.LAGS; ValueType: string; ValueName: Description;  ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue
Root: HKLM;   Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.LAGS; ValueType: string; ValueName: Driver;       ValueData: lagarith.dll;                   Flags: uninsdeletevalue
Root: HKLM;   Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.LAGS; ValueType: string; ValueName: FriendlyName; ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue
Root: HKLM;   Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: lagarith.dll; ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue
Root: HKLM;   Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.LAGS;    ValueData: lagarith.dll;                   Flags: uninsdeletevalue
Root: HKLM64; Subkey: Software\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: lagarith.dll; ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue; Check: Is64BitInstallMode()
Root: HKLM64; Subkey: Software\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.LAGS;    ValueData: lagarith.dll;                   Flags: uninsdeletevalue; Check: Is64BitInstallMode()


[INI]
FileName: {win}\system.ini; Section: drivers32; Key: VIDC.LAGS; String: lagarith.dll; Flags: uninsdeleteentry 


[Run]
Filename: {#WEBPAGE}; Description: Visit Webpage; Flags: nowait postinstall skipifsilent shellexec unchecked


[Code]
var
  is_update: Boolean;


function IsUpdate(): Boolean;
begin
  Result := is_update;
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if IsUpdate then begin
    Case PageID of
      // Hide the license page
      wpInfoBefore: Result := True;
      wpReady: Result := True;
    else
      Result := False;
    end;
  end;
end;


procedure CurPageChanged(CurPageID: Integer);
begin
  if IsUpdate AND (CurPageID = wpWelcome) then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpInfoBefore then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpFinished then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonFinish)
  else if IsUpdate then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
end;


Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling ask user to delete lagarith INI file based on whether this file exists
  if CurUninstallStep = usUninstall then begin
    if RegKeyExists(HKEY_CURRENT_USER, 'Software\Lagarith') then begin
      if MsgBox(ExpandConstant('Do you also want to delete Lagarith settings? If you plan on reinstalling Lagarith you do not have to delete them.'),
       mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then begin
         DeleteFile(ExpandConstant('{win}\lagarith.ini'));
         RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Lagarith');
      end;
    end;
  end;
end;


function InitializeSetup(): Boolean;
begin
  is_update := RegKeyExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{F59AC46C-10C3-4023-882C-4212A92283B3}_is1');
  Result := True;
end;
