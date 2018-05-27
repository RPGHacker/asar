#pragma once

// This function should only be called inside assembleblock.cpp.
void print(const char * str);

// This function should only be called inside errors.cpp.
void error_interface(int errid, int whichpass, const char * e_);

// This function should only be called inside warnings.cpp.
void warn(int errid, const char * e);
