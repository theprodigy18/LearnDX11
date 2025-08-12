@echo off
setlocal enabledelayedexpansion

set VS_TARGET=vs_5_0
set PS_TARGET=ps_5_0

set SHADER_PATH=assets\shaders

for %%F in (%SHADER_PATH%\*.hlsl) do (
    set FILE_NAME=%%~nF

    echo.
    echo Compiling: !FILE_NAME!.hlsl
    
    fxc.exe /T %VS_TARGET% /E VSMain /Fo "%SHADER_PATH%\!FILE_NAME!_vs.cso" "%%F"
    fxc.exe /T %PS_TARGET% /E PSMain /Fo "%SHADER_PATH%\!FILE_NAME!_ps.cso" "%%F"
)


endlocal