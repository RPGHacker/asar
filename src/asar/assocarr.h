// this used to be a custom map type, now replaced with just a wrapper around std::unordered_map.

#pragma once

#include <initializer_list>
#include <unordered_map>
#include "libstr.h"

template<typename right>
class assocarr {
private:

std::unordered_map<string, right> storage;

public:

bool exists(const char* key) const
{
	return storage.find(key) != storage.end();
}

right& find(const char* key)
{
	return storage.find(key)->second;
}

right& create(const char* key)
{
	return storage[key];
}

void remove(const char* key)
{
	storage.erase(key);
}

void reset()
{
	storage.clear();
}

assocarr()
{
}

assocarr(std::initializer_list<std::pair<const char*, right>> list)
{
	for (auto& it : list) {
		storage.insert(std::move(it));
	}
}

right& operator[](const char* key)
{
	return create(key);
}

//void(*func)(const char * key, right& value)
template<typename T> void each(T func)
{
	for (auto& it : storage) {
		func(it.first, it.second);
	}
}

};
