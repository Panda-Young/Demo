@REM *************************************************************************
@REM Description: copy all target files to Plugin folder
@REM version: 0.1.0
@REM Author: Panda-Young
@REM Date: 2024-03-27 11:37:11
@REM Copyright (c) 2024 by Panda-Young, All Rights Reserved.
@REM *************************************************************************
@echo off

@REM run as administrator
(PUSHD "%~DP0")&(REG QUERY "HKU\S-1-5-19">NUL 2>&1)||(powershell -Command "Start-Process '%~sdpnx0' -Verb RunAs")&&goto :eof

@REM enable delayed expansion after run as administrator
setlocal enabledelayedexpansion

@REM Configuration options
set copy_vst2=1
set copy_vst3=1

set VS_version=VisualStudio2022

set build_type=Debug

if "%build_type%" NEQ "Debug" (
    set build_type=Release
)

set APP_INSTALL_DIR=D:\Program Files

set host_program_path="%APP_INSTALL_DIR%\Adobe\Adobe Audition 2020\Adobe Audition.exe"
@REM set host_program_path="%APP_INSTALL_DIR%\Audacity\Audacity.exe"
@REM set host_program_path="%APP_INSTALL_DIR%\REAPER (x64)\reaper.exe"
@REM set host_program_path="%APP_INSTALL_DIR%\AudioMulch 2.2.4\mulch2.exe"

for %%f in (%host_program_path%) do (
    set host_program_name="%%~nxf"
    set host_program_root=%%~dpf
)

@REM Kill the program
taskkill /F /IM %host_program_name% >nul 2>&1
timeout /t 1 /nobreak >nul 2>&1

for %%f in (Builds\%VS_version%\*.sln) do (
    set plugin_name=%%~nf
)

if %host_program_name%=="Adobe Audition.exe" (
    set VST2_plugin_target_path="C:\Program Files\Common Files\VST\%plugin_name%.dll"
    set architecture=x64
) else if %host_program_name%=="reaper.exe" (
    set VST2_plugin_target_path="C:\Program Files\Common Files\VST\%plugin_name%.dll"
    set architecture=x64
) else if %host_program_name%=="Audacity.exe" (
    set VST2_plugin_target_path="C:\Program Files\Common Files\VST2\%plugin_name%.dll"
    set architecture=x64
) else if %host_program_name%=="mulch2.exe" (
    set VST2_plugin_target_path="%host_program_root%\VSTPlugins\%plugin_name%.dll"
    set architecture=Win32
    set copy_vst3=0
)
set VST2_plugin_product_path="Builds\%VS_version%\%architecture%\%build_type%\VST\%plugin_name%.dll"

set VST3_plugin_product_path="Builds\%VS_version%\%architecture%\%build_type%\VST3\%plugin_name%.vst3\Contents\x86_64-win\%plugin_name%.vst3"
set VST3_plugin_target_path="C:\Program Files\Common Files\VST3\%plugin_name%.vst3"

if %copy_vst2%==1 (
    copy /Y !VST2_plugin_product_path! !VST2_plugin_target_path!
    if errorlevel 1 (
        call :error_exit "Copy failed"
    )

    fc !VST2_plugin_product_path! !VST2_plugin_target_path!
    if errorlevel 1 (
        call :error_exit "files are different"
    )
)

if %copy_vst3%==1 (
    copy /Y !VST3_plugin_product_path! !VST3_plugin_target_path!
    if errorlevel 1 (
        call :error_exit "Copy failed"
    )

    fc !VST3_plugin_product_path! !VST3_plugin_target_path!
    if errorlevel 1 (
        call :error_exit "files are different"
    )
)

type nul > %Temp%\%plugin_name%_VST_Plugin.log

@REM Start the program
start /B "start host program" %host_program_path%
timeout /t 2 /NOBREAK >nul

:error_exit
if not "%~1"=="" (
    color 04
    echo %~1 With error code %errorlevel%
    pause
    color 07
    exit
)

exit /b 0
