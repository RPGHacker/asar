

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

#	if defined(_MSC_VER)
#		pragma warning(pop)
#	endif
#else
#	include <dirent.h>
#	include <vector>
#	include <string>
#	include <string.h>
#	include <sys/stat.h>
#endif

#if defined(ASAR_TEST_DLL)
#	include "asardll.h"
#endif

#define die() { fclose(fopen(fail_name, "wb")); numfailed++; free(expectedrom); free(truerom); printf("\nFailure!\n\n\n"); continue; }
#define dief(...) { printf(__VA_ARGS__); die(); }

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
			result.erase((size_t)(i + 1));
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
					strncpy(new_file.file_name, fd.cFileName, sizeof(new_file.file_name));
					new_file.file_name[sizeof(new_file.file_name) - 1] = '\0';
					strncpy(new_file.file_path, directory_name, sizeof(new_file.file_path));
					if (!has_path_seperator)
					{
						strncat(new_file.file_path, "/", sizeof(new_file.file_path));
					}
					strncat(new_file.file_path, fd.cFileName, sizeof(new_file.file_path));
					new_file.file_path[sizeof(new_file.file_path) - 1] = '\0';
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

	bool has_path_seperator = false;
	if (str_ends_with(directory_name, "/") || str_ends_with(directory_name, "\\"))
	{
		has_path_seperator = true;
	}
	DIR * dir;
	dirent * ent;
	if ((dir = opendir(directory_name)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
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
						strncat(new_file.file_path, "/", sizeof(new_file.file_path));
					}
					strncat(new_file.file_path, ent->d_name, sizeof(new_file.file_path));
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


void delete_file(const char * filename)
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

	fflush(stdout);
	
	FILE * fp = popen(commandline, "r");
	FILE * logfilehandle = fopen(log_file, "wt+");
	
	char buffer[4096];
	char* last_line = nullptr;
	
	do
	{
		last_line = fgets(buffer, sizeof(buffer), fp);
		if (last_line != nullptr)
		{
			fputs(last_line, logfilehandle);
		}
	}
	while (last_line != nullptr);
	
	pclose(fp);
	fclose(logfilehandle);

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


	FILE* stdincludesfile = fopen(stdincludespath.c_str(), "wb");

	if (stdincludesfile == nullptr)
	{
		printf("Error: Failed to write std includes file at '%s'.\n", stdincludespath.c_str());
		return 1;
	}
	else
	{
		std::string stdinclude = " ";
		stdinclude += test_directory;
		if (!str_ends_with(stdinclude.c_str(), "/") && !str_ends_with(stdinclude.c_str(), "\\")) stdinclude += "/";
		stdinclude += "stdinclude \n";
		fwrite((const void*)stdinclude.c_str(), 1, stdinclude.length(), stdincludesfile);
		fclose(stdincludesfile);
	}


	FILE* stddefinesfile = fopen(stddefinespath.c_str(), "wb");

	if (stddefinesfile == nullptr)
	{
		printf("Error: Failed to write std defines file at '%s'.\n", stddefinespath.c_str());
		return 1;
	}
	else
	{
		std::string stddefines = "!stddefined=1\n stddefined2=1\nstddefined3\nstddefined4 = 1 \n";
		fwrite((const void*)stddefines.c_str(), 1, stddefines.length(), stddefinesfile);
		fclose(stddefinesfile);
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
		FILE * asmfile = fopen(fname, "rb");
		int pos = 0;
		int len = 0;
		FILE * rom = fopen(out_rom_name, "wb");

		int numiter = 1;

		bool shouldfail = false;
		while (true)
		{
			long int line_start_pos = ftell(asmfile);
			
			// RPG Hacker: Not doing this in text mode so that it works fine on both
			// Linux and Windows.
			// First we read all characters until we hit a \n or \r.
			size_t current_line_length = 0;
			size_t just_read = 0;
			char last_read = '\0';
			do
			{
				just_read = fread(&line[current_line_length], 1, 1, asmfile);
				last_read = line[current_line_length];
				if (last_read != '\r')
				{
					current_line_length += just_read;
				}
			}
			while (just_read != 0 && current_line_length < sizeof(line)-1
				&& last_read != '\n' && last_read != '\0');
				
			// Next we skip all following \n and \r (since they might be part of the
			// same multi-byte new line sequence).
			char next_char = '\0';
			just_read = fread(&next_char, 1, 1, asmfile);
			
			if (just_read != 0 && next_char != '\r')
			{			
				// Seek back one byte, because we've actually read past the new line now.
				fseek(asmfile, -1, SEEK_CUR);
			}
				
			line[current_line_length-1] = '\0';
			
			if (line[0] != ';' || line[1] != '@')
			{				
				fseek(asmfile, line_start_pos, SEEK_SET);
				break;
			}
			
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
		
		FILE * azmfile = fopen(azm_name, "wt");		
		
		size_t remaining_read = 0;
		do
		{
			char next_char = '\0';
			remaining_read = fread(&next_char, 1, sizeof(next_char), asmfile);
			if (remaining_read > 0)
			{
				if (next_char != '\r')
				{
					if (next_char == '\n')
					{
						next_char = '\n';
					}
			
					fwrite(&next_char, 1, sizeof(next_char), azmfile);
				}
			}
		}
		while (remaining_read > 0);
		
		fclose(azmfile);

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

			patchparams asar_patch_params;
			asar_patch_params.structsize = (int)sizeof(asar_patch_params);

			asar_patch_params.patchloc = azm_name;
			asar_patch_params.romdata = truerom;
			asar_patch_params.buflen = (int)max_rom_size;
			asar_patch_params.romlen = &truelen;

			asar_patch_params.includepaths = &base_path;
			asar_patch_params.numincludepaths = 1;

			asar_patch_params.should_reset = true;

			asar_patch_params.stdincludesfile = stdincludespath.c_str();
			asar_patch_params.stddefinesfile = stddefinespath.c_str();


			for (int i = 0;i < numiter;i++)
			{			
				printf("\n");
				
				if (numiter > 1)
				{
					printf("Iteration %d of %d\n\n", i+1, numiter);
				}
				
				if (!asar_patch_ex(&asar_patch_params))
				{
					printf("asar_patch() failed on file '%s':\n", azm_name);
					int numerrors;
					const errordata * errdata = asar_geterrors(&numerrors);
					for (int j = 0; j < numerrors; ++j)
					{
						fwrite(errdata[j].fullerrdata, 1, strlen(errdata[j].fullerrdata), err);
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

						for (int j = 0; j < count; ++j)
						{
							printf("  %s: %X\n", labels[j].name, (unsigned int)labels[j].location);
						}
					}


					const definedata * defines = asar_getalldefines(&count);

					if (count > 0)
					{
						printf("Found defines:\n");

						for (int j = 0; j < count; ++j)
						{
							printf("  %s=%s\n", defines[j].name, defines[j].contents);
						}
					}


					const writtenblockdata * writtenblocks = asar_getwrittenblocks(&count);

					if (count > 0)
					{
						printf("Written blocks:\n");

						for (int j = 0; j < count; ++j)
						{
							printf("  %u bytes at %X\n", (unsigned int)writtenblocks[j].numbytes, (unsigned int)writtenblocks[j].pcoffset);
						}
					}
				}
			}
		}

		fclose(err);
#else
		{
			std::string base_path_string = dir(fname);
			const char* base_path = base_path_string.c_str();

			char cmd[1024];
			snprintf(cmd, sizeof(cmd), "\"%s\" -I\"%s\" \"%s\" \"%s\"", asar_exe_path, base_path, azm_name, out_rom_name);
			for (int i = 0;i < numiter;i++)
			{
				printf("Executing:\n > %s\n", cmd);
				if (!execute_command_line(cmd, log_name))
				{
					dief("Failed executing command line:\n    %s\n", cmd);
				}
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

		printf("\nSuccess!\n\n\n");
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
