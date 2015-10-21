@echo off

if not defined compiler set compiler=2015
echo compiler is: %compiler%

if "%compiler%"=="2015" (
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %platform%
    goto :eof
)

if "%compiler%"=="2013" (
    echo "Set 2013"
    call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" %platform%
    goto :eof
)

if "%compiler%"=="2012" (
    echo "Set 2012"
    call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" %platform%
    goto :eof
)

if "%compiler%"=="2010" (
    echo "Set 2010"
    call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" %platform%
    goto :eof
)

if "%compiler%"=="2008" (
    echo "Set 2008"
    call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" %platform%
    goto :eof
)

if "%compiler%"=="MinGW"  ( 
    echo "Set MinGW"
    set PATH=c:\mingw\bin;%PATH%
    goto :eof
)

:eof
