@echo off
.\build\Release\AIOneCLI.exe %* & call :ignoreCtrlC
goto :eof

:ignoreCtrlC
exit /b