#pragma once
#include <stdint.h> // for int64_t

void initmathcore();
void deinitmathcore();

int64_t getnum(const char * str);
int64_t getnum64(const char * str);
double getnumdouble(const char * str);

void createuserfunc(const char * name, const char * arguments, const char * content);

void closecachedfiles();

double math(const char * mystr);

extern bool foundlabel;
extern bool foundlabel_static;
extern bool forwardlabel;

extern bool math_pri;
extern bool math_round;
