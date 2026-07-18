@echo off

:: 1. Save the target, default to debug if empty
SET "TARGET=%~1"
IF "%TARGET%"=="" SET "TARGET=debug"

:: 2. Check for custom compiler directory
SET "COMPILER_DIR=../../Compiler"
IF NOT "%2" == "" (
    IF NOT "%~2" == "" (
        powershell write-host -fore Red Using custom compiler path
        SET "COMPILER_DIR=%~2"
    )
    :: Since a second argument was passed (even if empty ""), shift twice
    SHIFT
    SHIFT
) ELSE (
    :: Only one argument was passed, shift once
    SHIFT
)

:: Rebuild remaining shifted arguments since %* is not affected by SHIFT
:: Skip any empty string placeholders ("") in MAKE_ARGS
SET "MAKE_ARGS="
:argloop
IF "%1"=="" GOTO endargloop
IF NOT "%~1"=="" (
    SET "MAKE_ARGS=%MAKE_ARGS% %1"
)
SHIFT
GOTO argloop
:endargloop

:: 3. Environment Setup
SET "UTIL_DIR=%COMPILER_DIR%\Other Utilities"
SET "MSYS_DIR=%COMPILER_DIR%\msys2\usr\bin"
SET "BIN_DIR=%COMPILER_DIR%\sh2eb-elf\bin"

:: Safely append each directory only if it doesn't already exist in PATH
echo "%PATH%" | findstr /I /C:";%UTIL_DIR%;" /C:";%UTIL_DIR%\" >nul || SET "PATH=%UTIL_DIR%;%PATH%"
echo "%PATH%" | findstr /I /C:";%MSYS_DIR%;" /C:";%MSYS_DIR%\" >nul || SET "PATH=%MSYS_DIR%;%PATH%"
echo "%PATH%" | findstr /I /C:";%BIN_DIR%;" /C:";%BIN_DIR%\"   >nul || SET "PATH=%BIN_DIR%;%PATH%"

:: 4. Execute Build Targets
IF "%TARGET%" == "debug" GOTO debug
IF "%TARGET%" == "release" GOTO release
IF "%TARGET%" == "clean" GOTO clean
echo Unknown target: %TARGET%
exit /b 1

:debug
powershell write-host -back Yellow -fore Black Building debug...
make all DEBUG=1 %MAKE_ARGS%
GOTO end

:release
powershell write-host -back Green -fore White Building release...
make all %MAKE_ARGS%
GOTO end

:clean
powershell write-host -back Green -fore White Cleaning...
make clean %MAKE_ARGS%
GOTO end

:end
