

#if defined(_WIN32)
#	define NOMINMAX

#	if defined(_MSC_VER)
#		pragma warning(push)
#		pragma warning(disable : 4365)
#		pragma warning(disable : 4571)
#		pragma warning(disable : 4623)
#		pragma warning(disable : 4625)
#		pragma warning(disable : 4626)
#		pragma warning(disable : 4668)
#		pragma warning(disable : 4711)
#		pragma warning(disable : 4774)
#		pragma warning(disable : 4987)
#		pragma warning(disable : 5026)
#		pragma warning(disable : 5027)
#	endif

#	include <stdio.h>
#	include <string.h>
#	include <stdlib.h>
#	include <sys/stat.h>
#	include <windows.h>
#	include <vector>
#	include <string>
#	include <algorithm>
#	include <set>
#	include <list>

#	if defined(_MSC_VER)
#		pragma warning(pop)
#	endif
#else
#	include <dirent.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <vector>
#	include <string>
#	include <string.h>
#	include <algorithm>
#	include <sys/stat.h>
#	include <set>
#	include <list>
#endif

#if defined(ASAR_TEST_DLL)
#	include "asardll.h"
#endif

#define die() { numfailed++; free(expectedrom); free(truerom); printf("Failure!\n\n"); continue; }
#define dief(...) { printf(__VA_ARGS__); die(); }

#define min(a, b) ((a)<(b)?(a):(b))

static const size_t max_rom_size = 16777216u;


#if defined(_WIN32)

std::string utf16_to_utf8(const std::wstring input)
{
	int needed_chars = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), input.length(), NULL, 0, NULL, NULL);

	if (needed_chars == 0)
	{
		DWORD err_code = GetLastError();
		HRESULT hresult = HRESULT_FROM_WIN32(err_code);
		fprintf(stderr, "WideCharToMultiByte() (first call) failed with error code: %u (HRESULT: 0x%08x)\n",
			(unsigned int)err_code, (unsigned int)hresult);
		return "";
	}

	std::string out;
	out.resize(needed_chars);

	int ret_val = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), input.length(), const_cast<char*>(out.c_str()), out.length(), NULL, NULL);

	if (ret_val == 0)
	{
		DWORD err_code = GetLastError();
		HRESULT hresult = HRESULT_FROM_WIN32(err_code);
		fprintf(stderr, "WideCharToMultiByte() (second call) failed with error code: %u (HRESULT: 0x%08x)\n",
			(unsigned int)err_code, (unsigned int)hresult);
		return "";
	}

	return out;
}


std::wstring utf8_to_utf16(const std::string input)
{
	int needed_chars = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), input.length(), NULL, 0);

	if (needed_chars == 0)
	{
		DWORD err_code = GetLastError();
		HRESULT hresult = HRESULT_FROM_WIN32(err_code);
		fprintf(stderr, "MultiByteToWideChar() (first call) failed for string \"%s\" with error code: %u (HRESULT: 0x%08x)\n",
			input.c_str(), (unsigned int)err_code, (unsigned int)hresult);
		return L"";
	}

	std::wstring out;
	out.resize(needed_chars);

	int ret_val = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), input.length(), const_cast<wchar_t*>(out.c_str()), out.length());

	if (ret_val == 0)
	{
		DWORD err_code = GetLastError();
		HRESULT hresult = HRESULT_FROM_WIN32(err_code);
		fprintf(stderr, "MultiByteToWideChar() (second call) failed for string \"%s\" with error code: %u (HRESULT: 0x%08x)\n",
			input.c_str(), (unsigned int)err_code, (unsigned int)hresult);
		return L"";
	}

	return out;
}

inline bool file_exists(const char *filename)
{
	std::wstring filename_u16 = utf8_to_utf16(filename);
	return (GetFileAttributesW(filename_u16.c_str()) != INVALID_FILE_ATTRIBUTES);
}


