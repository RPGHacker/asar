#pragma once

#include "libstr.h"

bool file_exists(const char * filename);
bool path_is_absolute(const char* path);
char get_native_path_separator();
bool check_is_regular_file(const char* path);

string dir(char const *name);
string create_combined_path(const char* path1, const char* path2);
string normalize_path(const char* path);
string get_base_name(char const *name);
