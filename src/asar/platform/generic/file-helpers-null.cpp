#include "platform/file-helpers.h"

// "filesystem backend" which pretends there are no files

bool file_exists(const char * filename)
{
	return false;
}

bool path_is_absolute(const char* path)
{
	return ('/' == path[0]);
}

char get_native_path_separator()
{
	return '/';
}

bool check_is_regular_file(const char* path)
{
	return false;
}


FileHandleType open_file(const char* path, FileOpenMode mode, FileOpenError* error)
{
	*error = FileOpenError_NotFound;
	return NULL;
}

void close_file(FileHandleType handle)
{
}

uint64_t get_file_size(FileHandleType handle)
{
	return 0u;
}

void set_file_pos(FileHandleType handle, uint64_t pos)
{
}

uint32_t read_file(FileHandleType handle, void* buffer, uint32_t num_bytes)
{
	return 0u;
}

uint32_t write_file(FileHandleType handle, const void* buffer, uint32_t num_bytes)
{
	return 0u;
}
