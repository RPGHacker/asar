cmake_minimum_required(VERSION 3.9.0)

OPTION(ASAR_GEN_EXE_TEST "Build Asar standalone application test suite" ON)
OPTION(ASAR_GEN_DLL_TEST "Build Asar shared library test suite" ON)

set(CMAKE_CXX_STANDARD 17)

# Shared settings for Asar test applications

macro(set_asar_test_shared_properties target)
	# Maximum warning level
	if(MSVC)
		target_compile_definitions(${target} PRIVATE "_CRT_SECURE_NO_WARNINGS")
		target_compile_options(${target} PRIVATE /Wall /MT /EHa)
		
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
		)

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
	else()
		if(NOT MINGW)
			# Not available nor needed on mingw
			target_link_libraries(${target} dl)
		endif()

		target_compile_options(${target} PRIVATE -Wall -pedantic
			-Wno-varargs # passing an object that undergoes default argument promotion to 'va_start' has undefined behavior
			-Wno-unused-result # ignoring return value
		)

		# Static link for MinGW
		if(MINGW)
			target_compile_options(${target} PRIVATE -static -static-libgcc -s)
			target_link_libraries(${target} PRIVATE -static -static-libgcc -s)
		endif()

		# for some reason this isn't available on MSVC?
		target_compile_features(${target} PRIVATE c_std_99)
	endif()
			
	if (WIN32)
		# RPG Hacker: We define UNICODE on Windows, just to make sure we don't accidentally call an ANSI API somewhere.
		target_compile_definitions(${target} PRIVATE "UNICODE" "_UNICODE")
	endif()
endmacro()

set(CXX_EXTENSIONS OFF)

add_custom_target(run-tests)

if(ASAR_GEN_EXE_TEST)
	add_executable(
		asar-app-test
		
		"${CMAKE_CURRENT_SOURCE_DIR}/test.cpp"
	)

	set_asar_test_shared_properties(asar-app-test)

	add_custom_target(run-app-test
		asar-app-test $<TARGET_FILE:asar-standalone> ${CMAKE_CURRENT_SOURCE_DIR}/../../tests ${CMAKE_CURRENT_SOURCE_DIR}/../../dummy_rom.sfc ${CMAKE_CURRENT_BINARY_DIR}/tests-tmp)
	add_dependencies(run-tests run-app-test)
endif()

if(ASAR_GEN_DLL_TEST)
	add_executable(
		asar-dll-test
		
		"${CMAKE_CURRENT_SOURCE_DIR}/test.cpp"
		
		"${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/c/asardll.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/c/asardll.h"
	)

	target_include_directories(asar-dll-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/c/")
	target_compile_definitions(asar-dll-test PRIVATE ASAR_TEST_DLL)

	set_asar_test_shared_properties(asar-dll-test)

	add_custom_target(run-dll-test
		asar-dll-test $<TARGET_FILE:asar> ${CMAKE_CURRENT_SOURCE_DIR}/../../tests ${CMAKE_CURRENT_SOURCE_DIR}/../../dummy_rom.sfc ${CMAKE_CURRENT_BINARY_DIR}/tests-tmp)
	add_dependencies(run-tests run-dll-test)

	add_executable(newtest
		"${CMAKE_CURRENT_SOURCE_DIR}/newtest.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/c/asardll.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/c/asardll.h")
	target_include_directories(newtest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/c/")
	set_asar_test_shared_properties(newtest)

	add_custom_target(run-newtest
		newtest $<TARGET_FILE:asar> ${CMAKE_CURRENT_SOURCE_DIR}/../../tests ${CMAKE_CURRENT_SOURCE_DIR}/../../dummy_rom.sfc)
	add_dependencies(run-tests run-newtest)

	if(ASAR_USE_SANITIZER)
		target_compile_options(asar-dll-test PRIVATE -fsanitize=address -fsanitize=undefined)
		target_link_options(asar-dll-test PRIVATE -fsanitize=address -fsanitize=undefined)
		target_compile_options(newtest PRIVATE -fsanitize=address -fsanitize=undefined)
		target_link_options(newtest PRIVATE -fsanitize=address -fsanitize=undefined)
	endif()

	if(NOT ASAR_USE_SANITIZER)
		# the interface tests run python, and we can't run ASan in python (or
		# well, we can, but it has some memory leaks which ASan considers an
		# error) so disable the interface tests in this case.
		add_custom_target(run-interface-test
			"${CMAKE_COMMAND}" -E env "ASARDLL=$<TARGET_FILE:asar>" python3 interface-tests/main.py
			VERBATIM
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
		add_dependencies(run-tests run-interface-test)
	endif()
endif()
