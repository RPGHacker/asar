#-----------------------------------------------------------
# mingw toolchain common
#-----------------------------------------------------------

set(MINGW ON)
set(WIN32 ON)
set(CMAKE_SYSTEM_NAME Windows)
set(TOOL_PREFIX "${MINGW_TYPE}-")

if(CLANG)
        set(CMAKE_C_COMPILER "${TOOL_PREFIX}clang")
        set(CMAKE_CXX_COMPILER "${TOOL_PREFIX}clang++")
else(CLANG)
        set(CMAKE_C_COMPILER "${TOOL_PREFIX}gcc")
        set(CMAKE_CXX_COMPILER "${TOOL_PREFIX}g++")
endif(CLANG)

set(CMAKE_RC_COMPILER ${TOOL_PREFIX}windres)
set(CMAKE_RC_COMPILE_OBJECT 
	"<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCES> -o <OBJECT>")
#set(CMAKE_LINKER "${TOOL_PREFIX}g++")
#set(CMAKE_AR "${TOOL_PREFIX}ar")

set(CMAKE_FIND_ROOT_PATH "/usr/${MINGW_TYPE}/")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

