cmake_minimum_required(VERSION 3.9.0)

option(ASAR_GEN_TEST "Generate tests" ON)

add_subdirectory(asar)

if(ASAR_GEN_TEST)
	add_subdirectory(asar-tests)
endif(ASAR_GEN_TEST)
