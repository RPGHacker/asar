//API for assocarr<mytype> myarr:
//myarr.exists("index")
//  Returns a boolean telling whether the entry exists.
//  Complexity: O(log n).
//myarr.find("index")
//  Returns a mytype& corresponding to the relevant entry. If it doesn't exist, behaviour is undefined.
//  Complexity: O(log n).
//myarr.create("index")
//  Returns a mytype& corresponding to the relevant entry. If it doesn't exist, it's created.
//  Complexity: Worst case O(n), reduced to amortized O(log n) if the element should be at the end (strict weak ordering), or O(log n) if the element
//  exists.
//myarr.remove("index")
//  Removes the element corresponding to the relevant entry. If it doesn't exist, this structure is left untouched.
//  Complexity: Worst case O(n), reduced to amortized O(log n) if the element is at the end.
//myarr.reset()
//  Clears the data structure, calling all destructors. It is safe to call this function on an empty structure.
//  Complexity: O(n).
//myarr.move("from", "to")
//  Moves an element from index "from" to "to". If the source doesn't exist, behaviour is undefined. If the target exists, no action is performed.
//  Complexity: O(n).
//myarr["index"]
//  Similar to myarr.create("index"), but if the returned entry isn't changed by the next call to any function in the same structure, it's removed.
//  if (myarr["index"]) is safe if type can cast to a bool, and myarr["index"]=4 is also valid if the assignment is valid for a type&. However, it is
//  not safe to use assocarr<assocarr<int> > myarr; if (myarr["3"]["4"]), since the inner assocarr won't know that it's supposed to be garbage collected, 
//  and the outer one sees that the inner one changed, so you get pollution and memory waste. However, if (myarr["3"].exists("4")) is safe.
//  Complexity: Same as myarr.create().
//myarr.each(func)
//  Calls func() for each entry in the structure. func must match the prototype void func(const char * index, mytype& val), but can be a lambda. No
//  non-const function may be called on this structure from inside func(), but it is safe to call const functions of the structure. The function
//  calls are in the same order as the indexes, in a strict weak ordering.
//  Complexity: O(n).
//Space usage: O(n).
//C++ version: C++98 or C++03, not sure which.
//Serializer support: Yes, if mytype is serializable.
//"Undefined behaviour" means "segfault" in most cases.

#pragma once

#include <initializer_list>
#include "std-includes.h"
#include "libmisc.h"//bitround

//just a helper for initializer list
template<typename T1, typename T2> struct pair{
	T1 first;
	T2 second;
};

