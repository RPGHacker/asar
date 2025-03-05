
# Asar build rules

cmake_minimum_required(VERSION 3.13.0...3.31.0)

option(ASAR_GEN_EXE "Build Asar standalone application" ON)
option(ASAR_GEN_DLL "Build Asar shared library" ON)
option(ASAR_GEN_LIB "Build Asar static library" ON)
option(ASAR_COVERAGE "Build Asar with coverage tracking support" OFF)
option(ASAR_USE_THREADS "Run Asar in a new thread to alleviate stack usage problems" ON)
set(MSVC_LIB_TYPE "" CACHE STRING "In MSVC, set the type of the runtime library: options are D for dynamic and T for static")


set (CMAKE_CXX_STANDARD 17)

set(ASAR_API_VERSION_MAJOR 4)
set(ASAR_API_VERSION_MINOR 0)
set(ASAR_API_VERSION_PATCH 0)
set(ASAR_API_VERSION ${ASAR_API_VERSION_MAJOR}.${ASAR_API_VERSION_MINOR}.${ASAR_API_VERSION_PATCH})

include(GNUInstallDirs)

# This maro sets a number of properties required by both,
# stand-alone application and library interface

macro(set_asar_shared_properties target msvc_lib_type_param enable_sanitizer)
	# this is here because in CMake you can't modify macro parameters
	# so we create a new variable
	set(msvc_lib_type_var ${msvc_lib_type_param})
	if (MSVC)
		if (NOT MSVC_LIB_TYPE STREQUAL "")
			set(msvc_lib_type_var ${MSVC_LIB_TYPE})
		endif()
		if ("${msvc_lib_type_var}" STREQUAL "D")
			set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
		elseif ("${msvc_lib_type_var}" STREQUAL "T")
			set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		else()
			message(FATAL_ERROR "Invalid MSVC_LIB_TYPE, valid types are T and D instead of ${msvc_lib_type_var}")
		endif()
	endif()

	if (${enable_sanitizer} AND ${ASAR_USE_SANITIZER})
		if(MSVC)
			target_compile_options(${target} PRIVATE /fsanitize=address)
			target_link_options(${target} PRIVATE /fsanitize=address)
		else()
			target_compile_options(${target} PRIVATE -fsanitize=address -fsanitize=undefined)
			target_link_options(${target} PRIVATE -fsanitize=address -fsanitize=undefined)
		endif()
	endif()

	if(ASAR_COVERAGE)
		target_compile_options(${target} PRIVATE --coverage)
		target_link_options(${target} PRIVATE --coverage)
	endif()

	target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

	if(MSVC)
		target_compile_definitions(${target} PRIVATE "strncasecmp=_strnicmp")
		target_compile_definitions(${target} PRIVATE "strcasecmp=_stricmp")
		target_compile_definitions(${target} PRIVATE "_CRT_SECURE_NO_WARNINGS")
		target_compile_definitions(${target} PRIVATE "_CRT_NONSTDC_NO_WARNINGS")
		target_compile_definitions(${target} PRIVATE "_ALLOW_KEYWORD_MACROS")

		if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "^LLVM-.*$" OR CMAKE_VS_PLATFORM_TOOLSET STREQUAL "llvm" OR CMAKE_VS_PLATFORM_TOOLSET STREQUAL "ClangCL")
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
				-Wno-extra-semi-stmt # remove unnecessary ';'

				# a bunch of warnings about casts which we honestly should deal with at some point, but there's just way too many of them
				-Wno-sign-conversion # implicit conversion changes signedness
				-Wno-shorten-64-to-32 # implicit conversion of int64_t to int32_t
				-Wno-old-style-cast # use of old-style cast
				-Wno-cast-qual # cast drops const qualifier
				-Wno-implicit-int-conversion # implicit conversion loses precision

				# RPG Hacker: These two are really nasty, but not easily fixable in Asar, so have to disable them...
				-Wno-exit-time-destructors # declaration requires an exit-time destructor
				-Wno-global-constructors # delcaration requires a global destructor
			)
		endif()
	endif()

	if (WIN32)
		target_compile_definitions(${target} PRIVATE "windows")
		# RPG Hacker: We define UNICODE on Windows, just to make sure we don't accidentally call an ANSI API somewhere.
		target_compile_definitions(${target} PRIVATE "UNICODE" "_UNICODE")
	endif()

	if (UNIX)
		target_compile_definitions(${target} PRIVATE "linux")
		target_compile_definitions(${target} PRIVATE "stricmp=strcasecmp")

		if(ASAR_USE_THREADS)
			set(THREADS_PREFER_PTHREAD_FLAG ON)
			find_package(Threads REQUIRED)
			target_link_libraries(${target} PRIVATE Threads::Threads)
		endif()
	endif()


	# Enable maximum warning level

	if(MSVC)
		target_compile_options(${target} PRIVATE /W3 /EHa)
		
		# These certainly aren't worth a warning, though
		target_compile_options(${target} PRIVATE
			/wd4514 # unreferenced inline function removed
			/wd4710 # function not inlined
			/wd4711 # function selected for automatic inline expansion
			/wd4820 # 'bytes' bytes padding added after construct 'member_name'
			/wd4464 # relativ include path contains '..'
			/wd4668 # undefined define used in #if (thrown in windows headers)
			/wd5045 # compiler will insert spectre mitigation
			/wd4365 # implicit conversion changes signedness
			/wd4242 # implicit conversion loses precision
			/wd4244 # implicit conversion loses precision
			/wd4267 # implicit conversion loses precision
			/wd4800 # implicit conversion to bool
			/wd5045 # compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
		)
		target_link_options(${target} PRIVATE /STACK:4194304)
	else()
		target_compile_options(${target} PRIVATE -Wall -pedantic
			-Wno-varargs # passing an object that undergoes default argument promotion to 'va_start' has undefined behavior
			-Wno-nested-anon-types
		)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			# doesn't exist on clang, and clang yells about unknown options
			target_compile_options(${target} PRIVATE -Wno-class-memaccess)
		endif()

		# Static link for MinGW
		if(MINGW)
			target_compile_options(${target} PRIVATE -static -static-libgcc -s)
			target_link_libraries(${target} PRIVATE -static -static-libgcc -s)
		endif()

		# for some reason this isn't available on MSVC?
		target_compile_features(${target} PRIVATE c_std_99)
	endif()

	if(NOT(ASAR_USE_THREADS))
		target_compile_definitions(${target} PRIVATE NO_USE_THREADS)
	endif()
