@echo off
set NINJA_STATUS=[%%p %%f/%%t]
cmake --build build --config Debug -j %NUMBER_OF_PROCESSORS%