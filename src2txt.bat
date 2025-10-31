@echo off
set "SRC=src"
set "DEST=src_text"
set "MERGED=source_files.txt"

if not exist "%DEST%" mkdir "%DEST%"
> "%MERGED%" echo.

:: Add header with date/time and AI context
echo ============================================== >> "%MERGED%"
echo PROJECT SNAPSHOT FOR AI CONTEXT >> "%MERGED%"
echo Generated on: %date% %time% >> "%MERGED%"
echo This file contains: >> "%MERGED%"
echo  - Complete source code from src/ >> "%MERGED%"
echo  - Key project files (README.md, CMakeLists.txt, .qrc) >> "%MERGED%"
echo  - Directory structure in tree format >> "%MERGED%"
echo Purpose: Provide full context for AI analysis or review. >> "%MERGED%"
echo ============================================== >> "%MERGED%"
echo. >> "%MERGED%"

:: Add README.md
if exist "README.md" (
    echo ============================================== >> "%MERGED%"
    echo FILE: README.md >> "%MERGED%"
    echo ============================================== >> "%MERGED%"
    type "README.md" >> "%MERGED%"
    echo. >> "%MERGED%"
)

:: Add CMakeLists.txt
if exist "CMakeLists.txt" (
    echo ============================================== >> "%MERGED%"
    echo FILE: CMakeLists.txt >> "%MERGED%"
    echo ============================================== >> "%MERGED%"
    type "CMakeLists.txt" >> "%MERGED%"
    echo. >> "%MERGED%"
)

:: Add PipMatrixResolverQt.qrc
if exist "PipMatrixResolverQt.qrc" (
    copy "PipMatrixResolverQt.qrc" "%DEST%\PipMatrixResolverQt.qrc.txt" >nul
    echo ============================================== >> "%MERGED%"
    echo FILE: PipMatrixResolverQt.qrc >> "%MERGED%"
    echo ============================================== >> "%MERGED%"
    type "PipMatrixResolverQt.qrc" >> "%MERGED%"
    echo. >> "%MERGED%"
)

:: Loop through src files
for %%F in ("%SRC%\*") do (
    set "filename=%%~nxF"
    setlocal enabledelayedexpansion
    copy "%%F" "%DEST%\!filename!.txt" >nul
    echo ============================================== >> "%MERGED%"
    echo FILE: !filename! >> "%MERGED%"
    echo ============================================== >> "%MERGED%"
    type "%%F" >> "%MERGED%"
    echo. >> "%MERGED%"
    endlocal
)

:: Add directory tree listing
echo ============================================== >> "%MERGED%"
echo DIRECTORY LISTING >> "%MERGED%"
echo ============================================== >> "%MERGED%"
echo . >> "%MERGED%"
for /f "delims=" %%F in ('dir /b /a:-d ^| findstr /v "build"') do (
    echo ├── %%F >> "%MERGED%"
)
for /f "delims=" %%D in ('dir /b /a:d ^| findstr /v "build"') do (
    echo ├── %%D >> "%MERGED%"
    for /f "delims=" %%F in ('dir /b "%%D" ^| findstr /v "build"') do (
        echo │   ├── %%F >> "%MERGED%"
    )
)

echo [OK] All files copied to "%DEST%" with .txt extension.
echo [OK] Combined file created: "%MERGED%"
pause