@echo off
rem Starts the game (builds it first if needed). Extra arguments are passed to the game.
setlocal
if not exist "%~dp0build\lamplighter.exe" (
    call "%~dp0build.bat" || exit /b 1
)
"%~dp0build\lamplighter.exe" %*
