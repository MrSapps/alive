cmake_minimum_required(VERSION 2.6)

project(ALIVE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
find_package(SDL2 REQUIRED)

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
  message("RELEASE set")
endif ()

include_directories(
    include
    ${SDL2_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest
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

SET(gmock_src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/src/gmock-all.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest/src/gtest-all.cc
)

add_library(gmock STATIC ${gmock_src})

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

add_executable(Tests
    test/main.cpp
)
target_link_libraries(Tests oddlib gmock ${SDL2_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME Tests COMMAND Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Alive
    src/main.cpp

)
TARGET_LINK_LIBRARIES(Alive imgui oddlib ${SDL2_LIBRARY})

if (MSVC)
    # Build cpp files on all cores
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Wall /wd4351 /wd4514 /wd4350 /wd4710 /wd4571 /wd4820 /wd4668 /wd4626 /wd4625 /wd4365 /wd4640 /wd4628 /wd4061 /wd4826")
else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # C++14 by default
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -fPIC")
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
    SET(CPACK_WIX_UPGRADE_GUID 07c96c63-d705-4d6b-9cd1-869ec27e325b)

    # If empty assume release
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "" OR "${CMAKE_BUILD_TYPE}" STREQUAL "release")
        message("RELEASE installer build")
    else()
        message("DEBUG installer build")
    endif()
elseif(APPLE)
    # APPLE is also UNIX, so must check for APPLE before UNIX
    SET(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    # On Linux generate TAR.GZ,DEB and RPM packages
    #Find out what architecture are we running on and set the package architecture 
endif()

INCLUDE(CPack)
