@echo off
chcp 65001 >nul
setlocal
set SCRIPT_DIR=%~dp0

call "D:\Program Files (x86)\MSVC\VC\Auxiliary\Build\vcvars64.bat"

if /I "%1"=="debug" (
    echo === MSVC Debug ===
    cl.exe /utf-8 /std:c++14 /EHsc /Zi /Od /I"%SCRIPT_DIR%.." "%SCRIPT_DIR%bokumeido_unit_test.cpp" "%SCRIPT_DIR%another.cpp" /Fe"%SCRIPT_DIR%bokumeido_unit_test_d.exe"
) else (
    echo === MSVC Release ===
    cl.exe /utf-8 /std:c++14 /EHsc /O2 /DNDEBUG /I"%SCRIPT_DIR%.." "%SCRIPT_DIR%bokumeido_unit_test.cpp" "%SCRIPT_DIR%another.cpp" /Fe"%SCRIPT_DIR%bokumeido_unit_test.exe"
)

if %ERRORLEVEL% equ 0 (
    echo.
    echo OK!
) else (
    echo.
    echo FAILED: %ERRORLEVEL%
)

echo 清理临时文件 ...
del /f /q "%SCRIPT_DIR%*.obj" "%SCRIPT_DIR%*.lib" "%SCRIPT_DIR%*.exp" "%SCRIPT_DIR%*.pdb" "%SCRIPT_DIR%*.ilk" "%SCRIPT_DIR%*.idb" 2>nul

endlocal
exit /b %ERRORLEVEL%
