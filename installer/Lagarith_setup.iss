;#define ICL12

#define LAG_VERSION       "1.3.27"
#define LAG_VERSION_SHORT "1327"
#define PUBLISHER         "Ben Greenwood"
#define WEBPAGE           "http://lags.leetcode.net/codec.html"

#if VER < EncodeVer(5,5,1)
  #error Update your Inno Setup version (5.5.1 or newer)
#endif


#ifdef ICL12
  #define bindir "..\src\ICL12"
  #define sse2_required
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
MinVersion=5.1
SolidCompression=yes
Compression=lzma/ultra64
InternalCompressLevel=max
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
AllowCancelDuringInstall=no
ArchitecturesInstallIn64BitMode=x64


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
BeveledLabel     =Lagarith {#LAG_VERSION}
SetupAppTitle    =Setup - Lagarith
SetupWindowTitle =Setup - Lagarith


[CustomMessages]
en.msg_DeleteSettings        =Do you also want to delete Lagarith's settings?%n%nIf you plan on installing Lagarith again then you do not have to delete them.
en.msg_SetupIsRunningWarning =Lagarith setup is already running!
#if defined(sse_required)
en.msg_simd_sse              =This build of Lagarith requires a CPU with SSE extension support.%n%nYour CPU does not have those capabilities.
#elif defined(sse2_required)
en.msg_simd_sse2             =This build of Lagarith requires a CPU with SSE2 extension support.%n%nYour CPU does not have those capabilities.
#endif


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
// Global variables/constants and general functions
const installer_mutex_name = 'lagarith' + '_setup_mutex';

#if defined(sse_required) || defined(sse2_required)
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';
#endif


function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


#if defined(sse_required)
function Is_SSE_Supported(): Boolean;
begin
  // PF_XMMI_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(6);
end;

#elif defined(sse2_required)

function Is_SSE2_Supported(): Boolean;
begin
  // PF_XMMI64_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(10);
end;

#endif


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


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running then show a message and stop installation
  if CheckForMutexes(installer_mutex_name) and not WizardSilent() then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex_name);

#if defined(sse2_required)
    if not Is_SSE2_Supported() then begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse2'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#elif defined(sse_required)
    if not Is_SSE_Supported() then begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#endif

  end;
end;


function InitializeUninstall(): Boolean;
begin
  if CheckForMutexes(installer_mutex_name) then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex_name);
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
      if SuppressibleMsgBox(CustomMessage('msg_DeleteSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then begin
         RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Lagarith');
      end;
    end;
    // Always delete the old settings
    DeleteFile(ExpandConstant('{win}\lagarith.ini'));
  end;
end;
