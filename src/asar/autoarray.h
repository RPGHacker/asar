#pragma once
#include <stdlib.h>
#include <string.h>
#include <new>

//Note: T must be a pointer type, or stuff will screw up. To make a pointer last longer than this object, assign NULL to it and it won't free the old one.
template<typename T> class autoptr {
T ptr;
public:
operator T() const
{
	return ptr;
}
autoptr& operator=(T ptr_)
{
	ptr=ptr_;
	return *this;
}
autoptr()
{
	ptr=NULL;
}
autoptr(T ptr_)
{
	ptr=ptr_;
}
~autoptr()
{
	if (ptr) free((void*)ptr);
}
};

template<typename T> class autoarray {
public:
int count;

private:
T* ptr;
int bufferlen;

T dummy;

T& getconst(int id)
{
	if (id<0) return dummy;
	if (id>=count) return dummy;
	return ptr[id];
}

T& get(int id)
{
	if (id<0) return dummy;
	if (id>=bufferlen-4)
	{
		int oldlen=bufferlen;
		while (bufferlen<id+4) bufferlen*=2;
		ptr=(T*)realloc(ptr, sizeof(T)*bufferlen);
		memset(ptr+oldlen, 0, (bufferlen-oldlen)*sizeof(T));
	}
	if (id>=count)
	{
		for (int i=count;i<=id;i++) new(ptr+i) T;
		count=id+1;
	}
	return ptr[id];
}

public:

void reset(int keep=0)
{
	if (keep>=count) return;
	for (int i=keep;i<count;i++) ptr[i].~T();
	memset(ptr+keep, 0, (count-keep)*sizeof(T));
	if (keep<bufferlen/2)
	{
		while (keep<bufferlen/2 && bufferlen>4) bufferlen/=2;
		ptr=(T*)realloc(ptr, sizeof(T)*bufferlen);
	}
	count=keep;
}

T& operator[](int id)
{
	return get(id);
}

T& operator[](int id) const
{
	return getconst(id);
}

operator T*()
{
	return ptr;
}

operator const T*() const
{
	return ptr;
}

void append(const T& item)
{
	get(count)=item;
}

void insert(int pos)
{
	if (pos<0 || pos>count) return;
	if (count>=bufferlen-4)
	{
		int oldlen=bufferlen;
		while (bufferlen<count+4) bufferlen*=2;
		ptr=(T*)realloc(ptr, sizeof(T)*bufferlen);
		memset(ptr+oldlen, 0, (bufferlen-oldlen)*sizeof(T));
	}
	memmove(ptr+pos+1, ptr+pos, sizeof(T)*(count-pos));
	memset(ptr+pos, 0, sizeof(T));
	new(ptr+pos) T;
	count++;
}

void insert(int pos, const T& item)
{
	if (pos<0 || pos>count) return;
	if (count>=bufferlen-4)
	{
		int oldlen=bufferlen;
		while (bufferlen<count+4) bufferlen*=2;
		ptr=(T*)realloc(ptr, sizeof(T)*bufferlen);
		memset(ptr+oldlen, 0, (bufferlen-oldlen)*sizeof(T));
	}
	memmove(ptr+pos+1, ptr+pos, sizeof(T)*(count-pos));
	memset(ptr+pos, 0, sizeof(T));
	new(ptr+pos) T;
	ptr[pos]=item;
	count++;
}

void remove(int id)
{
	if (id<0 || id>=count) return;
	count--;
	ptr[id].~T();
	memmove(ptr+id, ptr+id+1, sizeof(T)*(count-id));
	if (count<bufferlen/2)
	{
		bufferlen/=2;
		ptr=(T*)realloc(ptr, sizeof(T)*bufferlen);
	}
}

autoarray()
{
	ptr=(T*)malloc(sizeof(T)*1);
	memset(ptr, 0, sizeof(T));
	bufferlen=1;
	count=0;
}

~autoarray()
{
	for (int i=0;i<count;i++) ptr[i].~T();
	free(ptr);
}

#ifdef SERIALIZER
void serialize(serializer& s)
{
	if (s.serializing) s(count);
	else
	{
		int i;
		s(i);
		get(i-1);
	}
	for (int i=0;i<count;i++) s(ptr[i]);
}
#endif
#define SERIALIZER_BANNED
};
