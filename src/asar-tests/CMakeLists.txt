cmake_minimum_required(VERSION 3.9.0)

OPTION(ASAR_GEN_EXE_TEST "Build Asar standalone application test suite" ON)
OPTION(ASAR_GEN_DLL_TEST "Build Asar shared library test suite" ON)

# Shared settings for Asar test applications

macro(set_asar_test_shared_properties target)
    # Maximum warning level
    if(MSVC)
        target_compile_definitions(${target} PRIVATE "_CRT_SECURE_NO_WARNINGS")
        target_compile_options(${target} PRIVATE /Wall)
        
        # These certainly aren't worth a warning, though
        target_compile_options(${target} PRIVATE
            /wd4514 # unreferenced inline function removed
            /wd4710 # function not inlined
            /wd4711 # function selected for automatic inline expansion
            /wd4820 # 'bytes' bytes padding added after construct 'member_name'
        )
		
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
			target_compile_options(${target} PRIVATE -static -static-libgcc -static-libstdc++ -s)
		endif()

        # for some reason this isn't available on MSVC?
        target_compile_features(${target} PRIVATE c_std_99)
		target_compile_features(${target} PRIVATE cxx_std_11)
    endif()
endmacro()

set(CXX_EXTENSIONS OFF)

if(ASAR_GEN_EXE_TEST)
    add_executable(
        asar-app-test
        
        "test.cpp"
    )

    set_asar_test_shared_properties(asar-app-test)
endif()

if(ASAR_GEN_DLL_TEST)
    add_executable(
        asar-dll-test
        
        "test.cpp"
        
        "../asar-dll-bindings/asardll.c"
        "../asar-dll-bindings/asardll.h"
    )

    target_include_directories(asar-dll-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/")
    target_compile_definitions(asar-dll-test PRIVATE ASAR_TEST_DLL)

    set_asar_test_shared_properties(asar-dll-test)
endif()