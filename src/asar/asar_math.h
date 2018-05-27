#pragma once

void initmathcore();
void deinitmathcore();

unsigned int getnum(const char * str);
double getnumdouble(const char * str);

void createuserfunc(const char * name, const char * arguments, const char * content);

void closecachedfiles();

double math(const char * mystr);

extern bool foundlabel;
extern bool forwardlabel;

extern bool math_pri;
extern bool math_round;
