
# Asar DLL text application build rules

cmake_minimum_required(VERSION 3.9.0)

include("${CMAKE_CURRENT_SOURCE_DIR}/../asar-tests-shared/AsarTestShared.cmake")

add_executable(
	asar-dll-test
	
	"${CMAKE_CURRENT_SOURCE_DIR}/../asar-tests-shared/test.cpp"
	
	"${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/asardll.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/asardll.h"
)

target_include_directories(asar-dll-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../asar-dll-bindings/")

set_asar_test_shared_properties(asar-dll-test)
add_definitions( -DASAR_TEST_DLL )