template<typename right> class assocarr {
public:
int num;

private:

const char ** indexes;
right ** ptr;
int bufferlen;

char lastone[sizeof(right)];
int lastid;

void collectgarbage(const char * ifnotmatch=nullptr)
{
	if (lastid==-1) return;
	if (ifnotmatch && !strcmp(indexes[lastid], ifnotmatch)) return;
	if (!memcmp(ptr[lastid], lastone, sizeof(right)))
	{
		int tmpid=lastid;
		lastid=-1;
		remove(indexes[tmpid]);
	}
	lastid=-1;
}

right& rawadd(const char * index, bool collect)
{
	collectgarbage(index);
	int loc=0;
	int skip= (int)bitround((unsigned int)num);
	while (skip)
	{
		int dir;
		if (loc>=num) dir=1;
		else dir=strcmp(indexes[loc], index);
		if (!dir) return ptr[loc][0];
		skip/=2;
		if (dir>0) loc-=skip;
		else loc+=skip;
		if (loc<0)
		{
			loc=0;
			break;
		}
	}
	if (loc<num && strcmp(indexes[loc], index)<0) loc++;
	if (num==bufferlen)
	{
		if (!num) bufferlen=1;
		else bufferlen*=2;
		ptr=(right**)realloc(ptr, sizeof(right*)*(size_t)bufferlen);
		indexes=(const char**)realloc(indexes, sizeof(const char *)*(size_t)bufferlen);
	}
	num++;
	memmove(indexes+loc+1, indexes+loc, sizeof(const char *)*(size_t)(num-loc-1));
	memmove(ptr+loc+1, ptr+loc, sizeof(right*)*(size_t)(num-loc-1));
	indexes[loc]= duplicate_string(index);
	ptr[loc]=(right*)malloc(sizeof(right));
	memset(ptr[loc], 0, sizeof(right));
	new(ptr[loc]) right;
	if (collect)
	{
		lastid=loc;
		memcpy(lastone, ptr[loc], sizeof(right));
	}
	return ptr[loc][0];
}

int find_i(const char * index) const
{
	int loc=0;
	int skip=(int)bitround((unsigned int)num);
	while (skip)
	{
		int dir;
		if (loc>=num) dir=1;
		else dir=strcmp(indexes[loc], index);
		if (!dir) return loc;
		skip/=2;
		if (dir>0) loc-=skip;
		else loc+=skip;
		if (loc<0) return -1;
	}
	return -1;
}

public:

bool exists(const char * index) const
{
	return find_i(index)>=0;
}

right& find(const char * index) const
{
	return ptr[find_i(index)][0];
}

right& create(const char * index)
{
	return rawadd(index, false);
}

void remove(const char * index)
{
	collectgarbage();
	int loc=0;
	int skip= (int)bitround((unsigned int)num);
	while (skip)
	{
		int dir;
		if (loc>=num) dir=1;
		else dir=strcmp(indexes[loc], index);
		if (!dir)
		{
			free((void*)indexes[loc]);
			ptr[loc]->~right();
			free(ptr[loc]);
			memmove(indexes+loc, indexes+loc+1, sizeof(const char *)*(size_t)(num-loc-1));
			memmove(ptr+loc, ptr+loc+1, sizeof(right*)*(size_t)(num-loc-1));
			num--;
			if (num==bufferlen/2)
			{
				bufferlen/=2;
				ptr=(right**)realloc(ptr, sizeof(right*)*(size_t)bufferlen);
				indexes=(const char**)realloc(indexes, sizeof(const char *)*(size_t)bufferlen);
			}
			return;
		}
		skip/=2;
		if (dir>0) loc-=skip;
		else loc+=skip;
		if (loc<0) return;
	}
}

void move(const char * from, const char * to)
{
	collectgarbage();
	int frompos=find_i(from);
	int topos=0;
	int skip=bitround(num);
	while (skip)
	{
		int dir;
		if (topos>=num) dir=1;
		else dir=strcmp(indexes[topos], to);
		if (!dir) return;
		skip/=2;
		if (dir>0) topos-=skip;
		else topos+=skip;
		if (topos<0)
		{
			topos=0;
			break;
		}
	}
	if (topos<num && strcmp(indexes[topos], to)<0) topos++;
	right * tmp=ptr[frompos];
	if (topos==frompos || topos==frompos+1)
	{
		free((void*)indexes[frompos]);
		indexes[frompos]= duplicate_string(to);
	}
	else if (topos>frompos)
	{
		free((void*)indexes[frompos]);
		memmove(indexes+frompos, indexes+frompos+1, sizeof(const char *)*(topos-frompos-1));
		memmove(ptr+frompos, ptr+frompos+1, sizeof(right*)*(topos-frompos-1));
		ptr[topos-1]=tmp;
		indexes[topos-1]= duplicate_string(to);//I wonder what the fuck I'm doing.
	}
	else
	{
		free((void*)indexes[frompos]);
		memmove(indexes+topos+1, indexes+topos, sizeof(const char *)*(frompos-topos));
		memmove(ptr+topos+1, ptr+topos, sizeof(right*)*(frompos-topos));
		ptr[topos]=tmp;
		indexes[topos]= duplicate_string(to);
	}
}

void reset()
{
	for (int i=0;i<num;i++)
	{
		free((void*)indexes[i]);
		ptr[i]->~right();
		free(ptr[i]);
	}
	free(indexes);
	free(ptr);
	indexes=nullptr;
	ptr= nullptr;
	num=0;
	bufferlen=0;
	lastid=-1;
}

assocarr()
{
	indexes= nullptr;
	ptr= nullptr;
	num=0;
	bufferlen=0;
	lastid=-1;
}

assocarr(std::initializer_list<pair<const char *, right>> list)
{
	indexes= nullptr;
	ptr= nullptr;
	num=0;
	bufferlen=0;
	lastid=-1;
	
	for(auto &item : list){
		rawadd(item.first, true) = item.second;
	}
}

~assocarr()
{
	reset();
}

right& operator[](const char * index)
{
	return rawadd(index, true);
}

//void(*func)(const char * key, right& value)
template<typename t> void each(t func)
{
	collectgarbage();
	for (int i=0;i<num;i++)
	{
		func(indexes[i], ptr[i][0]);
	}
}

//void debug(){puts("");for(int i=0;i<num;i++)puts(indexes[i]);}

#ifdef SERIALIZER
void serialize(serializer & s)
{
	collectgarbage();
	if (s.serializing)
	{
		s(num);
		s(bufferlen);
		for (int i=0;i<num;i++)
		{
			s(ptr[i][0]);
			s(indexes[i]);
		}
	}
	else
	{
		reset();
		s(num);
		s(bufferlen);
		ptr=(right**)malloc(sizeof(right*)*bufferlen);
		indexes=(const char**)malloc(sizeof(const char *)*bufferlen);
		for (int i=0;i<num;i++)
		{
			ptr[i]=(right*)malloc(sizeof(right));
			memset(ptr[i], 0, sizeof(right));
			new(ptr[i]) right;
			s(ptr[i][0]);
			s(indexes[i]);
		}
	}
}
#endif
#define SERIALIZER_BANNED
};
