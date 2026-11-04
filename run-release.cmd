@echo off
cd build/Release/
AIOneCLI.exe %* & call :ignoreCtrlC
cd ../..
goto :eof

:ignoreCtrlC
exit /b