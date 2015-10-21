:: Now we declare a scope
Setlocal EnableDelayedExpansion EnableExtensions

echo %compiler%

if not defined compiler set compiler=2015

if "%compiler%"=="MinGW" ( goto :mingw )

if "%compiler%"=="2015" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
)

if "%compiler%"=="2013" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
)

if "%compiler%"=="2012" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
)

if "%compiler%"=="2010" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
)

if "%compiler%"=="2008" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
)

:: Visual Studio detected
endlocal
echo %SET_VS_ENV%
echo "set vsvars"
call %SET_VS_ENV% %platform%
goto :eof

:: MinGW detected
:mingw
echo "set minGW vars"
endlocal & set PATH=c:\mingw\bin;%PATH%
