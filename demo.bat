@echo off
rem Plays the ~30-second demo (commands are typed automatically) for recording gameplay video.
rem Start screen recording (Win+Alt+R in Xbox Game Bar), then run this file.
setlocal
if not exist "%~dp0build\lamplighter.exe" (
    call "%~dp0build.bat" || exit /b 1
)
"%~dp0build\lamplighter.exe" --demo "%~dp0demo\demo.txt" %*
