cmake_minimum_required(VERSION 3.2)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
project(sfwcl)
# install target is not used here
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}" CACHE INTERNAL "")

# Crysis SDK headers
include_directories(src/cpp/mod/CryEngine)

file(GLOB SFWCL_MOD_SOURCES src/cpp/mod/*.cpp)
add_library(sfwcl MODULE ${SFWCL_MOD_SOURCES})

option(SFWCL_PAK "Create sfwcl.pak" ON)
if(SFWCL_PAK)
	set(SFWCL_PAK_FILE "${PROJECT_BINARY_DIR}/sfwcl.pak")
	set(SFWCL_PAK_BASE "${PROJECT_SOURCE_DIR}/src/lua")
	add_custom_command(
		OUTPUT ${SFWCL_PAK_FILE}
		COMMAND ${CMAKE_COMMAND} -E tar cvf ${SFWCL_PAK_FILE} --format=zip Scripts
		WORKING_DIRECTORY ${SFWCL_PAK_BASE}
	)
	add_custom_target(mod_pak ALL DEPENDS ${SFWCL_PAK_FILE})
endif()

option(SFWCL_LAUNCHER "Build SafeWritingClient.exe" OFF)
if(SFWCL_LAUNCHER)
	add_executable(SafeWritingClient src/cpp/launcher/launcher.cpp)
endif()

option(SFWCL_MAPDL "Build MapDownloader.exe" OFF)
if(SFWCL_MAPDL)
	set(ZLIB_ROOT "" CACHE PATH "Directory containing zlib")
	find_package(ZLIB REQUIRED)
	if(ZLIB_FOUND)
		find_library(ZLIB_STATIC zlibstatic PATHS "${ZLIB_ROOT}")
		include_directories("${ZLIB_INCLUDE_DIRS}" "${ZLIB_ROOT}/contrib/minizip")
		add_library(miniunz STATIC
			"${ZLIB_ROOT}/contrib/minizip/unzip.c"
			"${ZLIB_ROOT}/contrib/minizip/ioapi.c"
			"${ZLIB_ROOT}/contrib/minizip/iowin32.c"
		)
		add_executable(MapDownloader src/cpp/mapdl/mapdl.cpp)
		target_link_libraries(MapDownloader miniunz "${ZLIB_STATIC}")
	endif()
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	message(STATUS "Using MSVC compiler options")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	if(SFWCL_MAPDL)
		set_target_properties(MapDownloader PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
	endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	message(STATUS "Using GNU compiler options")
	option(SFWCL_MINGW_STATIC "Link with -static-libstdc++ -static-libgcc" ON)
	if(SFWCL_MINGW_STATIC)
		set(SFWCL_MINGW_STATIC_FLAGS "-static-libstdc++ -static-libgcc")
		list(APPEND CMAKE_EXE_LINKER_FLAGS ${SFWCL_MINGW_STATIC_FLAGS})
		list(APPEND CMAKE_SHARED_LINKER_FLAGS ${SFWCL_MINGW_STATIC_FLAGS})
		list(APPEND CMAKE_MODULE_LINKER_FLAGS ${SFWCL_MINGW_STATIC_FLAGS})
	endif()
	if(SFWCL_MAPDL)
		set_target_properties(MapDownloader PROPERTIES LINK_FLAGS "-mwindows")
	endif()
else()
	message("Unknown compiler \'" ${CMAKE_CXX_COMPILER_ID} "\'")
endif()
