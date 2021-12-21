
# Asar build rules

cmake_minimum_required(VERSION 3.9.0)

option(ASAR_GEN_EXE "Build Asar standalone application" ON)
option(ASAR_GEN_DLL "Build Asar shared library" ON)
option(ASAR_COVERAGE "Build Asar with coverage tracking support" OFF)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")


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

		if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "^LLVM-.*$" OR CMAKE_VS_PLATFORM_TOOLSET STREQUAL "llvm")
			target_compile_options(${target} PRIVATE
				-Wno-old-style-cast # use of old-style-cast
				-Wno-unknown-argument # unknown argument ignored in clang-cl
				-Wno-unused-command-line-argument # argument unused during compilation
				-Wno-c++98-compat # 'x' is incompatible with C++98
				-Wno-c++98-compat-pedantic
				-Wno-missing-noreturn # 'x' could be declared with attribute 'noreturn'
				-Wno-float-equal # comparting floating point with == or != is unsafe
				-Wno-covered-switch-default # default label in switch which covers all enumeration values
				-Wno-varargs # passing an object that undergoes default argument promotion to 'va_start' has undefined behavior

				# RPG Hacker: These two are really nasty, but not easily fixable in Asar, so have to disable them...
				-Wno-exit-time-destructors # declaration requires an exit-time destructor
				-Wno-global-constructors # delcaration requires a global destructor
			)
		endif()
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
		target_compile_options(${target} PRIVATE /Wall /MT /EHa)
		
		# These certainly aren't worth a warning, though
		target_compile_options(${target} PRIVATE
			/wd4514 # unreferenced inline function removed
			/wd4710 # function not inlined
			/wd4711 # function selected for automatic inline expansion
			/wd4820 # 'bytes' bytes padding added after construct 'member_name'
			/wd4464 # relativ include path contains '..'
		)
		target_link_options(${target} PRIVATE /STACK:4194304)
		if(MSVC_VERSION VERSION_LESS "1900")
			target_compile_features(${target} PRIVATE cxx_std_11)
		else()
			# MSVC throws errors in STL headers when building with MSVC 2017 without C++14...
			target_compile_features(${target} PRIVATE cxx_std_14)
		endif()
	else()
		target_compile_options(${target} PRIVATE -Wall -pedantic
			-Wno-varargs # passing an object that undergoes default argument promotion to 'va_start' has undefined behavior
			-Wno-unused-result # ignoring return value
			-Wno-class-memaccess # clearing/moving a non-trivially copyable type
		)

		# Static link for MinGW
		if(MINGW)
			target_compile_options(${target} PRIVATE -static -static-libgcc -s)
			target_link_libraries(${target} PRIVATE -static -static-libgcc -s)
		endif()

		# for some reason this isn't available on MSVC?
		target_compile_features(${target} PRIVATE c_std_99)

		target_compile_features(${target} PRIVATE cxx_std_11)
	endif()
endmacro()





# Add shared source files for Asar
set(ASAR_SHARED_SOURCE_FILES "")

list(
	APPEND ASAR_SHARED_SOURCE_FILES
	
	"${CMAKE_CURRENT_SOURCE_DIR}/addr2line.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-65816.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-spc700.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-superfx.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/assembleblock.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/crc32.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/libcon.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/libsmw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/libstr.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/macro.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar_math.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/virtualfile.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/warnings.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/errors.cpp"

	"${CMAKE_CURRENT_SOURCE_DIR}/platform/file-helpers.cpp"
	
	# Not required in here, but useful so they're add to your VS solution
	"${CMAKE_CURRENT_SOURCE_DIR}/addr2line.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/assocarr.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/autoarray.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/crc32.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libcon.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libmisc.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libsmw.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/libstr.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/warnings.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/errors.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/assembleblock.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar_math.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/macro.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/interface-shared.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-shared.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/virtualfile.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/dll_helper.h"
	
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

if(ASAR_GEN_EXE)
	add_executable(
		asar-standalone

		"${CMAKE_CURRENT_SOURCE_DIR}/interface-cli.cpp"

		${ASAR_SHARED_SOURCE_FILES}	
		${ASAR_RESOURCE_FILES}
		)

	set_asar_shared_properties(asar-standalone)

	if(ASAR_COVERAGE)
		target_compile_options(asar-standalone PRIVATE -fprofile-arcs -ftest-coverage)
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov --coverage -fprofile-arcs")
	endif()
endif(ASAR_GEN_EXE)





# Define dynamic library

if(ASAR_GEN_DLL)
	add_library(
		asar SHARED	
		
		"${CMAKE_CURRENT_SOURCE_DIR}/interface-lib.cpp"
		
		${ASAR_SHARED_SOURCE_FILES}
		${ASAR_LIB_RESOURCE_FILES}
	)
	target_compile_definitions(asar PRIVATE "ASAR_SHARED")
	set_asar_shared_properties(asar)
endif(ASAR_GEN_DLL)
