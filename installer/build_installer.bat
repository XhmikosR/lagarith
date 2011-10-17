@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
CD /D %~dp0

REM You can set here the Inno Setup path if for example you have Inno Setup Unicode
REM installed and you want to use the ANSI Inno Setup which is in another location
SET "InnoSetupPath=H:\progs\thirdparty\isetup-5.4.2"

REM Detect if we are running on 64bit WIN and use Wow6432Node, set the path
REM of Inno Setup accordingly and compile the installer
IF "%PROGRAMFILES(x86)%zzz"=="zzz" (
  SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
) ELSE (
  SET "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
)

IF NOT DEFINED InnoSetupPath (
  FOR /F "delims=" %%a IN (
    'REG QUERY "%U_%\Inno Setup 5_is1" /v "Inno Setup: App Path"2^>Nul^|FIND "REG_"') DO (
    SET "InnoSetupPath=%%a" & CALL :SubInnoSetupPath %%InnoSetupPath:*Z=%%)
)

IF NOT EXIST %InnoSetupPath% (
  ECHO. & ECHO.
  ECHO Inno Setup not found!
  GOTO END
)

"%InnoSetupPath%\iscc.exe" /Q "Lagarith_setup.iss"
IF %ERRORLEVEL% NEQ 0 (ECHO Build failed & GOTO END) ELSE (ECHO Installer compiled successfully!)


 :END
ECHO. & ECHO.
ENDLOCAL
PAUSE
EXIT /B


:SubInnoSetupPath
SET InnoSetupPath=%*
EXIT /B
