#pragma once
inline int min(int val)
{
	return val;
}

template<typename... Args> inline int min(int arg1, const Args&... args)//if this is functional programming, I don't want to know more about it.
{
	int i=min(args...);
	if (arg1<i) return arg1;
	return i;
}

inline int posmin(int val)
{
	return val;
}

template<typename... Args> inline int posmin(int arg1, const Args&... args)
{
	int i=posmin(args...);
	if (arg1>=0 && arg1<i) return arg1;
	return i;
}

template<int N> struct forceconst { enum { value = N }; };
#define forceconst(n) (forceconst<n>::value)

//from nall, license: ISC
    //round up to next highest single bit:
    //round(15) == 16, round(16) == 16, round(17) == 32
inline unsigned bitround(unsigned x)
{
	if ((x & (x - 1)) == 0) return x;
	while (x & (x - 1)) x &= x - 1;
	return x << 1;
}
