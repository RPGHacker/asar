
# Asar build rules

cmake_minimum_required(VERSION 3.9.0)





# This maro sets a number of properties required by both,
# stand-alone application and library interface

macro(set_asar_shared_properties target)
	target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
	
	if(MSVC)
		target_compile_definitions(${target} PRIVATE "strncasecmp=_strnicmp")
		target_compile_definitions(${target} PRIVATE "strcasecmp=_stricmp")
		target_compile_definitions(${target} PRIVATE "_CRT_SECURE_NO_WARNINGS")
		target_compile_definitions(${target} PRIVATE "_CRT_NONSTDC_NO_WARNINGS")
		target_compile_definitions(${target} PRIVATE "_ALLOW_KEYWORD_MACROS")
	endif()
		
	if (WIN32)
		target_compile_definitions(${target} PRIVATE "windows")
	endif()
		
	if (UNIX)
		target_compile_definitions(${target} PRIVATE "linux")
		target_compile_definitions(${target} PRIVATE "stricmp=strcasecmp")
	endif()
	
	
	# Enable maximum warning level
	
	if(MSVC)		
		if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
			string(REGEX REPLACE "/W[0-4]" "/Wall" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
		endif()		
		
		# These certainly aren't worth a warning, though
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4514")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4710")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4820")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
	endif()
endmacro()





# Add shared source files for Asar
set(ASAR_SHARED_SOURCE_FILES "")

list(
	APPEND ASAR_SHARED_SOURCE_FILES
	
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-65816.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-spc700.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-superfx.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/assembleblock.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/libcon.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/libsmw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/libstr.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/macro.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/math.cpp"
	
	# Not required in here, but useful so they're add to your VS solution
	"${CMAKE_CURRENT_SOURCE_DIR}/asar.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/assocarr.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/autoarray.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libcon.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libmisc.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libsmw.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libstr.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/scapegoat.hpp"
	
	"${CMAKE_CURRENT_SOURCE_DIR}/std-includes.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/platform/file-helpers.h"
)


# Add resource files and other platform-specific source files
set(ASAR_RESOURCE_FILES "")
set(ASAR_LIB_RESOURCE_FILES "")

if(WIN32)
	# Windos-specific files
	list(
		APPEND ASAR_SHARED_SOURCE_FILES
		
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/file-helpers-win32.cpp"
	)
	
	list(
		APPEND ASAR_RESOURCE_FILES		
	
		"${CMAKE_CURRENT_SOURCE_DIR}/res/windows/asar.rc"
	)

	list(
		APPEND ASAR_LIB_RESOURCE_FILES		
	
		"${CMAKE_CURRENT_SOURCE_DIR}/res/windows/asar_lib.rc"
	)
elseif(UNIX)
	# Linux-specific files
	list(
		APPEND ASAR_SHARED_SOURCE_FILES
		
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/linux/file-helpers-linux.cpp"
	)
else()
	# Files for any other platform
	list(
		APPEND ASAR_SHARED_SOURCE_FILES
		
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/generic/file-helpers-generic.cpp"
	)
endif()





# Define stand-alone application

add_executable(
	asar-standalone
	
	"${CMAKE_CURRENT_SOURCE_DIR}/interface-cli.cpp"
	
	${ASAR_SHARED_SOURCE_FILES}	
	${ASAR_RESOURCE_FILES}
)

set_asar_shared_properties(asar-standalone)





# Define dynamic library

add_library(
	asar SHARED	
	
	"${CMAKE_CURRENT_SOURCE_DIR}/interface-lib.cpp"
	
	${ASAR_SHARED_SOURCE_FILES}
	${ASAR_LIB_RESOURCE_FILES}
)

set_asar_shared_properties(asar)
