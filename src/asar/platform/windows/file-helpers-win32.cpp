// RPG Hacker: We can't include that here, because it leads to crazy
// errors in windows.h - probably related to our string class?!?
//#include "platform/file-helpers.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 4987)
#endif

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
	return (GetFileAttributesA(filename) != INVALID_FILE_ATTRIBUTES);
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
	return !(GetFileAttributesA(path) & FILE_ATTRIBUTE_DIRECTORY);
}


FileHandleType open_file(const char* path, FileOpenMode mode)
{
	HANDLE out_handle = NULL;
	DWORD access_mode = 0;
	DWORD share_mode = 0;
	DWORD disposition = 0;

	if (mode == FileOpenMode_ReadWrite)
	{
		access_mode = GENERIC_READ | GENERIC_WRITE;
		disposition = OPEN_EXISTING;
	}
	else if (mode == FileOpenMode_Read)
	{
		access_mode = GENERIC_READ;
		// This should be fine, right?
		share_mode = FILE_SHARE_READ;
		disposition = OPEN_EXISTING;
	}	
	else if (mode == FileOpenMode_Write)
	{
		access_mode = GENERIC_WRITE;
		disposition = CREATE_ALWAYS;
	}

	out_handle = CreateFileA(path, access_mode, share_mode, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);

	return out_handle;
}

void close_file(FileHandleType handle)
{
	if (handle == InvalidFileHandle) return;

	CloseHandle(handle);
}

uint64_t get_file_size(FileHandleType handle)
{
	if (handle == InvalidFileHandle) 0u;

	LARGE_INTEGER f_size;

	if (!GetFileSizeEx(handle, &f_size)) return 0u;

	return (uint64_t)f_size.QuadPart;
}

void set_file_pos(FileHandleType handle, uint64_t pos)
{
	if (handle == InvalidFileHandle) 0u;

	// TODO: Some error handling would be wise here.

	LARGE_INTEGER new_pos;
	new_pos.QuadPart = (LONGLONG)pos;

	SetFilePointerEx(handle, new_pos, NULL, FILE_BEGIN);
}

uint64_t get_file_pos(FileHandleType handle)
{
	if (handle == InvalidFileHandle) 0u;

	// TODO: Some error handling would be wise here.

	LARGE_INTEGER no_move;
	no_move.QuadPart = 0u;

	LARGE_INTEGER f_pos;
	f_pos.QuadPart = 0u;

	SetFilePointerEx(handle, no_move, &f_pos, FILE_CURRENT);

	return (uint64_t)f_pos.QuadPart;
}

uint32_t read_file(FileHandleType handle, void* buffer, uint32_t numBytes)
{
	if (handle == InvalidFileHandle) 0u;

	DWORD bytes_read = 0u;

	// TODO: Some error handling would be wise here.

	ReadFile(handle, buffer, (DWORD)numBytes, &bytes_read, NULL);

	return (uint32_t)bytes_read;
}

uint32_t write_file(FileHandleType handle, const void* buffer, uint32_t numBytes)
{
	if (handle == InvalidFileHandle) 0u;
	
	DWORD bytes_written = 0u;

	// TODO: Some error handling would be wise here.

	WriteFile(handle, buffer, (DWORD)numBytes, &bytes_written, NULL);

	return (uint32_t)bytes_written;
}
