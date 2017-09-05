
# Asar testing utility build rules

cmake_minimum_required(VERSION 3.9.0)

include("${CMAKE_CURRENT_SOURCE_DIR}/../asar-tests-shared/AsarTestShared.cmake")

add_executable(
	asar-app-test
	
	"${CMAKE_CURRENT_SOURCE_DIR}/../asar-tests-shared/test.cpp"
)

set_asar_test_shared_properties(asar-app-test)