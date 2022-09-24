#pragma once

#include <stdint.h>

#include "libstr.h"

// Functions in this file expect UTF-8 file paths.

#if defined(windows)
// RPG Hacker: These should be HANDLE rather than void*, but I don't feel like
// including Windows.h in here, just for these typedefs.
typedef void* FileHandleType;
const FileHandleType InvalidFileHandle = (void*)(size_t)-1;
#elif defined(linux)
typedef FILE* FileHandleType;
const FileHandleType InvalidFileHandle = nullptr;
#else
typedef FILE* FileHandleType;
const FileHandleType InvalidFileHandle = nullptr;
#endif

// RPG Hacker: Read/write is currently only used for handling the ROM.
// I could have made this a bit-mask, but since we only have these three modes,
// I think this is the nicer-looking solution.
enum FileOpenMode {
    FileOpenMode_Read,
    FileOpenMode_Write,
    FileOpenMode_ReadWrite,
};

enum FileOpenError {
    FileOpenError_None,
    FileOpenError_NotFound,
    FileOpenError_AccessDenied,
    FileOpenError_Unknown,
};

bool file_exists(const char* filename);
bool path_is_absolute(const char* path);
char get_native_path_separator();
bool check_is_regular_file(const char* path);

string dir(char const* name);
string create_combined_path(const char* path1, const char* path2);
string normalize_path(const char* path);
string get_base_name(char const* name);

FileHandleType open_file(const char* path, FileOpenMode mode,
                         FileOpenError* error = nullptr);
void close_file(FileHandleType handle);
uint64_t get_file_size(FileHandleType handle);
void set_file_pos(FileHandleType handle, uint64_t pos);
uint32_t read_file(FileHandleType handle, void* buffer, uint32_t num_bytes);
uint32_t write_file(FileHandleType handle, const void* buffer, uint32_t num_bytes);
