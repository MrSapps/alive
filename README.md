Build  | Status
------------- | -------------
Windows |[![Build status](https://ci.appveyor.com/api/projects/status/r7k50qbfx8wynbd2?svg=true)](https://ci.appveyor.com/project/paulsapps/alive)
Linux/OSX | [![Build Status](https://travis-ci.org/paulsapps/alive.svg?branch=dev)](https://travis-ci.org/paulsapps/alive)
Coverage  | [![Coverage Status](https://coveralls.io/repos/paulsapps/alive/badge.svg?branch=dev&service=github)](https://coveralls.io/github/paulsapps/alive?branch=dev)
Coverity  | [![Coverity status](https://scan.coverity.com/projects/5367/badge.svg)](https://scan.coverity.com/projects/5367)


Open source implementation of ALIVE engine that powers Oddworld Abes Oddysee and Oddworld Abes Exoddus. 

This engine will be designed to allow easier modding than the orignal engine and for more extensive and complex AI. For example it should be possible to allow Sligs to use lifts and have interaction between Scrabs and Paramites.

You can find the CI build outputs here:
www.mlgthatsme.com/alive_builds

You can become a patreon here (donations to support development):
https://www.patreon.com/alive

Or BitCoin:
1DnshozPxRQg3yeR777MqsVzfL97NQpUB

IRC channel is #oddworld on irc.esper.net


[![Video player](https://raw.githubusercontent.com/paulsapps/alive/dev/doc/screenshots/alive1.png)]

Build instructions.

Supported compilers are clang, gcc and msvc 2013/2015. The clang and gcc version must support C++11. Should be possible to build on Windows, OSX and most *nix OS'es.

1. Install CMake 2.8 or newer.
2. Clone down the repo with with --recursive option so that sub modules are also cloned
3. Create a build directory in the root of the repo
3a. If on Windows set the SDL2DIR enviroment varaible to your extracted pre-built SDL2 dir.
4. Cd into the build dir and run CMake .. to generate project files, pass -G "the generator you want"
5. Now run make/msbuild or whatever the build command is for your generator. Or if using the Visual Studio generator you can just open the .sln solution file and build that way.

