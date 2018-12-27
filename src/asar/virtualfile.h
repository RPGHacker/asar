#if !defined(ASAR_VIRTUALFILE_H)
#define ASAR_VIRTUALFILE_H

#include "autoarray.h"
#include "assocarr.h"
#include "libstr.h"

// RPG Hacker: A virtual file system which can work with physical files
// as well as in-memory files.

typedef void* virtual_file_handle;
static const virtual_file_handle INVALID_VIRTUAL_FILE_HANDLE = nullptr;

enum virtual_file_error
{
	vfe_none,

	vfe_doesnt_exist,
	vfe_access_denied,
	vfe_not_regular_file,
	vfe_unknown,

	vfe_num_errors
};

struct memory_buffer
{
    const void* data;
    size_t length;
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

	bool is_path_absolute(const char* path);

	string create_absolute_path(const char* base, const char* target);

	void add_memory_file(const char* name, const void* buffer, size_t length);

	inline virtual_file_error get_last_error()
	{
		return m_last_error;
	}

private:
	enum virtual_file_type
	{
		vft_physical_file,
		vft_memory_file
	};

	virtual_file_type get_file_type_from_path(const char* path);

	assocarr<memory_buffer> m_memory_files;
	autoarray<string> m_include_paths;
	virtual_file_error m_last_error;
};

#endif
