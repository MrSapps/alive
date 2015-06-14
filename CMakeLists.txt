cmake_minimum_required(VERSION 2.6)

project(ALIVE)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}")
#set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/3rdParty/cotire/CMake")
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/3rdParty/solar-cmake")

#include(cotire)
include(Solar)

# the RPATH to be used when installing
SET(CMAKE_INSTALL_RPATH "")

target_architecture(arch)
message("arch is ${arch}")

# i386 for 32bit and x86_64 for 64bit
if ("${arch}" STREQUAL "i386")
    set (arch "x86")
elseif("${arch}" STREQUAL "x86_64")
    set (arch "x64")
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

enable_testing()

#set( CMAKE_VERBOSE_MAKEFILE on )

# The version number.
set(ALIVE_VERSION_MAJOR 0)
set(ALIVE_VERSION_MINOR 0)
set(ALIVE_VERSION_PATCH 0)

# Enable Visual Studio solution "folders"
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

# On Linux gtest needs pthreads
if (UNIX)
    find_package (Threads)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
  #message("RELEASE set")
endif ()

include_directories(
    include
    ${SDL2_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/packaging
    ${OPENGL_INCLUDE_DIRS}
   
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest
    ${CMAKE_BINARY_DIR}/3rdParty/luajit
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/luajit/src
)

SET(imgui_src 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/stb_rect_pack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/stb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/stb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/imgui.cpp
)

add_library(imgui STATIC ${imgui_src})
SET_PROPERTY(TARGET imgui PROPERTY FOLDER "3rdparty")

SET(gmock_src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/src/gmock-all.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest/src/gtest-all.cc
)

add_library(gmock STATIC ${gmock_src})
SET_PROPERTY(TARGET gmock PROPERTY FOLDER "3rdparty")

SET(jsonxx_src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/jsonxx/jsonxx.cc
)
add_library(jsonxx STATIC ${jsonxx_src})
SET_PROPERTY(TARGET jsonxx PROPERTY FOLDER "3rdparty")

add_subdirectory(3rdParty/luajit)
SET_PROPERTY(TARGET buildvm PROPERTY FOLDER "3rdparty")
SET_PROPERTY(TARGET liblua PROPERTY FOLDER "3rdparty")
SET_PROPERTY(TARGET luajit PROPERTY FOLDER "3rdparty")
SET_PROPERTY(TARGET minilua PROPERTY FOLDER "3rdparty")

add_library(oddlib STATIC
    include/logger.hpp
    include/string_util.hpp
    include/oddlib/exceptions.hpp
    include/oddlib/stream.hpp
    include/oddlib/anim.hpp
    include/oddlib/lvlarchive.hpp
    src/oddlib/stream.cpp
    src/oddlib/anim.cpp
    src/oddlib/lvlarchive.cpp
)
#cotire(oddlib)

add_executable(Tests
    test/main.cpp
)
#cotire(Tests)
target_link_libraries(Tests oddlib gmock ${SDL2_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME Tests COMMAND Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Generate header containing window title/version information
git_shorttag(GIT_SHORT_REV)
set(ALIVE_VERSION "ALIVE v${ALIVE_VERSION_MAJOR}.${ALIVE_VERSION_MINOR}.${ALIVE_VERSION_PATCH} rev(${GIT_SHORT_REV})")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/packaging/alive_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/packaging/alive_version.h)

add_executable(Alive
    src/main.cpp
)
#cotire(Alive)
TARGET_LINK_LIBRARIES(Alive jsonxx imgui oddlib ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} liblua)
install(TARGETS Alive RUNTIME DESTINATION .)



if (WIN32 AND MSVC)
   add_custom_command(TARGET Alive POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_INCLUDE_DIR}/../lib/${arch}/SDL2.dll" "$(OutDir)SDL2.dll")
   add_custom_command(TARGET Alive POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/3rdParty/luajit/Debug/lua.dll" "$(OutDir)lua.dll")
   INSTALL(FILES "${SDL2_INCLUDE_DIR}/../lib/${arch}/SDL2.dll" DESTINATION ".")
endif()

if (APPLE)
    add_custom_command(TARGET Alive POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "/Library/Frameworks/SDL2.framework" "$(OutDir)/SDL2.framework")
    
    INSTALL(FILES "/Library/Frameworks/SDL2.framework" DESTINATION ".")
endif()

INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/videos.json" DESTINATION "./data")

if (MSVC)
    # Build cpp files on all cores
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Wall /wd4351 /wd4514 /wd4350 /wd4710 /wd4571 /wd4820 /wd4668 /wd4626 /wd4625 /wd4365 /wd4640 /wd4628 /wd4061 /wd4826 /wd4127 /wd4711 /wd4738")
else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # C++14 by default
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -fPIC -Wall")
    else()
        # Enable C++14, you may need to use -std=c++0x if using an older gcc compiler
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC -Wall -pedantic -Wmissing-field-initializers")
        
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0") # debug, no optimisation
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage") # enabling coverage
        
        # we dont want high warning levels on 3rd party things since we won't be fixing those warnings
        set_source_files_properties(${gmock_src} PROPERTIES COMPILE_FLAGS "-Wno-effc++ -Wno-missing-field-initializers")
    endif()
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#OS SPECIFIC 
if(WIN32)
    # On Windows generate MSI packages
    SET(CPACK_GENERATOR "WIX")
    
    set(CPACK_WIX_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/wix.template.in")
    SET(CPACK_WIX_UPGRADE_GUID 2a54667a-a192-4b78-a34d-b3282dfa7f3f)

    # If empty assume release
    #if ("${CMAKE_BUILD_TYPE}" STREQUAL "" OR "${CMAKE_BUILD_TYPE}" STREQUAL "release")
    #    message("RELEASE installer build")
    #else()
    #    message("DEBUG installer build")
    #endif()
elseif(APPLE)
    # APPLE is also UNIX, so must check for APPLE before UNIX
    SET(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    # On Linux generate TAR.GZ,DEB and RPM packages
    #Find out what architecture are we running on and set the package architecture 
endif()

INCLUDE(CPack)
