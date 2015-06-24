cmake_minimum_required(VERSION 2.6)

project(ALIVE)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}")
#set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/3rdParty/cotire/CMake")
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/3rdParty/solar-cmake")
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")


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
	${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files/generated_headers
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
    include/oddlib/masher.hpp
    src/oddlib/stream.cpp
    src/oddlib/anim.cpp
    src/oddlib/lvlarchive.cpp
    src/oddlib/masher.cpp
)
#cotire(oddlib)

# Bake binary test files in C array/header files
function(make_header src dst varname)
message(STATUS "Generating header '${dst}' from " '${src}')
  EXEC_PROGRAM(${CMAKE_COMMAND}
    ARGS -DBINARYBAKER_INPUT_FILE='${src}' -DBINARYBAKER_OUTPUT_FILE='${dst}' -DBINARYBAKER_VARIABLE_NAME='${varname}' -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BinaryBaker.cmakescript)
endfunction()
  
# Helper to take file foo.dat in any path and place it in generated headers as foo.dat.g.h with
# var name of foo
function(to_vec src_file)
    SET(generated_headers_dir ${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files/generated_headers)
    get_filename_component(NameOnly ${samples_dir}/${src_file} NAME)
    get_filename_component(NameOnlyNoExt ${samples_dir}/${src_file} NAME_WE)
    #message(STATUS "File name is ${NameOnly} with out ext is ${NameOnlyNoExt}")
    make_header(${src_file}
                ${generated_headers_dir}/${NameOnly}.g.h 
                ${NameOnlyNoExt})
endfunction()

SET(samples_dir ${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files)

to_vec(${samples_dir}/sample.lvl)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_8_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_8_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_16_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_16_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_8_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_8_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_16_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_16_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/all_colours_low_compression_15fps_8bit_mono_high_compression_5_frames_interleave.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_high_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_low_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_max_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_medium_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_min_compression_30_fps.ddv)


EXEC_PROGRAM(${CMAKE_COMMAND}
    ARGS -DBINARYBAKER_INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files/sample.lvl -DBINARYBAKER_OUTPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files/generated_headers/sample.lvl.g.h -DBINARYBAKER_VARIABLE_NAME=sample_lvl -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BinaryBaker.cmakescript)

add_executable(Tests 
    test/sample_files/generated_headers/sample.lvl.g.h
    test/main.cpp
)
#cotire(Tests)
target_link_libraries(Tests oddlib gmock ${SDL2_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME Tests COMMAND Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Generate header containing window title/version information
git_shorttag(GIT_SHORT_REV)
set(ALIVE_VERSION "ALIVE v${ALIVE_VERSION_MAJOR}.${ALIVE_VERSION_MINOR}.${ALIVE_VERSION_PATCH} rev(${GIT_SHORT_REV})")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/packaging/alive_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/packaging/alive_version.h)

add_executable(Alive MACOSX_BUNDLE
    src/main.cpp
)


#cotire(Alive)
TARGET_LINK_LIBRARIES(Alive jsonxx imgui oddlib ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} liblua)
install(
    TARGETS Alive 
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .)



if (WIN32 AND MSVC)
   add_custom_command(TARGET Alive POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_INCLUDE_DIR}/../lib/${arch}/SDL2.dll" "$(OutDir)SDL2.dll")
   add_custom_command(TARGET Alive POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/3rdParty/luajit/$(Configuration)/lua.dll" "$(OutDir)lua.dll")
   INSTALL(FILES "${SDL2_INCLUDE_DIR}/../lib/${arch}/SDL2.dll" DESTINATION ".")
endif()

if (APPLE)
    # Fix link path to look in the bundle
    add_custom_command(TARGET Alive POST_BUILD
                   COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change /usr/local/lib/libSDL2-2.0.0.dylib @executable_path/libSDL2-2.0.0.dylib $<TARGET_FILE:Alive>
                   COMMENT "Fixing up libSDL2-2.0.0.dylib rpath")

   
    INSTALL(FILES "/usr/local/Cellar/sdl2/2.0.3/lib/libSDL2-2.0.0.dylib" DESTINATION ".")
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
