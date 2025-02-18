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
