cmake_minimum_required(VERSION 3.9.0)
cmake_policy(SET CMP0091 NEW)

project(Asar)

option(ASAR_USE_SANITIZER "Build Asar with ASan" OFF)

add_subdirectory(asar)

add_subdirectory(asar-tests)