std::vector<char> read_file_into_buffer(const char *filename)
{
	std::wstring filename_u16 = utf8_to_utf16(filename);

	HANDLE handle = CreateFileW(filename_u16.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	std::vector<char> out;

	if (handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER f_size;

		GetFileSizeEx(handle, &f_size);

		out.resize((size_t)f_size.QuadPart);

		ReadFile(handle, out.data(), out.size(), NULL, NULL);

		CloseHandle(handle);
	}

	return out;
}

void write_buffer_to_file(const char *filename, const void* data, size_t data_size)
{
	std::wstring filename_u16 = utf8_to_utf16(filename);

	HANDLE handle = CreateFileW(filename_u16.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle != INVALID_HANDLE_VALUE)
	{
		WriteFile(handle, data, data_size, NULL, NULL);

		CloseHandle(handle);
	}
}

#else

inline bool file_exists(const char *filename)
{
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}


std::vector<char> read_file_into_buffer(const char *filename)
{
	FILE* handle = fopen(filename, "rb");

	std::vector<char> out;

	if (handle != nullptr)
	{
		fseek(handle, 0, SEEK_END);
		long int f_size = ftell(handle);
		fseek(handle, 0, SEEK_SET);

		out.resize((size_t)f_size);

		fread(out.data(), 1, out.size(), handle);

		fclose(handle);
	}

	return out;
}

void write_buffer_to_file(const char *filename, const void* data, size_t data_size)
{
	FILE* handle = fopen(filename, "wb");

	if (handle != nullptr)
	{
		fwrite(data, 1, data_size, handle);

		fclose(handle);
	}
}

#endif

void write_buffer_to_file(const char *filename, const std::vector<char>& to_write)
{
	write_buffer_to_file(filename, to_write.data(), to_write.size());
}

void write_buffer_to_file(const char *filename, const std::string& to_write)
{
	write_buffer_to_file(filename, to_write.c_str(), to_write.length());
}


// This function is based in part on nall, which is under the following licence.
// This modified version is licenced under the LGPL version 3 or later. See the LICENSE file
// for details.
//
//   Copyright (c) 2006-2015  byuu
//
// Permission to use, copy, modify, and/or distribute this software for
// any purpose with or without fee is hereby granted, provided that the
// above copyright notice and this permission notice appear in all
// copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
// DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
// PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
// TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
static std::string dir(char const *name)
{
	std::string result = name;
	for (signed i = (int)result.length(); i >= 0; i--)
	{
		if (result[(size_t)i] == '/' || result[(size_t)i] == '\\')
		{
			result.erase((size_t)(i + 1));
			break;
		}
		if (i == 0) result = "";
	}
	return result;
}


inline bool str_ends_with(const char * str, const char * suffix)
{
	if (str == nullptr || suffix == nullptr)
		return false;

	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);

	if (suffix_len > str_len)
		return false;

	return (strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0);
}


struct wrapped_file
{
	char file_name[64];
	char file_path[512];
};


static bool find_files_in_directory(std::vector<wrapped_file>& out_array, const char * directory_name)
{
#if defined(_WIN32)

	char search_path[512];
	bool has_path_seperator = false;
	if (str_ends_with(directory_name, "/") || str_ends_with(directory_name, "\\"))
	{
		snprintf(search_path, sizeof(search_path), "%s*.*", directory_name);
		has_path_seperator = true;
	}
	else
	{
		snprintf(search_path, sizeof(search_path), "%s/*.*", directory_name);
	}

	std::wstring search_path_w = utf8_to_utf16(search_path);

	WIN32_FIND_DATAW fd;
	HANDLE hFind = ::FindFirstFileW(search_path_w.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::string filename_u8 = utf16_to_utf8(fd.cFileName);

				if (str_ends_with(filename_u8.c_str(), ".asm"))
				{
					wrapped_file new_file;
					strncpy(new_file.file_name, filename_u8.c_str(), sizeof(new_file.file_name));
					new_file.file_name[sizeof(new_file.file_name) - 1] = '\0';
					strncpy(new_file.file_path, directory_name, sizeof(new_file.file_path));
					if (!has_path_seperator)
					{
#if defined(__MINGW32__)
						strncat(new_file.file_path, "/", sizeof(new_file.file_path) - strlen(new_file.file_path) - 1);
#else
						strcat_s(new_file.file_path, sizeof(new_file.file_path), "/");
#endif
					}
#if defined(__MINGW32__)
					strncat(new_file.file_path, filename_u8.c_str(), sizeof(new_file.file_path) - strlen(new_file.file_path) - 1);
#else
					strcat_s(new_file.file_path, sizeof(new_file.file_path), filename_u8.c_str());
#endif
					new_file.file_path[sizeof(new_file.file_path) - 1] = '\0';
					out_array.push_back(new_file);
				}
			}
		} while (::FindNextFileW(hFind, &fd));

		::FindClose(hFind);
	}
	else
	{
		DWORD err_code = GetLastError();
		HRESULT hresult = HRESULT_FROM_WIN32(err_code);
		fprintf(stderr, "FindFirstFileW() for path \"%s\" failed with error code: %u (HRESULT: 0x%08x)\n",
			search_path, (unsigned int)err_code, (unsigned int)hresult);
		return false;
	}

