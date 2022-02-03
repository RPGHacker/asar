#pragma once
inline int min(int a, int b)
{
	return a > b ? b : a;
}

inline unsigned bitround(unsigned v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline unsigned makepair(unsigned a, unsigned b)
{
	return (a + b) * ((a + b + 1) >> 1) + b;
	//return a >= b ? a * a + a + b : a + b * b;	//this version is more dense, needs testing
}
