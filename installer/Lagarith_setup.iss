;#define ICL12

#define LAG_VERSION       "1.3.26"
#define LAG_VERSION_SHORT "1326"
#define PUBLISHER         "Ben Greenwood"
#define WEBPAGE           "http://lags.leetcode.net/codec.html"

#if VER < 0x05040200
  #error Update your Inno Setup version
#endif


#ifdef ICL12
  #define bindir "..\src\ICL12"
#else
  #define bindir "..\src\VS2010"
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
InfoBeforeFile=copying.txt
OutputDir=.
#ifdef ICL12
OutputBaseFilename=LagarithSetup_{#LAG_VERSION_SHORT}_ICL12
#else
OutputBaseFilename=LagarithSetup_{#LAG_VERSION_SHORT}
#endif
UninstallDisplayName=Lagarith Lossless Codec [{#LAG_VERSION}]
MinVersion=0,5.1
SolidCompression=yes
Compression=lzma/ultra64
InternalCompressLevel=max
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
AllowCancelDuringInstall=no
ArchitecturesInstallIn64BitMode=x64


[Files]
Source: {#bindir}\Release\Win32\lagarith.dll; DestDir: {sys}; Flags: sharedfile ignoreversion uninsnosharedfileprompt restartreplace 32bit
Source: {#bindir}\Release\x64\lagarith.dll;   DestDir: {sys}; Flags: sharedfile ignoreversion uninsnosharedfileprompt restartreplace 64bit; Check: Is64BitInstallMode()
Source: copying.txt;                          DestDir: {app}; Flags: ignoreversion
Source: settings.txt;                         DestDir: {app}; Flags: ignoreversion


[Registry]
Root: HKCU;   Subkey: Software\Lagarith;                                             ValueType: string; ValueName: Multithread;  ValueData: 1
Root: HKLM;   Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.LAGS; ValueType: string; ValueName: Description;  ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue
Root: HKLM;   Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.LAGS; ValueType: string; ValueName: Driver;       ValueData: lagarith.dll;                   Flags: uninsdeletevalue
Root: HKLM;   Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.LAGS; ValueType: string; ValueName: FriendlyName; ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue
Root: HKLM;   Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: lagarith.dll; ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue
Root: HKLM;   Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.LAGS;    ValueData: lagarith.dll;                   Flags: uninsdeletevalue
Root: HKLM64; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: lagarith.dll; ValueData: Lagarith lossless codec [LAGS]; Flags: uninsdeletevalue; Check: Is64BitInstallMode()
Root: HKLM64; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.LAGS;    ValueData: lagarith.dll;                   Flags: uninsdeletevalue; Check: Is64BitInstallMode()


[INI]
FileName: {win}\system.ini; Section: drivers32; Key: VIDC.LAGS; String: lagarith.dll; Flags: uninsdeleteentry 


[Run]
Filename: {#WEBPAGE}; Description: Visit Webpage; Flags: nowait postinstall skipifsilent shellexec unchecked


[InstallDelete]
Type: files; Name: {app}\ReadMe.txt


[Code]
function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if IsUpgrade() then begin
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
  if IsUpgrade() and (CurPageID = wpWelcome) then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpInfoBefore then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpFinished then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonFinish)
  else if IsUpgrade() then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling ask user to delete Lagarith settings
  if CurUninstallStep = usUninstall then begin
    if RegKeyExists(HKEY_CURRENT_USER, 'Software\Lagarith') then begin
      if SuppressibleMsgBox('Do you also want to delete Lagarith settings? If you plan on reinstalling Lagarith you do not have to delete them.',
        mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then begin
         RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Lagarith');
      end;
    end;
    // Always delete the old settings
    DeleteFile(ExpandConstant('{win}\lagarith.ini'));
  end;
end;

