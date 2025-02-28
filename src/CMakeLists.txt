cmake_minimum_required(VERSION 3.13.0...3.31.0)

# allows changing the msvc runtime library i think?
cmake_policy(SET CMP0091 NEW)

project(asar-all)

# here because the DLL test needs to know its value
option(ASAR_USE_SANITIZER "Build Asar with ASan and UBSan" OFF)

add_subdirectory(asar)

add_subdirectory(asar-tests)