#else

	bool has_path_seperator = false;
	if (str_ends_with(directory_name, "/") || str_ends_with(directory_name, "\\"))
	{
		has_path_seperator = true;
	}
	DIR * dir;
	dirent * ent;
	if ((dir = opendir(directory_name)) != nullptr) {
		while ((ent = readdir(dir)) != nullptr) {
			// Only consider regular files
			if (ent->d_type == DT_REG)
			{
				if (str_ends_with(ent->d_name, ".asm"))
				{
					wrapped_file new_file;
					strncpy(new_file.file_name, ent->d_name, sizeof(new_file.file_name));
					new_file.file_name[sizeof(new_file.file_name) - 1] = '\0';
					strncpy(new_file.file_path, directory_name, sizeof(new_file.file_path));
					if (!has_path_seperator)
					{
						size_t currlen = strlen(new_file.file_path);
						if (currlen < sizeof(new_file.file_path))
						{
							strncpy(new_file.file_path + currlen, "/", sizeof(new_file.file_path) - currlen);
						}
					}
					size_t currlen = strlen(new_file.file_path);
					if (currlen < sizeof(new_file.file_path))
					{
						strncpy(new_file.file_path + currlen, ent->d_name, sizeof(new_file.file_path) - currlen);
					}
					new_file.file_path[sizeof(new_file.file_path) - 1] = '\0';
					out_array.push_back(new_file);
				}
			}
		}
		closedir(dir);
	}
	else {
		return false;
	}

#endif

	return true;
}


static void delete_file(const char * filename)
{
#if defined(_WIN32)

	DeleteFileA(filename);

#else

	remove(filename);

#endif
}


