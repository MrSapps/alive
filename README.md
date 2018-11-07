Build  | Status
------------- | -------------
Windows |[![Build status](https://ci.appveyor.com/api/projects/status/r7k50qbfx8wynbd2?svg=true)](https://ci.appveyor.com/project/paulsapps/alive)
Linux/OSX | [![Build Status](https://travis-ci.org/paulsapps/alive.svg?branch=dev)](https://travis-ci.org/paulsapps/alive)
Coverage  | [![Coverage Status](https://coveralls.io/repos/paulsapps/alive/badge.svg?branch=dev&service=github)](https://coveralls.io/github/paulsapps/alive?branch=dev)
Coverity  | [![Coverity status](https://scan.coverity.com/projects/5367/badge.svg)](https://scan.coverity.com/projects/5367)


Open source implementation of ALIVE engine that powers Oddworld: Abe's Oddysee and Oddworld: Abe's Exoddus.

This engine will be designed to allow easier modding than the orignal engine and for more extensive and complex AI. For example it should be possible to allow Sligs to use lifts and have interaction between Scrabs and Paramites.

UPDATE: Focus has shifted to reverse engineering Abe's Exoddus in order to replicate the AI/behaviour exactly, see the new repo here: https://github.com/AliveTeam/alive_reversing

Youtube playlist here:
https://www.youtube.com/playlist?list=PLZysgmpXqKgSCUuU9S2ccQq0Sdrai3UQY

You can become a patreon here (donations to support development):
https://www.patreon.com/alive

[![Alive Screenshot](https://raw.githubusercontent.com/paulsapps/alive/dev/doc/screenshots/alive1.png)]

Build instructions.

Supported compilers are clang, gcc and msvc 2015. The clang and gcc version must support at least C++14. Should be possible to build on Windows, OSX and most *nix OS'es.

1. Install CMake 2.8 or newer.
2. Clone down the repo with with `--recursive` option so that sub modules are also cloned.
3. Create a build directory in the root of the repo.
   - If on Windows, set the SDL2DIR enviroment varaible to the directory that you have extracted the [SDL2 development libraries](https://www.libsdl.org/download-2.0.php) into.
4. `cd` into the build dir and run `CMake ..` to generate project files, pass `-G "the generator you want"` for example `cmake -G "Visual Studio 14 2015 Win64"`.
5. Now run `make`/`msbuild` or whatever the build command is for your generator. Or if using the Visual Studio generator you can just open the .sln solution file and build that way.
