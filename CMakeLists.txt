cmake_minimum_required(VERSION 3.7)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

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

option(SFWCL_MAPDL_DLL "Build mapdl.dll" ON)
if(SFWCL_MAPDL_DLL)
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
	add_library(mapdl MODULE src/cpp/mapdl/mapdl.cpp)
	target_compile_definitions(mapdl PUBLIC -DMAKE_DLL)
	target_link_libraries(mapdl miniunz)
endif()
