set SDL2DIR=C:\SDL2-2.0.3
mkdir build
cd build
:: Call me with "Visual Studio 12 2013" or "Visual Studio 12 2013 Win64"
cmake .. -G%1
