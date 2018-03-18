#include "virtualfile.hpp"
#include "platform/file-helpers.h"

enum virtual_file_type
{
	vft_physical_file,
	vft_memory_file,

	vft_num_file_types
};



class virtual_file
{
public:
	virtual ~virtual_file()
	{
	}

	virtual virtual_file_type get_type() = 0;

	virtual void close() = 0;

	virtual size_t read(void* out_buffer, size_t pos, size_t num_bytes) = 0;

	virtual size_t get_size() = 0;
};



class physical_file : public virtual_file
{
public:
	physical_file()
		: m_file_handle(nullptr)
	{		
	}

	virtual ~physical_file()
	{
		close();
	}
	
	virtual virtual_file_type get_type()
	{
		return vft_physical_file;
	}

	virtual_file_error open(const char* path, const char* base_path, const autoarray<string>& include_paths)
	{
		string path_to_use = "";

		// First check if path is absolute
		if (path_is_absolute(path))
		{
			if (file_exists(path))
			{
				path_to_use = path;
			}
		}
		else
		{
			// Now check if path exists relative to the base path
			string test_path = "";

			if (base_path != nullptr && base_path[0] != '\0')
			{
				test_path = create_combined_path(dir(base_path), path);
			}

			if (test_path != "" && file_exists(test_path))
			{
				path_to_use = test_path;
			}
			else
			{
				// Finally check if path exists relative to any include path
				for (int i = 0; i < include_paths.count; ++i)
				{
					test_path = create_combined_path(include_paths[i], path);

					if (file_exists(test_path))
					{
						path_to_use = test_path;
						break;
					}
				}
			}
		}

		if (path_to_use != "")
		{
			m_file_handle = fopen((const char*)path_to_use, "rb");

			if (m_file_handle == nullptr)
			{
				return vfe_access_denied;
			}

			return vfe_none;
		}

		return vfe_doesnt_exist;
	}

	virtual void close()
	{
		if (m_file_handle != nullptr)
		{
			fclose(m_file_handle);
			m_file_handle = nullptr;
		}
	}

	virtual size_t read(void* out_buffer, size_t pos, size_t num_bytes)
	{
		fseek(m_file_handle, (long)pos, SEEK_SET);
		return fread(out_buffer, 1, num_bytes, m_file_handle);
	}

	virtual size_t get_size()
	{
		fseek(m_file_handle, 0, SEEK_END);
		long filepos = ftell(m_file_handle);
		fseek(m_file_handle, 0, SEEK_SET);

		return (size_t)filepos;
	}

private:
	FILE* m_file_handle;
};



void virtual_filesystem::initialize(const char** include_paths, size_t num_include_paths)
{
	m_include_paths.reset();

	for (size_t i = 0; i < num_include_paths; ++i)
	{
		m_include_paths[(int)i] = include_paths[i];
	}

	m_last_error = vfe_none;
}

void virtual_filesystem::destroy()
{
	m_include_paths.reset();
}


virtual_file_handle virtual_filesystem::open_file(const char* path, const char* base_path)
{
	m_last_error = vfe_none;

	// TODO: Add support for in-memory files here later

	if (true)
	{
		physical_file* new_file = new physical_file;

		if (new_file == nullptr)
		{
			m_last_error = vfe_unknown;
			return INVALID_VIRTUAL_FILE_HANDLE;
		}

		virtual_file_error error = new_file->open(path, base_path, m_include_paths);

		if (error != vfe_none)
		{
			delete new_file;
			m_last_error = error;
			return INVALID_VIRTUAL_FILE_HANDLE;
		}

		return static_cast<virtual_file_handle>(new_file);
	}
	else
	{
		// ...
	}

	m_last_error = vfe_unknown;
	return INVALID_VIRTUAL_FILE_HANDLE;
}

void virtual_filesystem::close_file(virtual_file_handle file_handle)
{
	if (file_handle != INVALID_VIRTUAL_FILE_HANDLE)
	{
		virtual_file* file = static_cast<virtual_file*>(file_handle);

		file->close();

		delete file;
	}
}



size_t virtual_filesystem::read_file(virtual_file_handle file_handle, void* out_buffer, size_t pos, size_t num_bytes)
{
	if (file_handle != INVALID_VIRTUAL_FILE_HANDLE)
	{
		virtual_file* file = static_cast<virtual_file*>(file_handle);

		return file->read(out_buffer, pos, num_bytes);
	}

	return 0u;
}

size_t virtual_filesystem::get_file_size(virtual_file_handle file_handle)
{
	if (file_handle != INVALID_VIRTUAL_FILE_HANDLE)
	{
		virtual_file* file = static_cast<virtual_file*>(file_handle);

		return file->get_size();
	}

	return 0u;
}
