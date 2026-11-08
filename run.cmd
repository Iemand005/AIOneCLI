@echo off
.\build\Debug\AIOneCLI.exe %* & call :ignoreCtrlC
goto :eof

:ignoreCtrlC
exit /b