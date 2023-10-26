// RPG Hacker: We can't include that here, because it leads to crazy
// errors in windows.h - probably related to our string class?!?
//#include "platform/file-helpers.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 4987)
#endif

#define NOMINMAX
#include <windows.h>
#include <ctype.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#include <ctype.h>

#include "../file-helpers.h"
#include "../../unicode.h"

bool file_exists(const char * filename)
{
	std::wstring u16_str;
	if (!utf8_to_utf16(&u16_str, filename)) return false;
	return (GetFileAttributesW(u16_str.c_str()) != INVALID_FILE_ATTRIBUTES);
}

bool path_is_absolute(const char* path)
{
	return ((isalpha(path[0]) &&
		(':' == path[1]) && (('\\' == path[2]) || ('/' == path[2])))	/* drive letter + root, e.g. C:\dir or C:/dir */
		|| ('\\' == path[0]) || ('/' == path[0]));						/* just root, e.g. \dir or /dir */
}

char get_native_path_separator()
{
	return '\\';
}

bool check_is_regular_file(const char* path)
{
	std::wstring u16_str;
	if (!utf8_to_utf16(&u16_str, path)) return false;
	return !(GetFileAttributesW(u16_str.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
}


FileHandleType open_file(const char* path, FileOpenMode mode, FileOpenError* error)
{
	HANDLE out_handle = NULL;
	DWORD access_mode = 0;
	DWORD share_mode = 0;
	DWORD disposition = 0;

	switch (mode)
	{
	case FileOpenMode_ReadWrite:
		access_mode = GENERIC_READ | GENERIC_WRITE;
		disposition = OPEN_EXISTING;
		break;
	case FileOpenMode_Read:
		access_mode = GENERIC_READ;
		// This should be fine, right?
		share_mode = FILE_SHARE_READ;
		disposition = OPEN_EXISTING;
		break;
	case FileOpenMode_Write:
		access_mode = GENERIC_WRITE;
		disposition = CREATE_ALWAYS;
		break;
	}

	std::wstring u16_str;
	if (!utf8_to_utf16(&u16_str, path))
	{
		// RPG Hacker: I treat encoding error as "file not found", which I guess is what
		// Windows would do, anyways.
		*error = FileOpenError_NotFound;
		return INVALID_HANDLE_VALUE;
	}

	out_handle = CreateFileW(u16_str.c_str(), access_mode, share_mode, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);

	if (error != NULL)
	{
		if (out_handle != INVALID_HANDLE_VALUE)
		{
			*error = FileOpenError_None;
		}
		else
		{
			DWORD win_error = GetLastError();

			switch (win_error)
			{
			case ERROR_FILE_NOT_FOUND:
				*error = FileOpenError_NotFound;
				break;
			case ERROR_ACCESS_DENIED:
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

	CloseHandle(handle);
}

uint64_t get_file_size(FileHandleType handle)
{
	if (handle == InvalidFileHandle) return 0u;

	LARGE_INTEGER f_size;

	if (!GetFileSizeEx(handle, &f_size)) return 0u;

	return (uint64_t)f_size.QuadPart;
}

void set_file_pos(FileHandleType handle, uint64_t pos)
{
	if (handle == InvalidFileHandle) return;

	// TODO: Some error handling would be wise here.

	LARGE_INTEGER new_pos;
	new_pos.QuadPart = (LONGLONG)pos;

	SetFilePointerEx(handle, new_pos, NULL, FILE_BEGIN);
}

uint32_t read_file(FileHandleType handle, void* buffer, uint32_t num_bytes)
{
	if (handle == InvalidFileHandle) return 0u;

	DWORD bytes_read = 0u;

	// TODO: Some error handling would be wise here.

	ReadFile(handle, buffer, (DWORD)num_bytes, &bytes_read, NULL);

	return (uint32_t)bytes_read;
}

uint32_t write_file(FileHandleType handle, const void* buffer, uint32_t num_bytes)
{
	if (handle == InvalidFileHandle) return 0u;

	DWORD bytes_written = 0u;

	// TODO: Some error handling would be wise here.

	WriteFile(handle, buffer, (DWORD)num_bytes, &bytes_written, NULL);

	return (uint32_t)bytes_written;
}
