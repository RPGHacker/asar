#include <errno.h>
#include "virtualfile.h"
#include "platform/file-helpers.h"
#include "warnings.h"



class virtual_file
{
public:
	virtual ~virtual_file()
	{
	}

	virtual void close() = 0;

	virtual size_t read(void* out_buffer, size_t pos, size_t num_bytes) = 0;

	virtual size_t get_size() = 0;
};

class memory_file : public virtual_file
{
public:
	memory_file(const void* data, size_t length)
		: m_data(data), m_length(length)
	{		
	}

	virtual ~memory_file()
	{
		close();
	}

	virtual void close()
	{
	}

	virtual size_t read(void* out_buffer, size_t pos, size_t num_bytes)
	{
		if(pos > m_length) return 0;

		int diff = (int)(pos + num_bytes) - (int)m_length;
		num_bytes -= diff < 0 ? 0 : (unsigned int)diff;

		memcpy(out_buffer, (const char*)m_data + pos, num_bytes);
		return num_bytes;
	}

	virtual size_t get_size()
	{
		return m_length;
	}

private:
	const void* m_data;
	size_t m_length;
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

	virtual_file_error open(const string& path)
	{
		if (path != "")
		{
			// randomdude999: checking this before file regularity to improve error messages
			if(!file_exists((const char*)path)) return vfe_doesnt_exist;
			if(!check_is_regular_file((const char*)path)) return vfe_not_regular_file;

			m_file_handle = fopen((const char*)path, "rb");

			if (m_file_handle == nullptr)
			{
				if (errno == ENOENT)
				{
					return vfe_doesnt_exist;
				}
				else if (errno == EACCES)
				{
					return vfe_access_denied;
				}
				else
				{
					return vfe_unknown;
				}
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
	friend class virtual_filesystem;

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
	m_memory_files.reset();
}

void virtual_filesystem::destroy()
{
	m_include_paths.reset();
}


virtual_file_handle virtual_filesystem::open_file(const char* path, const char* base_path)
{
	m_last_error = vfe_none;

	string absolutepath = create_absolute_path(base_path, path);

	virtual_file_type vft = get_file_type_from_path(absolutepath);

	if (vft != vft_memory_file)
	{
		asar_throw_warning(0, warning_id_check_memory_file, path, (int)warning_id_check_memory_file);
	}

	switch (vft)
	{
		case vft_physical_file:
		{
			physical_file* new_file = new physical_file;

			if (new_file == nullptr)
			{
				m_last_error = vfe_unknown;
				return INVALID_VIRTUAL_FILE_HANDLE;
			}

			virtual_file_error error = new_file->open(absolutepath);

			if (error != vfe_none)
			{
				delete new_file;
				m_last_error = error;
				return INVALID_VIRTUAL_FILE_HANDLE;
			}

			return static_cast<virtual_file_handle>(new_file);
		}

		case vft_memory_file:
		{
			if(m_memory_files.exists(absolutepath)) {
				memory_buffer mem_buf = m_memory_files.find(absolutepath);
				memory_file* new_file = new memory_file(mem_buf.data, mem_buf.length);
				return static_cast<virtual_file_handle>(new_file);
			} else {
				m_last_error =	vfe_doesnt_exist;
				return INVALID_VIRTUAL_FILE_HANDLE;
			}
		}

		default:
			// We should not get here
			m_last_error = vfe_unknown;
			return INVALID_VIRTUAL_FILE_HANDLE;
	}
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


virtual_filesystem::virtual_file_type virtual_filesystem::get_file_type_from_path(const char* path)
{
	if(m_memory_files.exists(path)) {
		return vft_memory_file;
	} else {
		return vft_physical_file;
	}
}

void virtual_filesystem::add_memory_file(const char* name, const void* buffer, size_t length) {
	memory_buffer mem_buf = { buffer, length };
	string normalized_path = normalize_path(name);
	m_memory_files.remove(normalized_path);
	m_memory_files.create(normalized_path) = mem_buf;
	
}

bool virtual_filesystem::is_path_absolute(const char* path)
{
	return path_is_absolute(path);
}

string virtual_filesystem::create_absolute_path(const char* base, const char* target)
{
	if (is_path_absolute(target) || base == nullptr || base[0] == '\0')
	{
		return normalize_path(target);
	}

	string path_to_use = "";
	string test_path = "";

	test_path = normalize_path(target);

	// First check if path is absolute
	if (path_is_absolute(test_path))
	{
		if (m_memory_files.exists(test_path) || file_exists(test_path))
		{
			path_to_use = test_path;
		}
	}
	else
	{
		// Now check if path exists relative to the base path
		if (base != nullptr)
		{
			test_path = create_combined_path(dir(base), target);
		}

		if (test_path != "" && (m_memory_files.exists(test_path) || file_exists(test_path)))
		{
			path_to_use = test_path;
		}
		else
		{
			// Finally check if path exists relative to any include path
			bool found = false;
			for (int i = 0; i < m_include_paths.count; ++i)
			{
				test_path = create_combined_path(m_include_paths[i], target);

				if (m_memory_files.exists(test_path) || file_exists(test_path))
				{
					found = true;
					path_to_use = test_path;
					break;
				}
			}

			if (!found)
			{
				// Reset our path so that we don't return an empty one
				// (that will do some weird shit and fuck up error messages)
				path_to_use = target;
			}
		}
	}

	return path_to_use;
}
