if (APPLE)
# We need a newer version on apple otherwise compling objective-c files fails when mixed with C/C++ source
cmake_minimum_required(VERSION 3.1)
else()
cmake_minimum_required(VERSION 2.8)
endif()

project(ALIVE)

option(UseValgrind "UseValgrind" OFF)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}")
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/3rdParty/cotire/CMake")
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/3rdParty/solar-cmake")
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")

include(cotire)
include(Solar)

# the RPATH to be used when installing
SET(CMAKE_INSTALL_RPATH "")

target_architecture(arch)
message(STATUS "arch is ${arch}")

# i386 for 32bit and x86_64 for 64bit
if ("${arch}" STREQUAL "i386")
    set (arch "x86")
elseif("${arch}" STREQUAL "x86_64")
    set (arch "x64")
    # Used by deflate lib
    add_definitions(-D_M_X64)
endif()

add_definitions(-DGLM_FORCE_SWIZZLE=1)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

enable_testing()

#set( CMAKE_VERBOSE_MAKEFILE on )

# The version number.
set(ALIVE_VERSION_MAJOR 0)
set(ALIVE_VERSION_MINOR 1)
set(ALIVE_VERSION_PATCH 0)

# Enable Visual Studio solution "folders"
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

if (APPLE)
# Required to prevent runtime crash on 64bit OSX - however this breaks 32bit compile
  set ( CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000 -image_base 7fff04c4a000" )
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
  #message("RELEASE set")
endif ()

if(UNIX AND NOT APPLE)
   set(LINUX TRUE)
endif()

# On Linux gtest needs pthreads
if (LINUX)
    find_package (Threads)
endif()

if (LINUX)
  find_package (PkgConfig REQUIRED)
  pkg_check_modules (GTK3 REQUIRED gtk+-3.0)
endif()

include_directories(
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files/generated_headers
    ${SDL2_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/packaging
    ${OPENGL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/squirrel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/boost_1_63_mini
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gl3w/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googlemock
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googlemock/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googlemock/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googlemock/internal/custom
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googletest
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googletest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googletest/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googletest/internal/custom
    ${CMAKE_BINARY_DIR}/3rdParty/luajit
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/luajit/src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/libdeflate
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/libdeflate/common
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/rapidjson/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/Detours/src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/sol2/single
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/vorbis/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/vorbis/lib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTK3_INCLUDE_DIRS}
)


SET(nativefiledialog_src 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/include/nfd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/nfd_common.c
)

if(LINUX)
    SET(nativefiledialog_src ${nativefiledialog_src} ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/nfd_gtk.c)
endif()

if(APPLE)
    SET(nativefiledialog_src ${nativefiledialog_src} ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/nfd_cocoa.m)
endif()

if(WIN32)
    SET(nativefiledialog_src ${nativefiledialog_src} ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nativefiledialog/src/nfd_win.cpp)
endif()

add_library(nativefiledialog STATIC ${nativefiledialog_src})
SET_PROPERTY(TARGET nativefiledialog PROPERTY FOLDER "3rdparty")

SET(lodepng_src 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/lodepng/lodepng.cpp
)
add_library(lodepng STATIC ${lodepng_src})
SET_PROPERTY(TARGET lodepng PROPERTY FOLDER "3rdparty")


SET(stk_src 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stk/src/Delay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stk/src/OnePole.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stk/src/FreeVerb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stk/src/Stk.cpp
)

add_library(stk STATIC ${stk_src})
SET_PROPERTY(TARGET stk PROPERTY FOLDER "3rdparty")


SET(nanovg_src 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/fontstash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/nanovg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/nanovg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/nanovg_gl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/nanovg_gl_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/nanovg/src/stb_truetype.h
)
add_library(NanoVg STATIC ${nanovg_src})
SET_PROPERTY(TARGET NanoVg PROPERTY FOLDER "3rdparty")

SET(gmock_src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googlemock/src/gmock-all.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/googletest/googletest/src/gtest-all.cc
)

add_library(gmock STATIC ${gmock_src})
SET_PROPERTY(TARGET gmock PROPERTY FOLDER "3rdparty")

SET(libdeflate_src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/libdeflate/lib/deflate_decompress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/libdeflate/lib/x86_cpu_features.c
)

add_library(libdeflate STATIC ${libdeflate_src})
SET_PROPERTY(TARGET libdeflate PROPERTY FOLDER "3rdparty")


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

set(VORBIS_HEADERS
    3rdParty/vorbis/lib/envelope.h
    3rdParty/vorbis/lib/lpc.h
    3rdParty/vorbis/lib/lsp.h
    3rdParty/vorbis/lib/codebook.h
    3rdParty/vorbis/lib/misc.h
    3rdParty/vorbis/lib/psy.h
    3rdParty/vorbis/lib/masking.h
    3rdParty/vorbis/lib/os.h
    3rdParty/vorbis/lib/mdct.h
    3rdParty/vorbis/lib/smallft.h
    3rdParty/vorbis/lib/highlevel.h
    3rdParty/vorbis/lib/registry.h
    3rdParty/vorbis/lib/scales.h
    3rdParty/vorbis/lib/window.h
    3rdParty/vorbis/lib/lookup.h
    3rdParty/vorbis/lib/lookup_data.h
    3rdParty/vorbis/lib/codec_internal.h
    3rdParty/vorbis/lib/backends.h
    3rdParty/vorbis/lib/bitrate.h
)

set(VORBIS_SOURCES
    3rdParty/vorbis/lib/mdct.c
    3rdParty/vorbis/lib/smallft.c
    3rdParty/vorbis/lib/block.c
    3rdParty/vorbis/lib/envelope.c
    3rdParty/vorbis/lib/window.c
    3rdParty/vorbis/lib/lsp.c
    3rdParty/vorbis/lib/lpc.c
    3rdParty/vorbis/lib/analysis.c
    3rdParty/vorbis/lib/synthesis.c
    3rdParty/vorbis/lib/psy.c
    3rdParty/vorbis/lib/info.c
    3rdParty/vorbis/lib/floor1.c
    3rdParty/vorbis/lib/floor0.c
    3rdParty/vorbis/lib/res0.c
    3rdParty/vorbis/lib/mapping0.c
    3rdParty/vorbis/lib/registry.c
    3rdParty/vorbis/lib/codebook.c
    3rdParty/vorbis/lib/sharedbook.c
    3rdParty/vorbis/lib/lookup.c
    3rdParty/vorbis/lib/bitrate.c
    3rdParty/ogg/framing.c
    3rdParty/ogg/bitwise.c
)

set(VORBISFILE_SOURCES
    3rdParty/vorbis/lib/vorbisfile.c
)

set(VORBISENC_SOURCES
    3rdParty/vorbis/lib/vorbisenc.c
)

add_library(libvorbis STATIC ${VORBISFILE_SOURCES} ${VORBISENC_SOURCES} ${VORBIS_SOURCES} ${VORBIS_HEADERS})
SET_PROPERTY(TARGET libvorbis PROPERTY FOLDER "3rdparty")

SET(oddlib_src
    src/gpuselect.cpp
    include/reverse_for.hpp
    include/types.hpp
    include/bitutils.hpp
    include/proxy_nanovg.h
    include/msvc_sdl_link.hpp
    include/stdthread.h
    include/logger.hpp
    include/string_util.hpp
    include/oddlib/exceptions.hpp
    include/oddlib/stream.hpp
    include/oddlib/anim.hpp
    include/oddlib/lvlarchive.hpp
    include/oddlib/masher.hpp
    include/oddlib/masher_tables.hpp
    src/oddlib/stream.cpp
    src/oddlib/anim.cpp
    src/oddlib/lvlarchive.cpp
    src/oddlib/masher.cpp
    include/oddlib/PSXMDECDecoder.h
    include/oddlib/PSXADPCMDecoder.h
    src/oddlib/PSXADPCMDecoder.cpp
    src/oddlib/PSXMDECDecoder.cpp
    src/oddlib/audio/AliveAudio.cpp
    src/oddlib/audio/SequencePlayer.cpp
    src/oddlib/audio/Soundbank.cpp
    src/oddlib/audio/vab.cpp
    src/oddlib/audio/Voice.cpp
    include/oddlib/audio/AliveAudio.h
    include/oddlib/audio/AudioInterpolation.h
    include/oddlib/audio/Sample.h
    include/oddlib/audio/SequencePlayer.h
    include/oddlib/audio/Soundbank.h
    include/oddlib/audio/vab.hpp
    include/oddlib/audio/Voice.h
    include/oddlib/cdromfilesystem.hpp
    src/oddlib/path.cpp
    include/oddlib/path.hpp
    src/oddlib/bits_ao_pc.cpp
    include/oddlib/bits_ao_pc.hpp
    src/oddlib/bits_ae_pc.cpp
    include/oddlib/bits_ae_pc.hpp
    src/oddlib/bits_psx.cpp
    include/oddlib/bits_psx.hpp
    src/oddlib/bits_factory.cpp
    include/oddlib/bits_factory.hpp
    src/oddlib/bits_fg1.cpp
    include/oddlib/bits_fg1.hpp
    include/oddlib/compressiontype3ae.hpp
    src/oddlib/compressiontype3ae.cpp
    include/oddlib/compressiontype4or5.hpp
    src/oddlib/compressiontype4or5.cpp
    include/oddlib/compressiontype6ae.hpp
    src/oddlib/compressiontype6ae.cpp
    include/oddlib/compressiontype3.hpp
    src/oddlib/compressiontype3.cpp
    include/oddlib/compressiontype2.hpp
    src/oddlib/compressiontype2.cpp
    include/oddlib/compressiontype6or7aepsx.hpp
    src/oddlib/compressiontype6or7aepsx.cpp
)

add_library(oddlib STATIC
    src/oddlib/oddlib_pch.cpp ${oddlib_src}
)

if (WIN32 AND MSVC)
    set_target_properties(oddlib PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/oddlib/oddlib_pch.h")
    cotire(oddlib)
endif()

SET(alivelib_src
    include/core/audiobuffer.hpp
    src/core/audiobuffer.cpp
    include/fmv.hpp
    src/fmv.cpp
    include/sound.hpp
    src/sound.cpp
    include/renderer.hpp
    src/renderer.cpp
    include/gui.h
    src/gui.cpp
    include/guiwidgets.hpp
    src/guiwidgets.cpp
    include/engine.hpp
    src/engine.cpp
    include/engine.hpp
    src/developerscreen.cpp
    include/developerscreen.hpp
    src/gameselectionscreen.cpp
    include/gameselectionscreen.hpp
    include/gridmap.hpp
    src/gridmap.cpp
    include/fsm.hpp
    src/fsm.cpp
    include/gridobject.hpp
    src/gridobject.cpp
    include/subtitles.hpp
    include/sdl_raii.hpp
    src/sdl_raii.cpp
    include/proxy_sol.hpp
    include/resourcemapper.hpp
    src/resourcemapper.cpp
    include/zipfilesystem.hpp
    src/zipfilesystem.cpp
    include/debug.hpp
    src/debug.cpp
    include/collisionline.hpp
    src/collisionline.cpp
    include/physics.hpp
    src/physics.cpp
)

add_library(AliveLib STATIC
    src/alivelib_pch.cpp ${alivelib_src}
)
if (WIN32 AND MSVC)
    set_target_properties(AliveLib PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/alivelib_pch.h")
    cotire(AliveLib)
endif()

if(WIN32)
SET(WIN32_RESOURCES_SRC
    rsc/Resource.rc
    rsc/resource.h
    rsc/app.ico)
endif()

add_executable(Alive MACOSX_BUNDLE
    ${WIN32_RESOURCES_SRC}
    src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gl3w/src/gl3w.c
)

if(LINUX)
    # Sometimes on Linux pthreads and rt need explicitly linking for SDL2
    SET(Alive_Libs AliveLib libvorbis jsonxx oddlib NanoVg stk ${OPENGL_LIBRARIES} pthread ${SDL2_LIBRARY} rt liblua lodepng libdeflate nativefiledialog ${GTK3_LIBRARIES})
ELSE()
    SET(Alive_Libs AliveLib libvorbis jsonxx oddlib NanoVg stk ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} liblua lodepng libdeflate nativefiledialog)
ENDIF()

#cotire(Alive)
TARGET_LINK_LIBRARIES(Alive ${Alive_Libs})

add_executable(DataTool
  ${WIN32_RESOURCES_SRC}
  test/data_test_main.cpp
  test/phash.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gl3w/src/gl3w.c
)
TARGET_LINK_LIBRARIES(DataTool ${Alive_Libs})

#cotire(oddlib)


add_executable(bin2h tools/bin2h/bin2h.cpp)
SET_PROPERTY(TARGET bin2h PROPERTY FOLDER "Header generators")

SET(OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

# Hook dll used for hacking/reversing hard coded game logic
# to reimplement in ALIVE
if (MSVC AND NOT "${CMAKE_GENERATOR}" MATCHES "(Win64)")
  
    SET(detours_src 
        3rdParty/Detours/src/detours.cpp
        3rdParty/Detours/src/disasm.cpp)

    set_source_files_properties(${detours_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} /wd4091 /DDETOURS_X86 /DDETOURS_32BIT")

    add_library(detours STATIC ${detours_src})
    SET_PROPERTY(TARGET detours PROPERTY FOLDER "3rdparty")

    add_library(ddraw SHARED
        tools/engine_hook/exports.def
        tools/engine_hook/dllmain.cpp
        tools/engine_hook/sound_hooks.cpp
        tools/engine_hook/sound_hooks.hpp
        tools/engine_hook/ddraw7proxy.hpp
        tools/engine_hook/dsurface7proxy.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gl3w/src/gl3w.c
        )
    set_target_properties(ddraw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
    set_target_properties(ddraw PROPERTIES LINK_FLAGS_RELEASE  "/SUBSYSTEM:WINDOWS")
    TARGET_LINK_LIBRARIES(ddraw detours ${Alive_Libs})
endif()


# Bake binary test files in C array/header files
function(make_header src dst targetname varname)
    message(STATUS "Generating header '${dst}' from " '${src}')
    ADD_CUSTOM_COMMAND(OUTPUT "${dst}"
                   COMMAND bin2h "${src}" "${dst}" ${varname}
                   DEPENDS "${src}" bin2h )

    add_custom_target(
      Generate_${targetname} ALL
      DEPENDS "${dst}" bin2h
    )

    SET_PROPERTY(TARGET Generate_${targetname} PROPERTY FOLDER "Header generators")

endfunction()

SET(generated_headers_dir ${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files/generated_headers)

# Helper to take file foo.dat in any path and place it in generated headers as foo.dat.g.h with
# var name of foo
function(to_vec src_file out_file_name)
    get_filename_component(NameOnly ${samples_dir}/${src_file} NAME)
    get_filename_component(NameOnlyNoExt ${samples_dir}/${src_file} NAME_WE)
    message(STATUS "File name is ${NameOnly} with out ext is ${NameOnlyNoExt}")
    make_header(${src_file}
                ${generated_headers_dir}/${out_file_name}.g.h 
                ${out_file_name}
                ${NameOnlyNoExt})
endfunction()

SET(samples_dir ${CMAKE_CURRENT_SOURCE_DIR}/test/sample_files)

to_vec(${samples_dir}/zips/MaxECDRComment.zip MaxECDRComment.zip)
to_vec(${samples_dir}/zips/SimpleNoComp.zip SimpleNoComp.zip)
to_vec(${samples_dir}/test.bin test.bin)
to_vec(${samples_dir}/xa.bin xa.bin)
to_vec(${samples_dir}/sample.lvl sample.lvl)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_8_high_compression_all_samples.ddv mono_8_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_8_low_compression_all_samples.ddv mono_8_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_16_high_compression_all_samples.ddv mono_16_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/mono_16_low_compression_all_samples.ddv mono_16_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_8_high_compression_all_samples.ddv stereo_8_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_8_low_compression_all_samples.ddv stereo_8_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_16_high_compression_all_samples.ddv stereo_16_high_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/audio_only/stereo_16_low_compression_all_samples.ddv stereo_16_low_compression_all_samples.ddv)
to_vec(${samples_dir}/ae_ddv/all_colours_low_compression_15fps_8bit_mono_high_compression_5_frames_interleave.ddv ddv_test1.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_high_compression_30_fps.ddv all_colours_high_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_low_compression_30_fps.ddv all_colours_low_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_max_compression_30_fps.ddv all_colours_max_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_medium_compression_30_fps.ddv all_colours_medium_compression_30_fps.ddv)
to_vec(${samples_dir}/ae_ddv/video_only/all_colours_min_compression_30_fps.ddv all_colours_min_compression_30_fps.ddv)

SET(generated_headers
    ${generated_headers_dir}/MaxECDRComment.zip.g.h
    ${generated_headers_dir}/SimpleNoComp.zip.g.h
    ${generated_headers_dir}/test.bin.g.h
    ${generated_headers_dir}/xa.bin.g.h
    ${generated_headers_dir}/sample.lvl.g.h
    ${generated_headers_dir}/mono_8_high_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/mono_8_low_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/mono_16_high_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/mono_16_low_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/stereo_8_high_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/stereo_8_low_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/stereo_16_high_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/stereo_16_low_compression_all_samples.ddv.g.h
    ${generated_headers_dir}/ddv_test1.ddv.g.h
    ${generated_headers_dir}/all_colours_high_compression_30_fps.ddv.g.h
    ${generated_headers_dir}/all_colours_low_compression_30_fps.ddv.g.h
    ${generated_headers_dir}/all_colours_max_compression_30_fps.ddv.g.h
    ${generated_headers_dir}/all_colours_medium_compression_30_fps.ddv.g.h
    ${generated_headers_dir}/all_colours_min_compression_30_fps.ddv.g.h
)

SET(test_src
    test/main.cpp
    test/resource_locator_test.cpp
    test/zip_fs_tests.cpp
    test/string_util_tests.cpp
    test/collision_test.cpp
    test/coordinatespace_test.cpp
    test/undoredo_test.cpp
    include/subtitles.hpp)

add_executable(Tests 
    ${test_src}
    ${generated_headers}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gl3w/src/gl3w.c
)

#cotire(Tests)
target_link_libraries(Tests oddlib gmock ${Alive_Libs} ${CMAKE_THREAD_LIBS_INIT})

if (UseValgrind)
    add_test(NAME Tests COMMAND valgrind -v --leak-check=full --track-origins=yes --show-reachable=yes --tool=memcheck --error-exitcode=1 $<TARGET_FILE:Tests> WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else()
    add_test(NAME Tests COMMAND Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()


# Generate header containing window title/version information
git_shorttag(GIT_SHORT_REV)
set(ALIVE_VERSION "ALIVE v${ALIVE_VERSION_MAJOR}.${ALIVE_VERSION_MINOR}.${ALIVE_VERSION_PATCH} rev(${GIT_SHORT_REV})")
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/packaging/alive_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/packaging/alive_version.h)


SET(CPACK_PACKAGE_EXECUTABLES Alive "A.L.I.V.E")
SET(CPACK_WIX_PROGRAM_MENU_FOLDER "Alive")
SET(CPACK_PACKAGE_VENDOR "PaulsApps.com")
SET(CPACK_PACKAGE_VERSION_MAJOR ${ALIVE_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${ALIVE_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${ALIVE_VERSION_PATCH})

install(
    TARGETS Alive 
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .)

if (WIN32 AND MSVC)
   add_custom_command(TARGET Alive POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_INCLUDE_DIR}/../lib/${arch}/SDL2.dll" "$(OutDir)SDL2.dll")
   INSTALL(FILES "${SDL2_INCLUDE_DIR}/../lib/${arch}/SDL2.dll" DESTINATION ".")
endif()

if (APPLE)
    # Fix link path to look in the bundle
    add_custom_command(TARGET Alive POST_BUILD
                   COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change /usr/local/lib/libSDL2-2.0.0.dylib @executable_path/../Frameworks/libSDL2-2.0.0.dylib $<TARGET_FILE:Alive>
                   COMMENT "Fixing up libSDL2-2.0.0.dylib rpath")

    INSTALL(FILES "/usr/local/Cellar/sdl2/2.0.5/lib/libSDL2-2.0.0.dylib" DESTINATION "Alive.app/Contents/Frameworks")

endif()

IF (APPLE)
    SET(DATA_DIR Alive.app/Contents/Resources/data)
ELSE()
    SET(DATA_DIR ./data)
ENDIF()

INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/DataSetIds.json" DESTINATION "${DATA_DIR}")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/Roboto-Regular.ttf" DESTINATION "${DATA_DIR}")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/resources.json" DESTINATION "${DATA_DIR}")

INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/subtitles/INGRDNT_DDV_AePc.SRT" DESTINATION "${DATA_DIR}/subtitles")

INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/abe.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/background_animation.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/door.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/electric_wall.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/hoist.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/mine.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/object_factory.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/slam_door.lua" DESTINATION "${DATA_DIR}/scripts")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/scripts/switch.lua" DESTINATION "${DATA_DIR}/scripts")

INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesExoddusPc.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesExoddusPcDemo.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesExoddusPsx.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesExoddusPsxCd1.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesExoddusPsxCd2.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesExoddusPsxDemo.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesOddyseePc.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesOddyseePcDemo.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesOddyseePsxDemo.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/AbesOddyseePsx.json" DESTINATION "${DATA_DIR}/GameDefinitions")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/GameDefinitions/Developer mode.json" DESTINATION "${DATA_DIR}/GameDefinitions")

if (MSVC)
    # Build cpp files on all cores
  
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Wall /wd4355 /wd4201 /wd4324 /wd4464 /wd4371 /wd4503 /wd4351 /wd4514 /wd4350 /wd4710 /wd4571 /wd4820 /wd4668 /wd4626 /wd4625 /wd4365 /wd4640 /wd4628 /wd4061 /wd4826 /wd4127 /wd4711 /wd4738 /wd4255 /wd4868 /WX")

    # Warnings added in VS2013 and later
    if(MSVC_VERSION GREATER 1800)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd5026 /wd5027")
    endif()
    
    set_source_files_properties(${test_src} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} /wd5026 /wd5027")

    # Use default flags on 3rd party things that generate warnings on max levels
    
    set_source_files_properties(${nativefiledialog_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS /wd4018 /wd4267 /wd4334 /wd4244")
    set_source_files_properties(${libdeflate_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4334 /wd4244")
    set_source_files_properties(${lodepng_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4334")
    set_source_files_properties(${stk_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} /wd4018 /wd4267")
    set_source_files_properties(${jsonxx_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
    set_source_files_properties(${gmock_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
    set_source_files_properties(${oddlib_src} PROPERTIES COMPILE_FLAGSS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    set_source_files_properties(${alivelib_src} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    set_source_files_properties(${nanovg_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4456 /wd4457 /wd4459")

    set_source_files_properties(${VORBISFILE_SOURCES} ${VORBISENC_SOURCES} ${VORBIS_SOURCES} ${VORBIS_HEADERS} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4305")

else()
    set_source_files_properties(${stk_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} -Wno-sign-compare")
    set_source_files_properties(${nanovg_src} PROPERTIES COMPILE_FLAGS "${OLD_CMAKE_CXX_FLAGS} -Wno-unused-result")

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # C++14 by default
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -fPIC -Wall -Werror")
    else()
        # Enable C++14, you may need to use -std=c++0x if using an older gcc compiler
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC -Wall -pedantic -Werror")
        
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
    SET(CPACK_WIX_PRODUCT_ICON ../rsc/app.ico)
    SET(CPACK_WIX_UI_BANNER  ../packaging/Wix_Ui_Banner_493x58.bmp)
    SET(CPACK_WIX_UI_DIALOG  ../packaging/Wix_Ui_Dialog_493x312.bmp)

    # If empty assume release
    #if ("${CMAKE_BUILD_TYPE}" STREQUAL "" OR "${CMAKE_BUILD_TYPE}" STREQUAL "release")
    #    message("RELEASE installer build")
    #else()
    #    message("DEBUG installer build")
    #endif()

elseif(APPLE)
    SET(CPACK_GENERATOR "DragNDrop")
elseif(LINUX)
    # On Linux generate TAR.GZ,DEB and RPM packages
    #Find out what architecture are we running on and set the package architecture 
endif()

INCLUDE(CPack)
