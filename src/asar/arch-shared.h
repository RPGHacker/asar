#pragma once
#include "libstr.h"

// Should only be called inside assembleblock.cpp
bool asblock_65816(const string& word, const char* params);
bool asblock_spc700(char** word, int numwords);
bool asblock_superfx(char** word, int numwords);
