@echo off
REM --- SET EXECUTION CONTEXT TO SCRIPT DIRECTORY ---
REM Change drive and directory to the script's location
CD /D "%~dp0"
pushd "%~dp0"

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
if exist "CMakeListsList.txt" (
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

:: Define the exclusion pattern for build, .git, and .gitignore folders/files.
:: Ensure we exclude the temporary file itself!
set "TEMP_DIR_LISTING=temp_dir_list.txt"
:: Note: Putting %TEMP_DIR_LISTING% in the pattern below prevents errors when processing the list.
set "EXCLUDE_PATTERN=build ^\.git$ ^\.gitignore$ %TEMP_DIR_LISTING%$"

:: --- NEW STABLE DIRECTORY LISTING LOGIC ---

:: 1. List ALL root-level files and directories (AII, including system/hidden) into the temporary file.
dir /b /a > "%TEMP_DIR_LISTING%"

:: 2. Filter the listing - FOLDERS FIRST (A-Z)
for /f "delims=" %%I in ('cmd /c type "%TEMP_DIR_LISTING%" ^| sort ^| findstr /v /i /r "%EXCLUDE_PATTERN%"') do (
    :: Check if it's a directory
    if exist "%%I\" (
        REM --- Directory
        echo ├── %%I >> "%MERGED%"
        :: List contents of the subdirectory (files only)
        :: Use sort and filtering for internal alphabetical order
        for /f "delims=" %%J in ('dir /b /a:-s-d "%%I" 2^>nul ^| sort ^| findstr /v /i /r "%EXCLUDE_PATTERN%"') do (
            echo │   ├── %%J >> "%MERGED%"
        )
    )
)

:: 3. Filter the listing - FILES SECOND (A-Z)
for /f "delims=" %%I in ('cmd /c type "%TEMP_DIR_LISTING%" ^| sort ^| findstr /v /i /r "%EXCLUDE_PATTERN%"') do (
    :: Check if it's a file
    if not exist "%%I\" (
        REM --- File
        echo ├── %%I >> "%MERGED%"
    )
)


:: 4. Cleanup temporary file
del "%TEMP_DIR_LISTING%" 2>nul

echo [OK] All files copied to "%DEST%" with .txt extension.
echo [OK] Combined file created: "%MERGED%"

REM --- RESTORE ORIGINAL DIRECTORY ---
popd

pause
