cmake_minimum_required(VERSION 3.9.0)

# Without this, CMake will just throw a warning and add an invisible "project(Project)".
project(asar-all)

add_subdirectory(asar)

add_subdirectory(asar-tests)
