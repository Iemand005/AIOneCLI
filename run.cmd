@echo off
cd build/Debug/
AIOneCLI.exe %* & call :ignoreCtrlC
cd ../..
goto :eof

:ignoreCtrlC
exit /b