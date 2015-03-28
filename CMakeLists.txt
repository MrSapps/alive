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
    ${SDL2_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/include
	${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest/include
	${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest
)

SET(gmock_src
	${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/src/gmock-all.cc
	${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/gmock-1.7.0/gtest/src/gtest-all.cc
)

add_library(gmock STATIC ${gmock_src})

add_executable(Tests
	test/main.cpp
)
target_link_libraries(Tests gmock ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME Tests COMMAND Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Alive
	src/main.cpp
)
TARGET_LINK_LIBRARIES(Alive ${SDL2_LIBRARY})

if (MSVC)
	# Build cpp files on all cores
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W4")
else()
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		# C++11 by default
	else()
		# Enable C++11, you may need to use -std=c++0x if using an older gcc compiler
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Weffc++ -pedantic -Wmissing-field-initializers")
		
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
    SET(CPACK_WIX_UPGRADE_GUID 8fbb293f-5622-4f6d-a7f6-c221f7be5d52)

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