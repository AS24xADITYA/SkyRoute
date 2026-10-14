@echo off
gcc -std=c11 -Wall -Wextra src/*.c -o skyroute.exe
if %ERRORLEVEL% equ 0 (
    echo Compilation successful! Run skyroute.exe to start.
) else (
    echo Compilation failed.
)