#if !defined(ASAR_TEST_DLL)
// RPG Hacker: Original test application used system(), but
// that really made my anti-virus sad, so here is a new,
// complicated platform-specific solution.
// NOTE: No cont char*, for commandline, because CreateProcess()
// actually can mess with this string for some reason...
static bool execute_command_line(char * commandline, const char * stdout_log_file, const char * stderr_log_file)
{
#if defined(_WIN32)

	HANDLE stdout_read_handle = nullptr;
	HANDLE stdout_write_handle = nullptr;
	HANDLE stderr_read_handle = nullptr;
	HANDLE stderr_write_handle = nullptr;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = nullptr;

	if (!CreatePipe(&stdout_read_handle, &stdout_write_handle, &sa, 0))
	{
		return false;
	}

	if (!SetHandleInformation(stdout_read_handle, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(stdout_read_handle);
		CloseHandle(stdout_write_handle);
		return false;
	}

	if (!CreatePipe(&stderr_read_handle, &stderr_write_handle, &sa, 0))
	{
		CloseHandle(stdout_read_handle);
		CloseHandle(stdout_write_handle);
		return false;
	}

	if (!SetHandleInformation(stderr_read_handle, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(stdout_read_handle);
		CloseHandle(stdout_write_handle);
		CloseHandle(stderr_read_handle);
		CloseHandle(stderr_write_handle);
		return false;
	}

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	si.hStdError = stderr_write_handle;
	si.hStdOutput = stdout_write_handle;
	si.dwFlags |= STARTF_USESTDHANDLES;

	std::wstring u16_commandline = utf8_to_utf16(commandline);

	if (!CreateProcessW(nullptr, const_cast<wchar_t*>(u16_commandline.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi))
	{
		printf("execute_command_line() failed with HRESULT: 0x%8x\n", (unsigned int)HRESULT_FROM_WIN32(GetLastError()));
		CloseHandle(stdout_read_handle);
		CloseHandle(stdout_write_handle);
		CloseHandle(stderr_read_handle);
		CloseHandle(stderr_write_handle);
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(stdout_write_handle);
	CloseHandle(stderr_write_handle);

	{
		std::vector<char> stdout_buffer;

		DWORD dwRead;
		CHAR chBuf[4096];
		BOOL bSuccess = FALSE;

		for (;;)
		{
			bSuccess = ReadFile(stdout_read_handle, chBuf, sizeof(chBuf), &dwRead, nullptr);
			if (bSuccess == FALSE || dwRead == 0) break;
			stdout_buffer.insert(stdout_buffer.end(), chBuf, chBuf + dwRead);
		}

		write_buffer_to_file(stdout_log_file, stdout_buffer);

		CloseHandle(stdout_read_handle);
	}

	{
		std::vector<char> stderr_buffer;

		DWORD dwRead;
		CHAR chBuf[4096];
		BOOL bSuccess = FALSE;

		for (;;)
		{
			bSuccess = ReadFile(stderr_read_handle, chBuf, sizeof(chBuf), &dwRead, nullptr);
			if (bSuccess == FALSE || dwRead == 0) break;
			stderr_buffer.insert(stderr_buffer.end(), chBuf, chBuf + dwRead);
		}

		write_buffer_to_file(stderr_log_file, stderr_buffer);

		CloseHandle(stderr_read_handle);
	}

#else

	// RPG Hacker: TODO: Needs support for splitting stderr and stdout.
	// Currently, I expect tests to just fail on Linux.

	fflush(stdout);

	std::string line = commandline;
	line += std::string(" 2>\"") + stderr_log_file + "\" 1>\"" + stdout_log_file + "\"";
	FILE * fp = popen(line.c_str(), "r");

	pclose(fp);

#endif

	return true;
}
#endif


static std::vector<std::string> tokenize_string(const char * str, const char * key)
{
	std::string s = str;
	std::string delimiter = key;

	size_t pos = 0;
	std::string token;
	std::vector<std::string> list;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		// Don't bother adding empty tokens (they're just whitespace)
		if (token != "")
		{
			list.push_back(token);
		}
		s.erase(0, pos + delimiter.length());
	}
	list.push_back(s);
	return list;
}

int main(int argc, char * argv[])
{
#if defined(_WIN32)
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	if (argc != 5)
	{
		// don't treat this as an error and just return 0
#if defined(ASAR_TEST_DLL)
		printf("Usage: test.exe [asar_dll_path] [path_to_tests_directory] [path_to_unheadered_SMW_ROM_file] [output_directory]\n");
#else
		printf("Usage: test.exe [asar_exe_path] [path_to_tests_directory] [path_to_unheadered_SMW_ROM_file] [output_directory]\n");
#endif
		return 0;
	}

#if defined(ASAR_TEST_DLL)
	const char * asar_dll_path = argv[1];
	std::string stdincludespath = dir(asar_dll_path) + "stdincludes.txt";
	std::string stddefinespath = dir(asar_dll_path) + "stddefines.txt";
#else
	const char * asar_exe_path = argv[1];
	std::string stdincludespath = dir(asar_exe_path) + "stdincludes.txt";
	std::string stddefinespath = dir(asar_exe_path) + "stddefines.txt";
#endif
	const char * test_directory = argv[2];
	const char * unheadered_rom_file = argv[3];
	const char * output_directory = argv[4];
	std::vector<wrapped_file> input_files;


	{
		std::string stdinclude = " ";
		stdinclude += test_directory;
		if (!str_ends_with(stdinclude.c_str(), "/") && !str_ends_with(stdinclude.c_str(), "\\")) stdinclude += "/";
		stdinclude += "stdinclude \n";

		write_buffer_to_file(stdincludespath.c_str(), stdinclude);
	}


	{
		std::string stddefines = "!stddefined=1\n stddefined2=1\n\nstddefined3\nstddefined4 = 1 \nstddefined5 = \" $60,$50,$40 \"\n";

		write_buffer_to_file(stddefinespath.c_str(), stddefines);
	}


#if defined(ASAR_TEST_DLL)
	if (!asar_init_with_dll_path(asar_dll_path))
	{
		printf("Error: Failed to load Asar DLL at '%s'.\n", asar_dll_path);
		return 1;
	}
#endif

	if (!find_files_in_directory(input_files, test_directory) || input_files.size() <= 0)
	{
		printf("Error: No ASM files found in %s.\n", test_directory);
#if defined(ASAR_TEST_DLL)
		asar_close();
#endif
		return 1;
	}

	std::sort(input_files.begin(), input_files.end(), [](const wrapped_file& a, const wrapped_file& b) {
		return strcmp(a.file_path, b.file_path) < 0;
	});

	if (!file_exists(unheadered_rom_file))
	{
		printf("Error: '%s' doesn't exist!\n", unheadered_rom_file);
#if defined(ASAR_TEST_DLL)
		asar_close();
#endif
		return 1;
	}

	static const unsigned int smw_size = 512 * 1024;

	std::vector<char> smwrom_buf(read_file_into_buffer(unheadered_rom_file));
	const char* smwrom = smwrom_buf.data();

	if (smwrom_buf.size() != smw_size)
	{
		printf("Error: '%s' doesn't seem to be an unheadered SMW ROM file! Expected a size of %u bytes, got %u.\n",
			unheadered_rom_file, smw_size, (unsigned int)smwrom_buf.size());
#if defined(ASAR_TEST_DLL)
		asar_close();
#endif
		return 1;
	}

	bool has_path_seperator = false;
	if (str_ends_with(output_directory, "/") || str_ends_with(output_directory, "\\"))
	{
		has_path_seperator = true;
	}


	int numfailed = 0;

	// RPG Hacker: A list of standard prints to ignore from print output verification. They will make tests fail that are
	// supposed to be successful. This is kind of stinky solution, but there currently isn't really a good way to distinguish
	// between "user prints" and prints coming directly from Asar.
	std::set<std::string> standard_prints;
	standard_prints.insert("Errors were detected while assembling the patch. Assembling aborted. Your ROM has not been modified.");
	standard_prints.insert("A fatal error was detected while assembling the patch. Assembling aborted. Your ROM has not been modified.");

	bool all_lib_test_passed = false;

	for (size_t testno = 0; testno < input_files.size(); ++testno)
	{
		char * fname = input_files[testno].file_path;

		char out_rom_name[512];
		snprintf(out_rom_name, sizeof(out_rom_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".sfc");
		char stdout_log_name[512];
		snprintf(stdout_log_name, sizeof(stdout_log_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".stdout.log");
		char stderr_log_name[512];
		snprintf(stderr_log_name, sizeof(stderr_log_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".stderr.log");

		// Delete files if they already exist, so we don't get leftovers from a previous testrun
		delete_file(out_rom_name);
		delete_file(stdout_log_name);
		delete_file(stderr_log_name);

		char * expectedrom = (char*)malloc(max_rom_size);
		char * truerom = (char*)malloc(max_rom_size);
		// RPG Hacker: Those memsets are required.
		// Without them, tests can fail due to garbage being left in memory.
		memset(expectedrom, 0, max_rom_size);
		memset(truerom, 0, max_rom_size);
		if (!file_exists(fname)) dief("Error: '%s' doesn't exist!\n", fname);
		int pos = 0;
		int len = 0;

		std::vector<char> asmfile(read_file_into_buffer(fname));

		// RPG Hacker: Skip BoM if present.
		if (asmfile[0] == '\xEF' || asmfile[1] == '\xBB' || asmfile[2] == '\xBF')
		{
			asmfile.erase(asmfile.begin(), asmfile.begin() + 3);
		}

		int numiter = 1;

		std::vector<int> expected_errors;
		std::vector<int> expected_warnings;
		std::vector<std::string> expected_prints;
		std::vector<std::string> expected_error_prints;
		std::vector<std::string> expected_warn_prints;

		int asm_file_pos = 0;

		while (true)
		{
			std::string line;

			long int line_start_pos = asm_file_pos;

			while (asm_file_pos < (int)asmfile.size() && asmfile[asm_file_pos] != '\n')
			{
				// RPG Hacker: We ignore \r, because we don't support text mode and we want our checks
				// to work consistently across platforms.
				if (asmfile[asm_file_pos] != '\r')
				{
					line += asmfile[asm_file_pos];
				}
				asm_file_pos++;
			}

			// If we haven't reached EoF yet, last read must have been a \n, so skip it.
			if (asm_file_pos < (int)asmfile.size()) asm_file_pos++;

			if (line[0] == ';' && line[1] == '`')
			{
				std::vector<std::string> words = tokenize_string(line.c_str() + 2, " ");
				for (size_t i = 0; i < words.size(); ++i)
				{
					std::string& cur_word = words[i];
					char * outptr;
					int num = (int)strtol(words[i].c_str(), &outptr, 16);
					if (*outptr) num = -1;
					if (cur_word == "+")
					{
						if (line[2] == '+')
						{
							memcpy(expectedrom, smwrom, smw_size);
							write_buffer_to_file(out_rom_name, smwrom, smw_size);
							len = smw_size;
						}
					}
					if ((cur_word.length() == 5 || cur_word.length() == 6) && num >= 0) pos = num;
					if (cur_word.length() == 2 && num >= 0) expectedrom[pos++] = (char)num;
					if (cur_word[0] == '#')
					{
						numiter = atoi(cur_word.c_str() + 1);
					}

					const char* token = "errE";
					if (strncmp(cur_word.c_str(), token, strlen(token)) == 0)
					{
						const char* idstr = cur_word.c_str() + strlen(token);
						char* endpos = nullptr;
						long int id = strtol(idstr, &endpos, 10);

						if (endpos == nullptr || *endpos != '\0')
						{
							dief("Error: Invalid %s declaration!\n", token);
						}

						expected_errors.push_back((int)id);
					}

					token = "warnW";
					if (strncmp(cur_word.c_str(), token, strlen(token)) == 0)
					{
						const char* idstr = cur_word.c_str() + strlen(token);
						char* endpos = nullptr;
						long int id = strtol(idstr, &endpos, 10);

						if (endpos == nullptr || *endpos != '\0')
						{
							dief("Error: Invalid %s declaration!\n", token);
						}

						expected_warnings.push_back((int)id);
					}

					if (pos > len) len = pos;
				}
			}
			else if(line[0] == ';' && line[1] != '\0' && line[2] == '>')
			{
				std::string string_to_print(line, 3, std::string::npos);
				if (line[1] == 'P')
				{
					expected_prints.push_back(string_to_print);
				}
				else if (line[1] == 'E')
				{
					expected_error_prints.push_back(string_to_print);
				}
				else if (line[1] == 'W')
				{
					expected_warn_prints.push_back(string_to_print);
				}
			}
			else
			{
				asm_file_pos = line_start_pos;
				break;
			}
		}

		printf("(%u/%u) ", (unsigned int)(testno + 1), (unsigned int)input_files.size());

#if defined(ASAR_TEST_DLL)
		// RPG Hacker: We'll just use truerom directly when testing the DLL, since it takes
		// a memory buffer, anyways. Makes everything easier.
		std::vector<char> out_rom_data(read_file_into_buffer(out_rom_name));
		int truelen = (int)out_rom_data.size();
		memcpy(truerom, out_rom_data.data(), out_rom_data.size());

		printf("Patching: %s\n", fname);

		{
			std::string base_path_string = dir(fname);
			const char* base_path = base_path_string.c_str();

			patchparams asar_patch_params;
			memset(&asar_patch_params, 0, sizeof(asar_patch_params));

			asar_patch_params.structsize = (int)sizeof(asar_patch_params);

			asar_patch_params.patchloc = fname;
			asar_patch_params.romdata = truerom;
			asar_patch_params.buflen = (int)max_rom_size;
			asar_patch_params.romlen = &truelen;

			asar_patch_params.includepaths = &base_path;
			asar_patch_params.numincludepaths = 1;

			asar_patch_params.stdincludesfile = stdincludespath.c_str();
			asar_patch_params.stddefinesfile = stddefinespath.c_str();

			const definedata libdefines[] =
			{
				{ "cmddefined", nullptr },
				{ "!cmddefined2", "" },
				{ " !cmddefined3 ", " $10,$F0,$E0 "},
				// RPG Hacker: 日本語🇯🇵 in UTF-8
				{ "cmdl_define_utf8", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xf0\x9f\x87\xaf\xf0\x9f\x87\xb5" },
			};

			asar_patch_params.additional_defines = libdefines;
			asar_patch_params.additional_define_count = sizeof(libdefines) / sizeof(libdefines[0]);

			const char memory_file_path[] = "file.memory";
			const char memory_file_buffer[] = "!fancy_memory_file_define = $123456";

			const memoryfile libmemoryfiles[] =
			{
				{memory_file_path, (const void *)memory_file_buffer, sizeof(memory_file_buffer) }
			};

			asar_patch_params.memory_files = libmemoryfiles;
			asar_patch_params.memory_file_count = sizeof(libmemoryfiles) / sizeof(libmemoryfiles[0]);


			asar_patch_params.warning_settings = nullptr;
			asar_patch_params.warning_setting_count = 0;

			for (int i = 0;i < numiter;i++)
			{

				if (numiter > 1)
				{
					printf("Iteration %d of %d\n", i+1, numiter);
				}

				if (!asar_patch(&asar_patch_params))
				{
					//printf("asar_patch_ex() failed on file '%s':\n", fname);
				}
				else
				{
					bool test_status = true;
					// Applying patch via DLL succeeded; print some stuff (mainly to verify most of our functions)
					mappertype mapper = asar_getmapper();

					test_status &= (unsigned int)mapper == lorom ? 1 : 0;

					int count = 0;
					const labeldata * labels = asar_getalllabels(&count);

					if (count > 0)
					{
						bool current_test = false;
						for (int j = 0; j < count; ++j)
						{
							if(!strcmp("lib_test_file_label", labels[j].name) &&
							labels[j].location == 0x008000 && asar_getlabelval("lib_test_file_label") == 0x8000)  current_test = true;
						}
						test_status &= current_test;
					}


					const definedata * defines = asar_getalldefines(&count);

					if (count > 0)
					{
						bool current_test = false;
						for (int j = 0; j < count; ++j)
						{
							if(!strcmp("lib_test_file_define", defines[j].name) &&
								!strcmp("$123456", defines[j].contents)) current_test = true;
						}
						test_status &= !strcmp(asar_getdefine("lib_test_file_define"), "$123456");
						test_status &= current_test;
						if(test_status)
						{
							test_status &= !strcmp(asar_resolvedefines("!lib_test_file_define"), "$123456");
						}
					}


					const writtenblockdata * writtenblocks = asar_getwrittenblocks(&count);

					if (count > 0)
					{
						bool current_test = false;
						for (int j = 0; j < count; ++j)
						{
							if(writtenblocks[j].numbytes == 3 && writtenblocks[j].pcoffset == 0) current_test = true;
						}
						test_status &= current_test;
					}

					const char *error_buffer = new char[512];
					test_status &= (int64_t)asar_math("1+0", &error_buffer) == 1;
					delete [] error_buffer;

					test_status &= asar_version() >= 20000;
					test_status &= asar_maxromsize() == 16*1024*1024;

					//the cli test verifies the symbol file, just make sure something generated
					test_status &= asar_getsymbolsfile("wla")[0] == ';';

					if(test_status) all_lib_test_passed = true;
				}

				std::string stderr_buff;
				std::string stdout_buff;

				{
					int numerrors;
					const struct errordata * errors = asar_geterrors(&numerrors);
					for (int j = 0; j < numerrors; ++j)
					{
						stderr_buff += errors[j].fullerrdata;
						stderr_buff += "\n";
					}
				}

				{
					int numwarnings;
					const struct errordata * warnings = asar_getwarnings(&numwarnings);
					for (int j = 0; j < numwarnings; ++j)
					{
						stderr_buff += warnings[j].fullerrdata;
						stderr_buff += "\n";
					}
				}

				{
					int numprints;
					const char * const * prints = asar_getprints(&numprints);
					for (int j = 0; j < numprints; ++j)
					{
						stdout_buff += prints[j];
						stdout_buff += "\n";
					}
				}

				write_buffer_to_file(stderr_log_name, stderr_buff);
				write_buffer_to_file(stdout_log_name, stdout_buff);
			}
		}
#else
		{
			std::string base_path_string = dir(fname);
			const char* base_path = base_path_string.c_str();

			char cmd[1024];
			// randomdude999: temp workaround: using $ in command line is unsafe on linux, so use dec representation instead (for !cmddefined3)
			snprintf(cmd, sizeof(cmd),
				"\"%s\" -I\"%s\" -Dcli_only=\\$1 -Dcmddefined -D!cmddefined2= --define \" !cmddefined3 = 16,240,224 \""
				// RPG Hacker: 日本語🇯🇵 in UTF-8
				" -Dcmdl_define_utf8=\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xf0\x9f\x87\xaf\xf0\x9f\x87\xb5"
				" \"%s\" \"%s\"",
				asar_exe_path, base_path, fname, out_rom_name);
			for (int i = 0;i < numiter;i++)
			{
				printf("Executing: %s\n", cmd);
				if (!execute_command_line(cmd, stdout_log_name, stderr_log_name))
				{
					dief("Failed executing command line:\n    %s\n", cmd);
				}
			}
			all_lib_test_passed = true;
		}
#endif

		std::vector<int> actual_errors;
		std::vector<int> actual_warnings;
		std::vector<std::string> actual_prints;
		std::vector<std::string> actual_error_prints;
		std::vector<std::string> actual_warn_prints;
		std::list<std::string> lines_to_print;

		{
			std::vector<char> buf(read_file_into_buffer(stderr_log_name));

			std::string log_line;

			for (size_t i = 0; i < buf.size(); ++i)
			{
				if (buf[i] == '\n' || buf[i] == '\0')
				{
					const char* token = ": error: (E";
					size_t found = log_line.find(token);
					if (found != std::string::npos)
					{
						char* endpos = nullptr;
						long int num = strtol(log_line.c_str() + found + strlen(token), &endpos, 10);

						if (endpos == nullptr || *endpos != ')')
						{
							dief("Error: Failed parsing error code from Asar output!\n");
						}

						actual_errors.push_back(num);

						// RPG Hacker: Check if it's the error command. If so, we also need to add a print as well.
						{
							std::string command_token = ": error command: ";
							std::string remainder = endpos;
							size_t command_found = remainder.find(command_token);

							if (command_found != std::string::npos)
							{
								actual_error_prints.push_back(std::string(remainder, command_found + command_token.length(), std::string::npos));
							}
						}

						// RPG Hacker: Same goes for the assert command.
						{
							std::string command_token_1 = ": Assertion failed: ";
							std::string command_token_2 = " [assert ";
							std::string remainder = endpos;
							size_t command_found_1 = remainder.find(command_token_1);
							size_t command_found_2 = remainder.find(command_token_2);

							if (command_found_1 != std::string::npos/* && command_found_2 != std::string::npos*/)
							{
								size_t string_start_pos = command_found_1 + command_token_1.length();
								actual_error_prints.push_back(std::string(remainder, string_start_pos, command_found_2-string_start_pos));
							}
						}

						lines_to_print.push_back(log_line);
					}

					token = ": warning: (W";
					found = log_line.find(token);
					if (found != std::string::npos)
					{
						char* endpos = nullptr;
						long int num = strtol(log_line.c_str() + found + strlen(token), &endpos, 10);

						if (endpos == nullptr || *endpos != ')')
						{
							dief("Error: Failed parsing warning code from Asar output!\n");
						}

						actual_warnings.push_back(num);

						// RPG Hacker: Check if it's the warn command. If so, we also need to add a print as well.
						{
							std::string command_token = ": warn command: ";
							std::string remainder = endpos;
							size_t command_found = remainder.find(command_token);

							if (command_found != std::string::npos)
							{
								actual_warn_prints.push_back(std::string(remainder, command_found + command_token.length(), std::string::npos));
							}
						}

						lines_to_print.push_back(log_line);
					}

					log_line.clear();
				}
				else
				{
					if (buf[i] != '\r')
					{
						log_line += buf[i];
					}
				}
			}
		}

		{
			std::vector<char> buf(read_file_into_buffer(stdout_log_name));

			std::string log_line;

			for (size_t i = 0; i < buf.size(); ++i)
			{
				// RPG Hacker: We treat \0 as EOF now.
				// I have no idea how these even end up in the output file, but they definitely shouldn't be considered
				// part of the actual output, and doing so will make a bunch of tests fail.
				if (buf[i] == '\0')
				{
					break;
				}

				if (buf[i] == '\n')
				{
					if (standard_prints.find(log_line) == standard_prints.cend())
					{
						actual_prints.push_back(log_line);

						lines_to_print.push_back(log_line);
					}
					log_line.clear();
				}
				else
				{
					if (buf[i] != '\r')
					{
						log_line += buf[i];
					}
				}
			}
		}

		bool did_fail = false;
		if (expected_errors.size() != actual_errors.size()
			|| expected_warnings.size() != actual_warnings.size()
			|| expected_prints.size() != actual_prints.size()
			|| expected_error_prints.size() != actual_error_prints.size()
			|| expected_warn_prints.size() != actual_warn_prints.size()
			|| !std::equal(expected_errors.begin(), expected_errors.end(), actual_errors.begin())
			|| !std::equal(expected_warnings.begin(), expected_warnings.end(), actual_warnings.begin())
			|| !std::equal(expected_prints.begin(), expected_prints.end(), actual_prints.begin())
			|| !std::equal(expected_error_prints.begin(), expected_error_prints.end(), actual_error_prints.begin())
			|| !std::equal(expected_warn_prints.begin(), expected_warn_prints.end(), actual_warn_prints.begin()))
			did_fail = true;

		if(did_fail) {
			// this thing depends on c++11 already, right?
			for(std::string line_to_print : lines_to_print)
			{
				printf("%s\n", line_to_print.c_str());
			}
			printf("\nExpected errors: ");
			for (auto it = expected_errors.begin(); it != expected_errors.end(); ++it)
			{
				printf("%sE%d", (it != expected_errors.begin() ? "," : ""), *it);
			}
			printf("\n");

			printf("Actual errors: ");
			for (auto it = actual_errors.begin(); it != actual_errors.end(); ++it)
			{
				printf("%sE%d", (it != actual_errors.begin() ? "," : ""), *it);
			}
			printf("\n");

			printf("\nExpected warnings: ");
			for (auto it = expected_warnings.begin(); it != expected_warnings.end(); ++it)
			{
				printf("%sW%d", (it != expected_warnings.begin() ? "," : ""), *it);
			}
			printf("\n");

			printf("Actual warnings: ");
			for (auto it = actual_warnings.begin(); it != actual_warnings.end(); ++it)
			{
				printf("%sW%d", (it != actual_warnings.begin() ? "," : ""), *it);
			}
			printf("\n");

			printf("\nExpected user prints: \n");
			for (auto it = expected_prints.begin(); it != expected_prints.end(); ++it)
			{
				printf("(Print) \"%s\"\n", it->c_str());
			}
			for (auto it = expected_error_prints.begin(); it != expected_error_prints.end(); ++it)
			{
				printf("(Error) \"%s\"\n", it->c_str());
			}
			for (auto it = expected_warn_prints.begin(); it != expected_warn_prints.end(); ++it)
			{
				printf("(Warning) \"%s\"\n", it->c_str());
			}
			printf("\n");

			printf("Actual user prints: \n");
			for (auto it = actual_prints.begin(); it != actual_prints.end(); ++it)
			{
				printf("(Print) \"%s\"\n", it->c_str());
			}
			for (auto it = actual_error_prints.begin(); it != actual_error_prints.end(); ++it)
			{
				printf("(Error) \"%s\"\n", it->c_str());
			}
			for (auto it = actual_warn_prints.begin(); it != actual_warn_prints.end(); ++it)
			{
				printf("(Warning) \"%s\"\n", it->c_str());
			}
			printf("\n");

			dief("Mismatch!\n");
		}


#if !defined(ASAR_TEST_DLL)
		std::vector<char> out_rom_data(read_file_into_buffer(out_rom_name));
		int truelen = (int)out_rom_data.size();
		memcpy(truerom, out_rom_data.data(), out_rom_data.size());
#endif
		bool fail = false;
		for (int i = 0;i < min(len, truelen);i++)
		{
			if (truerom[i] != expectedrom[i] && !(i >= 0x07FDC && i <= 0x07FDF && (expectedrom[i] == 0x00 || expectedrom[i] == smwrom[i])))
			{
				printf("%s: Mismatch at %.5X: Expected %.2X, got %.2X\n", fname, i, (unsigned char)expectedrom[i], (unsigned char)truerom[i]);
				fail = true;
			}
		}
		if (truelen != len) dief("%s: Bad ROM length (expected %X, got %X)\n", fname, len, truelen);
		if (fail) die();

		free(expectedrom);
		free(truerom);

		printf("Success!\n\n");
	}

	printf("%u out of %u performed tests succeeded.\n", (unsigned int)(input_files.size() - (size_t)numfailed), (unsigned int)input_files.size());

	if (numfailed > 0 || !all_lib_test_passed)
	{
		if(!all_lib_test_passed) printf("Library API failed to verify.\n");
		printf("Some tests failed!\n");
		return 1;
	}

#if defined(ASAR_TEST_DLL)
	asar_close();
#endif

	return 0;
}
