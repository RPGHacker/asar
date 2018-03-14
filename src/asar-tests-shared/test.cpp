#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#if defined(_WIN32)
#	define NOMINMAX

#	if defined(_MSC_VER)
#		pragma warning(push)
#		pragma warning(disable : 4668)
#	endif

#	include <windows.h>
#	include <vector>
#	include <string>

#	if defined(_MSC_VER)
#		pragma warning(pop)
#	endif
#else
#	include <dirent.h>
#	include <vector>
#	include <string>
#endif

#if defined(ASAR_TEST_DLL)
#	include "asardll.h"
#endif

#define die() do { fclose(fopen(fail_name, "wb")); numfailed++; free(expectedrom); free(truerom); printf("Failure!\n\n"); goto next_test; } while(0)
#define dief(...) do { printf(__VA_ARGS__); die(); } while(0)

#define min(a, b) ((a)<(b)?(a):(b))

static const size_t max_rom_size = 16777216u;


inline int file_exist(const char *filename)
{
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}


//modified from somewhere in nall (license: ISC)
std::string dir(char const *name)
{
	std::string result = name;
	for (signed i = (int)result.length(); i >= 0; i--)
	{
		if (result[(size_t)i] == '/' || result[(size_t)i] == '\\')
		{
			result[(size_t)(i + 1)] = 0;
			break;
		}
		if (i == 0) result = "";
	}
	return result;
}


inline bool str_ends_with(const char * str, const char * suffix)
{
	if (str == NULL || suffix == NULL)
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


bool find_files_in_directory(std::vector<wrapped_file>& out_array, const char * directory_name)
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

	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (str_ends_with(fd.cFileName, ".asm"))
				{
					wrapped_file new_file;
					strncpy(new_file.file_name, fd.cFileName, sizeof(new_file.file_path));
					new_file.file_path[sizeof(new_file.file_path) - 1] = '\0';
					strcpy(new_file.file_path, directory_name);
					if (!has_path_seperator)
					{
						strcat(new_file.file_path, "/");
					}
					strcat(new_file.file_path, fd.cFileName);
					out_array.push_back(new_file);
				}
			}
		} while (::FindNextFile(hFind, &fd));

		::FindClose(hFind);
	}
	else
	{
		return false;
	}

#else

#	error TODO

	DIR * dir;
	dirent * ent;
	if ((dir = opendir(directory_name)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			printf("%s\n", ent->d_name);
		}
		closedir(dir);
	}
	else {
		return false;
	}

#endif

	return true;
}


void delete_file(const char * filename)
{
#if defined(_WIN32)

	DeleteFileA(filename);

#else

#	error TODO

#endif
}


