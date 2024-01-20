#pragma once

#include "std-includes.h"

//Note: T must be a pointer type, or stuff will screw up. To make a pointer last longer than this object, assign nullptr to it and it won't free the old one.
template<typename T> class autoptr {
	T ptr;
public:
	operator T() const
	{
		return ptr;
	}

	autoptr& operator=(T ptr_)
	{
		ptr = ptr_;
		return *this;
	}

	autoptr()
	{
		ptr = nullptr;
	}

	autoptr(T ptr_)
	{
		ptr = ptr_;
	}

	autoptr(const autoptr<T>& ptr_)
	{
		ptr = ptr_.ptr;
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
	static const int default_size = 128;

	const T& getconst(int id) const
	{
		if (id < 0) return dummy;
		if (id >= count) return dummy;
		return ptr[id];
	}

	T& get(int id)
	{
		if (id < 0) return dummy;
		if (id >= bufferlen - 4)
		{
			resize(id);
		}
		if (id >= count)
		{
			for (int i = count;i <= id;i++) new(ptr + i) T();
			count = id + 1;
		}
		return ptr[id];
	}
	
	void resize(int size)
	{
		int oldlen = count;
		while (bufferlen <= size + 4) bufferlen *= 2;
		T *old = ptr;
		ptr = (T*)malloc(sizeof(T)*(size_t)bufferlen);
		for(int i = 0; i < oldlen; i++){
			new(ptr + i) T();
			ptr[i] = static_cast<T &&>(old[i]);
		}
		free(old);
		memset(ptr + oldlen, 0, (size_t)(bufferlen - oldlen) * sizeof(T));
	}

public:

	void reset(int keep = 0)
	{
		if (keep >= count) return;
		for (int i = keep;i < count;i++) ptr[i].~T();
		memset(ptr + keep, 0, (size_t)(count - keep) * sizeof(T));
		if (keep < bufferlen / 2)
		{
			while (keep < bufferlen / 2 && bufferlen>8) bufferlen /= 2;
			T *old = ptr;
			ptr = (T*)malloc(sizeof(T)*(size_t)bufferlen);
			for(int i = 0; i < keep; i++){
				new(ptr + i) T();
				ptr[i] = static_cast<T &&>(old[i]);
			}
			free(old);
		
		}
		count = keep;
	}

	T& operator[](int id)
	{
		return get(id);
	}

	const T& operator[](int id) const
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

	T& append(const T& item)
	{
		return (get(count) = item);
	}

	//insert is not safe for non pod types!!!
	void insert(int pos)
	{
		if (pos<0 || pos>count) return;
		if (count >= bufferlen - 4)
		{
			resize(count);
		}
		memmove(ptr + pos + 1, ptr + pos, sizeof(T)*(count - pos));
		memset(ptr + pos, 0, sizeof(T));
		new(ptr + pos) T();
		count++;
	}

	void insert(int pos, const T& item)
	{
		if (pos<0 || pos>count) return;
		if (count >= bufferlen - 4)
		{
			resize(count);
		}
		memmove(ptr + pos + 1, ptr + pos, sizeof(T)*(size_t)(count - pos));
		memset(ptr + pos, 0, sizeof(T));
		new(ptr + pos) T();
		ptr[pos] = item;
		count++;
	}

	void remove(int id)
	{
		if (id < 0 || id >= count) return;
		count--;
		ptr[id].~T();
		for(int i = id; i < count; i++){
			ptr[i] = static_cast<T &&>(ptr[i+1]);
		}
	}

	autoarray()
	{
		ptr = (T*)malloc(sizeof(T) * default_size);
		memset(ptr, 0, default_size*sizeof(T));
		bufferlen = default_size;
		count = 0;
	}

	~autoarray()
	{
		for (int i = 0;i < count;i++) ptr[i].~T();
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
			get(i - 1);
		}
		for (int i = 0;i < count;i++) s(ptr[i]);
	}
#endif
#define SERIALIZER_BANNED
};
