cmake_minimum_required(VERSION 3.7)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build")
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# force static link of MSVC runtime by overriding compiler flags
# https://cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_c_flags_override.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_cxx_flags_override.cmake)

# install target is not used here
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}" CACHE INTERNAL "")

project(sfwcl)

if(NOT MSVC)
	message(FATAL_ERROR "MSVC is the only supported compiler")
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

option(SFWCL_DLL "Build sfwcl.dll" ON)
if(SFWCL_DLL)
	file(GLOB SFWCL_DLL_SOURCES src/cpp/mod/*.cpp)
	add_library(sfwcl MODULE ${SFWCL_DLL_SOURCES})
	# Crysis SDK headers
	target_include_directories(sfwcl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/mod/CryEngine)
	target_compile_definitions(sfwcl PUBLIC -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
endif()

# only Windows version of 7-zip creates valid .pak files
if(CMAKE_HOST_WIN32)
	option(SFWCL_PAK "Create sfwcl.pak (requires 7-zip)" OFF)
	if(SFWCL_PAK)
		set(SFWCL_PAK_FILE "${CMAKE_CURRENT_BINARY_DIR}/sfwcl.pak")
		set(SFWCL_PAK_CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/src/lua/Scripts")
		option(SFWCL_PAK_CRYMP_LOGO "Add CryMP logo (GameSpy logo replacement) to sfwcl.pak" ON)
		if(SFWCL_PAK_CRYMP_LOGO)
			list(APPEND SFWCL_PAK_CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/lib/crymp_logo/Libs")
		endif()
		find_program(7Z_EXE 7z PATHS "$ENV{ProgramFiles}/7-Zip" "$ENV{ProgramFiles\(x86\)}/7-Zip" DOC "Path to 7z executable")
		if(NOT 7Z_EXE)
			message(FATAL_ERROR "7-zip executable not found")
		endif()
		message(STATUS "Using 7-zip: ${7Z_EXE}")
		add_custom_command(
			OUTPUT ${SFWCL_PAK_FILE}
			COMMAND ${7Z_EXE} a -bb -bd -tzip ${SFWCL_PAK_FILE} ${SFWCL_PAK_CONTENT}
		)
		add_custom_target(mod_pak ALL DEPENDS ${SFWCL_PAK_FILE})
		if(MSVC_IDE)
			# copy sfwcl.pak to output directory created by Visual Studio
			add_custom_command(TARGET mod_pak POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy ${SFWCL_PAK_FILE} ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
			)
		endif()
	endif()
endif()

option(SFWCL_PRECHECK "Build sfwcl_precheck.exe" OFF)
if(SFWCL_PRECHECK)
	add_executable(sfwcl_precheck src/cpp/precheck/main.cpp)
	# VS2010 and higher
	if(MSVC_IDE AND (MSVC_VERSION GREATER_EQUAL 1600))
		set_target_properties(sfwcl_precheck PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /level='requireAdministrator' /uiAccess='false'")
	else()
		set_target_properties(sfwcl_precheck PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
	endif()
endif()

option(SFWCL_LAUNCHER "Build SafeWritingClient.exe" OFF)
if(SFWCL_LAUNCHER)
	add_executable(SafeWritingClient src/cpp/launcher/launcher.cpp)
endif()

option(SFWCL_MAPDL_DLL "Build MapDownloader.dll" ON)
option(SFWCL_MAPDL_EXE "Build MapDownloader.exe" OFF)

if(SFWCL_MAPDL_DLL OR SFWCL_MAPDL_EXE)
	# hide not needed options of zlib
	mark_as_advanced(FORCE ASM686 AMD64 CMAKE_BACKWARDS_COMPATIBILITY EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)
	mark_as_advanced(FORCE INSTALL_BIN_DIR INSTALL_LIB_DIR INSTALL_INC_DIR INSTALL_MAN_DIR INSTALL_PKGCONFIG_DIR)
	add_subdirectory(lib/zlib EXCLUDE_FROM_ALL)
	add_library(miniunz STATIC
		lib/zlib/contrib/minizip/unzip.c
		lib/zlib/contrib/minizip/ioapi.c
		lib/zlib/contrib/minizip/iowin32.c
	)
	target_include_directories(miniunz PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/contrib/minizip
		${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib
		${CMAKE_CURRENT_BINARY_DIR}/lib/zlib
	)
	target_link_libraries(miniunz zlibstatic)
endif()

if(SFWCL_MAPDL_DLL)
	add_library(mapdl_dll MODULE src/cpp/mapdl/mapdl.cpp)
	target_compile_definitions(mapdl_dll PUBLIC -DMAKE_DLL)
	target_link_libraries(mapdl_dll miniunz)
	set_target_properties(mapdl_dll PROPERTIES OUTPUT_NAME "MapDownloader")
endif()

if(SFWCL_MAPDL_EXE)
	add_executable(mapdl_exe src/cpp/mapdl/mapdl.cpp)
	target_link_libraries(mapdl_exe miniunz)
	set_target_properties(mapdl_exe PROPERTIES OUTPUT_NAME "MapDownloader")
	# VS2010 and higher
	if(MSVC_IDE AND (MSVC_VERSION GREATER_EQUAL 1600))
		set_target_properties(mapdl_exe PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /level='requireAdministrator' /uiAccess='false'")
	else()
		set_target_properties(mapdl_exe PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
	endif()
endif()
