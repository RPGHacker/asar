#if !defined(ASAR_VIRTUALFILE_H)
#define ASAR_VIRTUALFILE_H

#include "autoarray.h"
#include "libstr.h"

// RPG Hacker: A virtual file system which can work with physical files
// as well as (eventually) in-memory files. 

typedef void* virtual_file_handle;
static const virtual_file_handle INVALID_VIRTUAL_FILE_HANDLE = nullptr;

enum virtual_file_error
{
	vfe_none,

	vfe_doesnt_exist,
	vfe_access_denied,
	vfe_unknown,

	vfe_num_errors
};

class virtual_filesystem
{
public:
	void initialize(const char** include_paths, size_t num_include_paths);
	void destroy();

	virtual_file_handle open_file(const char* path, const char* base_path);
	void close_file(virtual_file_handle file_handle);

	size_t read_file(virtual_file_handle file_handle, void* out_buffer, size_t pos, size_t num_bytes);

	size_t get_file_size(virtual_file_handle file_handle);

	inline virtual_file_error get_last_error()
	{
		return m_last_error;
	}

private:
	autoarray<string> m_include_paths;
	virtual_file_error m_last_error;
};

#endif