#if !defined(ASAR_TEST_DLL)
// RPG Hacker: Original test application used system(), but
// that really made my anti-virus sad, so here is a new,
// complicated platform-specific solution.
// NOTE: No cont char*, for commandline, because CreateProcess()
// actually can mess with this string for some reason...
bool execute_command_line(char * commandline, const char * log_file)
{
#if defined(_WIN32)

	HANDLE read_handle = NULL;
	HANDLE write_handle = NULL;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&read_handle, &write_handle, &sa, 0))
	{
		return false;
	}

	if (!SetHandleInformation(read_handle, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(read_handle);
		CloseHandle(write_handle);
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	si.hStdError = write_handle;
	si.hStdOutput = write_handle;
	si.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcessA(NULL, commandline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		printf("execute_command_line() failed with HRESULT: 0x%8x", (unsigned int)HRESULT_FROM_WIN32(GetLastError()));
		CloseHandle(read_handle);
		CloseHandle(write_handle);
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(write_handle);

	FILE * logfilehandle = fopen(log_file, "wt");

	DWORD dwRead;
	CHAR chBuf[4096];
	BOOL bSuccess = FALSE;
	for (;;)
	{
		bSuccess = ReadFile(read_handle, chBuf, sizeof(chBuf), &dwRead, NULL);
		if (bSuccess == FALSE || dwRead == 0) break;
		fwrite(chBuf, 1, dwRead, logfilehandle);
	}

	fclose(logfilehandle);

	CloseHandle(read_handle);

#else

#	error TODO

#endif

	return true;
}
#endif


std::vector<std::string> tokenize_string(const char * str, const char * key)
{
	std::string s = str;
	std::string delimiter = key;

	size_t pos = 0;
	std::string token;
	std::vector<std::string> list;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		list.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	list.push_back(s);
	return list;
}

int main(int argc, char * argv[])
{
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
#else
	const char * asar_exe_path = argv[1];
#endif
	const char * test_directory = argv[2];
	const char * unheadered_rom_file = argv[3];
	const char * output_directory = argv[4];
	std::vector<wrapped_file> input_files;

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

	char * smwrom=(char*)malloc(512*1024);
	if (!file_exist(unheadered_rom_file))
	{
		printf("Error: '%s' doesn't exist!\n", unheadered_rom_file);
#if defined(ASAR_TEST_DLL)
		asar_close();
#endif
		return 1;
	}
	FILE * smwromf=fopen(unheadered_rom_file, "rb");
	fread(smwrom, 1, 512*1024, smwromf);
	fclose(smwromf);

	bool has_path_seperator = false;
	if (str_ends_with(output_directory, "/") || str_ends_with(output_directory, "\\"))
	{
		has_path_seperator = true;
	}


	int numfailed = 0;

	for (size_t testno = 0; testno < input_files.size(); ++testno)
	{
		char * fname = input_files[testno].file_path;

		char out_rom_name[512];
		snprintf(out_rom_name, sizeof(out_rom_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".sfc");
		char azm_name[512];
		snprintf(azm_name, sizeof(azm_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".azm");
		char log_name[512];
		snprintf(log_name, sizeof(log_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".log");
		char fail_name[512];
		snprintf(fail_name, sizeof(fail_name), "%s%s%s%s", output_directory, (has_path_seperator ? "" : "/"), input_files[testno].file_name, ".fail");

		// Delete files if they already exist, so we don't get leftovers from a previous testrun
		delete_file(out_rom_name);
		delete_file(azm_name);
		delete_file(log_name);
		delete_file(fail_name);

		char * expectedrom = (char*)malloc(max_rom_size);
		char * truerom = (char*)malloc(max_rom_size);
		// RPG Hacker: Those memsets are required.
		// Without them, tests can fail due to garbage being left in memory.
		memset(expectedrom, 0, max_rom_size);
		memset(truerom, 0, max_rom_size);
		char line[256];
		if (!file_exist(fname)) dief("Error: '%s' doesn't exist!\n", fname);
		FILE * asmfile = fopen(fname, "rt");
		int pos = 0;
		int len = 0;
		FILE * rom = fopen(out_rom_name, "wb");

		int numiter = 1;

		bool shouldfail = false;
		while (true)
		{
			fgets(line, 250, asmfile);
			if (line[0] != ';' || line[1] != '@') break;
			*strchr(line, '\n') = 0;
			std::vector<std::string> words = tokenize_string(line + 2, " ");
			for (size_t i = 0; i < words.size(); ++i)
			{
				std::string& cur_word = words[i];
				char * outptr;
				int num = strtol(words[i].c_str(), &outptr, 16);
				if (*outptr) num = -1;
				if (cur_word == "+")
				{
					if (line[2] == '+')
					{
						memcpy(expectedrom, smwrom, 512 * 1024);
						fwrite(smwrom, 1, 512 * 1024, rom);
						len = 512 * 1024;
					}
				}
				if ((cur_word.length() == 5 || cur_word.length() == 6) && num >= 0) pos = num;
				if (cur_word.length() == 2 && num >= 0) expectedrom[pos++] = (char)num;
				if (cur_word[0] == '#')
				{
					numiter = atoi(cur_word.c_str() + 1);
				}
				if (cur_word == "err") shouldfail = true;
				if (pos > len) len = pos;
			}
		}

		//this is rather hacky and fragile, but it works. it's not like anyone's gonna use this thing
		char * asmdata = (char*)malloc(65536);
		memset(asmdata, 0, 65536);
		char * asmdata_ptr_backup = asmdata;
		strcpy(asmdata, line);
		char * asmdataend = strchr(asmdata, '\0');
		asmdataend[fread(asmdataend, 1, 65536, asmfile)] = '\0';
		FILE * azmfile = fopen(azm_name, "wt");
		while (asmdata[0] == '\n') asmdata++;
		fwrite(asmdata, 1, strlen(asmdata), azmfile);
		fclose(azmfile);

		free(asmdata_ptr_backup);

		fclose(asmfile);
		fclose(rom);

#if defined(ASAR_TEST_DLL)
		// RPG Hacker: We'll just use truerom directly when testing the DLL, since it takes
		// a memory buffer, anyways. Makes everything easier.
		rom = fopen(out_rom_name, "rb");
		fseek(rom, 0, SEEK_END);
		int truelen = ftell(rom);
		fseek(rom, 0, SEEK_SET);
		fread(truerom, 1, (size_t)truelen, rom);
		fclose(rom);
		printf("Patching:\n > %s\n", azm_name);
		FILE * err = fopen(log_name, "wt");

		{
			std::string base_path_string = dir(fname);
			const char* base_path = base_path_string.c_str();

			patchparams asar_path_params;
			asar_path_params.structsize = (int)sizeof(asar_path_params);

			asar_path_params.patchloc = azm_name;
			asar_path_params.romdata = truerom;
			asar_path_params.buflen = (int)max_rom_size;
			asar_path_params.romlen = &truelen;

			asar_path_params.includepaths = &base_path;
			asar_path_params.numincludepaths = 1;

			if (!asar_patch_ex(&asar_path_params))
			{
				printf("asar_patch() failed on file '%s':\n", azm_name);
				int numerrors;
				const errordata * errdata = asar_geterrors(&numerrors);
				for (int i = 0; i < numerrors; ++i)
				{
					fwrite(errdata[i].fullerrdata, 1, strlen(errdata[i].fullerrdata), err);
					fwrite("\n", 1, strlen("\n"), err);
				}
			}
			else
			{
				// Applying patch via DLL succeeded; print some stuff (mainly to verify most of our functions)
				mappertype mapper = asar_getmapper();

				printf("Detected mapper: %u\n", (unsigned int)mapper);

				int count = 0;
				const labeldata * labels = asar_getalllabels(&count);

				if (count > 0)
				{
					printf("Found labels:\n");

					for (int i = 0; i < count; ++i)
					{
						printf("  %s: %X\n", labels[i].name, (unsigned int)labels[i].location);
					}
				}


				const definedata * defines = asar_getalldefines(&count);

				if (count > 0)
				{
					printf("Found defines:\n");

					for (int i = 0; i < count; ++i)
					{
						printf("  %s=%s\n", defines[i].name, defines[i].contents);
					}
				}


				const writtenblockdata * writtenblocks = asar_getwrittenblocks(&count);

				if (count > 0)
				{
					printf("Written blocks:\n");

					for (int i = 0; i < count; ++i)
					{
						printf("  %u bytes at %X\n", (unsigned int)writtenblocks[i].numbytes, (unsigned int)writtenblocks[i].pcoffset);
					}
				}
			}
		}

		fclose(err);
#else
		char cmd[1024];
		snprintf(cmd, sizeof(cmd), "%s %s %s", asar_exe_path, azm_name, out_rom_name);
		for (int i = 0;i < numiter;i++)
		{
			printf("Executing:\n > %s\n", cmd);
			if (!execute_command_line(cmd, log_name))
			{
				dief("Failed executing command line:\n    %s\n", cmd);
			}
		}
		FILE * err = NULL;
#endif
		err = fopen(log_name, "rt");
		fseek(err, 0, SEEK_END);
		if (ftell(err) && !shouldfail)
		{
			fseek(err, 0, SEEK_SET);
			fgets(line, 250, err);
			fclose(err);
			dief("%s: Insertion error: %s\n", fname, line);
		}
		if (!ftell(err) && shouldfail)
		{
			fclose(err);
			dief("%s: No insertion error\n", fname);
		}
		fclose(err);

#if !defined(ASAR_TEST_DLL)
		rom = fopen(out_rom_name, "rb");
		fseek(rom, 0, SEEK_END);
		int truelen = ftell(rom);
		fseek(rom, 0, SEEK_SET);
		fread(truerom, 1, (size_t)truelen, rom);
		fclose(rom);
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

	next_test:
		continue;
	}

	free(smwrom);

	printf("%zu out of %zu performed tests succeeded.\n", input_files.size() - numfailed, input_files.size());

	if (numfailed > 0)
	{
		printf("Some tests failed!\n");
		return 1;
	}

#if defined(ASAR_TEST_DLL)
	asar_close();
#endif

	return 0;
}