endmacro()





# Add shared source files for Asar
set(ASAR_SHARED_SOURCE_FILES "")

set(ASAR_PUBLIC_HEADER
	"${CMAKE_CURRENT_SOURCE_DIR}/../dll-bindings/asar.h"
)

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
	"${CMAKE_CURRENT_SOURCE_DIR}/math_eval.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/math_functions.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/math_parse.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/virtualfile.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/warnings.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/errors.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/table.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/unicode.cpp"

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
	"${CMAKE_CURRENT_SOURCE_DIR}/unicode.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/assembleblock.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/asar_math.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/macro.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/interface-shared.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/arch-shared.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/virtualfile.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/table.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/std-includes.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/platform/file-helpers.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/platform/thread-helpers.h"
	"${ASAR_PUBLIC_HEADER}"
)


# Add resource files and other platform-specific source files
set(ASAR_RESOURCE_FILES "")
set(ASAR_LIB_RESOURCE_FILES "")

if(WIN32)
	# Windos-specific files
	list(
		APPEND ASAR_SHARED_SOURCE_FILES

		"${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/file-helpers-win32.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/thread-helpers-win32.h"
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
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/generic/thread-helpers-pthread.h"
	)
else()
	# Files for any other platform
	list(
		APPEND ASAR_SHARED_SOURCE_FILES

		"${CMAKE_CURRENT_SOURCE_DIR}/platform/generic/file-helpers-generic.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/generic/thread-helpers-pthread.h"
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

	set_asar_shared_properties(asar-standalone "T" TRUE)

	set_target_properties(asar-standalone
		PROPERTIES
		OUTPUT_NAME "asar"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
	)

	install(TARGETS asar-standalone DESTINATION ${CMAKE_INSTALL_BINDIR})

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
	set_asar_shared_properties(asar "T" TRUE)

	set_target_properties(asar
		PROPERTIES
		VERSION "${ASAR_API_VERSION}"
		SOVERSION "${ASAR_API_VERSION_MAJOR}"
		PUBLIC_HEADER "${ASAR_PUBLIC_HEADER}"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
	)

	install(
		TARGETS asar DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/asar
	)

endif(ASAR_GEN_DLL)





# Define static library

if (ASAR_GEN_LIB)
	add_library(
		asar-static STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/interface-lib.cpp"

		${ASAR_SHARED_SOURCE_FILES}
		${ASAR_LIB_RESOURCE_FILES}
	)
	target_compile_definitions(asar-static PRIVATE "ASAR_STATIC")
	set_asar_shared_properties(asar-static "D" FALSE)

	set_target_properties(asar-static
		PROPERTIES
		PUBLIC_HEADER "${ASAR_PUBLIC_HEADER}"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
	)

	install(
		TARGETS asar-static DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/asar
	)

endif()





# Define a pkgconfig file

if (ASAR_GEN_DLL OR ASAR_GEN_LIB)
	set (ASAR_PROJECT_NAME "asar")
	set (ASAR_PKGCONFIG_NAME "res/pkgconfig/libasar.pc")

	include(../../cmake/JoinPaths.cmake)

	join_paths(
		ASAR_PKGCONF_LIBDIR "\${prefix}" "${CMAKE_INSTALL_LIBDIR}"
	)

	join_paths(
		ASAR_PKGCONF_INCLUDEDIR "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}"
	)

	configure_file(
		"${ASAR_PKGCONFIG_NAME}.in"
		"${CMAKE_CURRENT_BINARY_DIR}/${ASAR_PKGCONFIG_NAME}"
		@ONLY
	)

	install(
		FILES "${CMAKE_CURRENT_BINARY_DIR}/${ASAR_PKGCONFIG_NAME}"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	)

endif()
