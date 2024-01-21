cmake_minimum_required(VERSION 3.9.0)

OPTION(ASAR_GEN_EXE_TEST "Build Asar standalone application test suite" ON)
OPTION(ASAR_GEN_DLL_TEST "Build Asar shared library test suite" ON)

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
		)

		if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "^LLVM-.*$")
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
		
		if(MSVC_VERSION VERSION_LESS "1900")
			target_compile_features(${target} PRIVATE cxx_std_11)
		else()
			# MSVC throws errors in STL headers when building with MSVC 2017 without C++14...
			target_compile_features(${target} PRIVATE cxx_std_14)
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
	target_compile_features(${target} PRIVATE cxx_std_11)
	endif()
endmacro()

set(CXX_EXTENSIONS OFF)

add_custom_target(run-tests)
set_property(TARGET run-tests PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)

if(ASAR_GEN_EXE_TEST)
	add_executable(
		asar-app-test
		
		"${CMAKE_CURRENT_SOURCE_DIR}/test.cpp"
	)

	set_asar_test_shared_properties(asar-app-test)

	add_custom_target(run-app-test
		asar-app-test "$<TARGET_FILE:asar-standalone>" "${CMAKE_CURRENT_SOURCE_DIR}/../../tests" "${CMAKE_CURRENT_SOURCE_DIR}/../../dummy_rom.sfc" "${CMAKE_CURRENT_BINARY_DIR}/tests-tmp-app"
		VERBATIM)
	add_dependencies(run-tests run-app-test)
	set_property(TARGET run-app-test PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)
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

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
	set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

	add_custom_target(run-dll-test
		asar-dll-test "$<TARGET_FILE:asar>" "${CMAKE_CURRENT_SOURCE_DIR}/../../tests" "${CMAKE_CURRENT_SOURCE_DIR}/../../dummy_rom.sfc" "${CMAKE_CURRENT_BINARY_DIR}/tests-tmp-dll"
		VERBATIM)
	add_dependencies(run-tests run-dll-test)
	set_property(TARGET run-dll-test PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endif()
