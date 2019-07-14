#pragma once
#include <stdint.h> // for int64_t
#include "freeram.h" // for freeram_handle

void initmathcore();
void deinitmathcore();

unsigned int getnum(const char * str);
int64_t getnum64(const char * str);
double getnumdouble(const char * str);

void createuserfunc(const char * name, const char * arguments, const char * content);

void closecachedfiles();

double math(const char * mystr);

extern bool foundlabel;
extern bool forwardlabel;

extern bool math_pri;
extern bool math_round;

extern bool freeram_lib_loaded;
extern freeram_handle rom_freeram_handle;