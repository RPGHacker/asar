#pragma once
#define nullptr NULL

template <class Key, class Data>
class lightweight_map {

};

#include "libstr.h"
#include "assocarr.h"

template <class Data> class lightweight_map<string, Data> {
	assocarr<Data> map;
public:
	void insert(string key, Data value)
	{
		map.create(key)=value;
	}
	
	bool find(string key, Data& result)
	{
		if (!map.exists(key)) return false;
		result=map.find(key);
		return true;
	}

	bool exists(string key)
	{
		return map.exists(key);
	}
	
	void remove(string key)
	{
		map.remove(key);
	}
	
	void clear()
	{
		map.reset();
	}
	
	void traverse(void (*action)(const string& key, Data& data))
	{
		map.each(action);
	}
};