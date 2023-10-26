#include "platform/file-helpers.h"
#include <stdio.h>

bool file_exists(const char * filename)
{
	FILE * f = fopen(filename, "rb");
	if (f) fclose(f);
	return f;
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
	// no really universal way to check this
	return true;
}


FileHandleType open_file(const char* path, FileOpenMode mode, FileOpenError* error)
{
	FILE* out_handle = NULL;
	const char* open_mode;

	switch (mode)
	{
	case FileOpenMode_ReadWrite:
		open_mode = "r+b";
		break;
	case FileOpenMode_Read:
		open_mode = "rb";
		break;
	case FileOpenMode_Write:
		open_mode = "wb";
		break;
	}

	out_handle = fopen(path, open_mode);

	if (error != NULL)
	{
		if (out_handle != NULL)
		{
			*error = FileOpenError_None;
		}
		else
		{
			int fopen_error = errno;

			switch (fopen_error)
			{
			case ENOENT:
				*error = FileOpenError_NotFound;
				break;
			case EACCES:
				*error = FileOpenError_AccessDenied;
				break;
			default:
				*error = FileOpenError_Unknown;
				break;
			}
		}
	}

	return out_handle;
}

void close_file(FileHandleType handle)
{
	if (handle == InvalidFileHandle) return;

	fclose(handle);
}

uint64_t get_file_size(FileHandleType handle)
{
	if (handle == InvalidFileHandle) return 0u;

	long int f_pos = ftell(handle);
	fseek(handle, 0, SEEK_END);
	long int f_size = ftell(handle);
	fseek(handle, f_pos, SEEK_SET);

	return (uint64_t)f_size;
}

void set_file_pos(FileHandleType handle, uint64_t pos)
{
	if (handle == InvalidFileHandle) return;

	// TODO: Some error handling would be wise here.

	fseek(handle, pos, SEEK_SET);
}

uint32_t read_file(FileHandleType handle, void* buffer, uint32_t num_bytes)
{
	if (handle == InvalidFileHandle) return 0u;

	// TODO: Some error handling would be wise here.

	return (uint32_t)fread(buffer, 1, num_bytes, handle);
}

uint32_t write_file(FileHandleType handle, const void* buffer, uint32_t num_bytes)
{
	if (handle == InvalidFileHandle) return 0u;

	// TODO: Some error handling would be wise here.

	return (uint32_t)fwrite(buffer, 1, num_bytes, handle);
}
