cmake_minimum_required(VERSION 2.8.7)
project (EngineHook)

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

IF (UNIX)
	IF (EXISTS "/home/paulm/Desktop/linux_to_windows_cross_compile/bin" AND IS_DIRECTORY "/home/paulm/Desktop/linux_to_windows_cross_compile/bin")
		SET(MINGW_INSTALL_PATH "/home/paulm/Desktop/linux_to_windows_cross_compile/bin")
	ENDIF()

	IF (EXISTS "/home/paul/Desktop/mingw/bin" AND IS_DIRECTORY "/home/paul/Desktop/mingw/bin")
		SET(MINGW_INSTALL_PATH "/home/paul/Desktop/mingw/bin")
	ENDIF()

	IF (EXISTS "/home/build/mingw/bin" AND IS_DIRECTORY "/home/build/mingw/bin")
		SET(MINGW_INSTALL_PATH "/home/build/mingw/bin")
	ENDIF()

	# here is the target environment located
	SET(CMAKE_FIND_ROOT_PATH ${MINGW_INSTALL_PATH})

	# which compilers to use for C and C++
	SET(CMAKE_C_COMPILER ${MINGW_INSTALL_PATH}/i686-w64-mingw32-gcc)
	SET(CMAKE_CXX_COMPILER ${MINGW_INSTALL_PATH}/i686-w64-mingw32-g++)
	SET(CMAKE_RC_COMPILER ${MINGW_INSTALL_PATH}/i686-w64-mingw32-windres)

	# adjust the default behaviour of the FIND_XXX() commands:
	# search headers and libraries in the target environment, search 
	# programs in the host environment
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
ENDIF()

# -pedantic
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -static -static-libgcc -static-libstdc++ -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-s -flto -O3 -static -static-libgcc -static-libstdc++ -std=c++11")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

configure_file(
  ${PROJECT_SOURCE_DIR}/rsc/resources.rc.in
  ${PROJECT_SOURCE_DIR}/rsc/generated_resources.rc
  @ONLY)

SET (SRC
rsc/generated_resources.rc
inc/dsound.h
inc/jmphookedfunction.hpp
inc/msgsbase.hpp
inc/signaturescanner.hpp
inc/DDrawProxy7.h
inc/hex_print.h
inc/ledmsgs.hpp
inc/NewData.h
inc/sound_debug.hpp
inc/DDrawSurface7Proxy.h
inc/hintflymsgs.hpp
inc/Log.h
inc/pointerpatcher.hpp
inc/stringutils.hpp
inc/DDrawUtils.h
inc/HookMain.h
inc/lvlreader.hpp
inc/resource.h
inc/vab.hpp
inc/DSound7Proxy.h
inc/IatHookedFunction.hpp
inc/memoryprotection.hpp
src/DDrawSurface7Proxy.cpp
src/DDrawUtils.cpp
src/DllMain.cpp
src/HookMain.cpp
src/memoryprotection.cpp
src/sound_debug.cpp
src/vab.cpp
)

# Enable the "resource" language to allow CMake to understand RC files
ENABLE_LANGUAGE(RC) 

IF (UNIX)
	SET(CMAKE_SHARED_LINKER_FLAGS "'${PROJECT_SOURCE_DIR}/rsc/Exports.def'")
ELSE()
	if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		SET(CMAKE_SHARED_LINKER_FLAGS "-static \"${PROJECT_SOURCE_DIR}/rsc/Exports.def\"")
	else()
		# -s means: Omit all symbol information from the output file.
		SET(CMAKE_SHARED_LINKER_FLAGS "-s -static \"${PROJECT_SOURCE_DIR}/rsc/Exports.def\"")
	endif()
ENDIF()

add_library(EngineHook SHARED ${SRC} )
TARGET_LINK_LIBRARIES(EngineHook psapi gdi32 shlwapi version)

if (PUB_DIR)
add_custom_target (
    publish_output_EngineHook ALL
	COMMAND ${CMAKE_COMMAND} -E
                         copy "$<TARGET_FILE:EngineHook>" "${PUB_DIR}/hook/ddraw.dll"
 )
endif()
