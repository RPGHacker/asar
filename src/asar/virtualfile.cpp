#include "virtualfile.h"
#include "platform/file-helpers.h"



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

	virtual_file_error open(const char* path, const char* base_path, const autoarray<string>& include_paths)
	{
		string path_to_use = create_final_path(path, base_path, include_paths);

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
	friend class virtual_filesystem;

	static string create_final_path(const char* path, const char* base_path, const autoarray<string>& include_paths)
	{
		string path_to_use = "";
		string test_path = "";

		test_path = normalize_path(path);

		// First check if path is absolute
		if (path_is_absolute(test_path))
		{
			if (file_exists(test_path))
			{
				path_to_use = test_path;
			}
		}
		else
		{
			// Now check if path exists relative to the base path
			if (base_path != nullptr)
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

		return path_to_use;
	}

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

	virtual_file_type vft = get_file_type_from_path(path);

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

			virtual_file_error error = new_file->open(path, base_path, m_include_paths);

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
			if(m_memory_files.exists(path)) {
				memory_buffer mem_buf = m_memory_files.find(path);
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
	m_memory_files.remove(name);
	m_memory_files.create(name) = mem_buf;
	
}

bool virtual_filesystem::is_path_absolute(const char* path)
{
	virtual_file_type vft = get_file_type_from_path(path);

	switch (vft)
	{
	case vft_memory_file:
		return true;

	case vft_physical_file:
		return path_is_absolute(path);

	default:
		// We should not get here
		return true;
	}
}

string virtual_filesystem::create_absolute_path(const char* base, const char* target)
{
	if (is_path_absolute(target) || base == nullptr || base[0] == '\0')
	{
		return target;
	}

	virtual_file_type base_type = get_file_type_from_path(base);

	switch (base_type)
	{
	case vft_physical_file:
		return physical_file::create_final_path(target, base, m_include_paths);

	case vft_memory_file:
		{
			// TODO
			return "";
		}

	default:
		// We should not get here
		return "";
	}
}
