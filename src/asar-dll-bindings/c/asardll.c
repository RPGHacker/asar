#include <stdbool.h>
#include <stddef.h>

#if defined(_WIN32)
#	if defined(_MSC_VER)
#		pragma warning(push)
#		pragma warning(disable : 4255)
#		pragma warning(disable : 4668)
#	endif

#	include <windows.h>
#	include <stdio.h>

#	if defined(_MSC_VER)
#		pragma warning(pop)
#	endif

inline static void * getlib(void)
{
	void * ret_val = LoadLibraryW(L"asar.dll");

	if (ret_val == NULL)
	{
		// TODO: Add a better method of error checking? This won't do much for people who are using
		// Asar with a GUI application, they probably won't see this error output.
		char buf[1024];
		sprintf(buf, "Failed to load Asar DLL! HRESULT: 0x%08x\n", (unsigned int)HRESULT_FROM_WIN32(GetLastError()));
		printf("%s", buf);
		OutputDebugStringA(buf);
	}

	return ret_val;
}

inline static void * getlibfrompath(const char * path)
{
	int required_size = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
	if (required_size <= 0) return NULL;

	wchar_t* path_buf = (wchar_t*)malloc((size_t)required_size * sizeof(wchar_t));
	if (path_buf == NULL) return NULL;

	int converted_size = MultiByteToWideChar(CP_UTF8, 0, path, -1, path_buf, required_size);

	if (converted_size == 0)
	{
		return NULL;
	}

	void * ret_val = LoadLibraryW(path_buf);
	free(path_buf);

	if (ret_val == NULL)
	{
		// TODO: Add a better method of error checking? This won't do much for people who are using
		// Asar with a GUI application, they probably won't see this error output.
		char buf[1024];
		sprintf(buf, "Failed to load Asar DLL! HRESULT: 0x%08x\n", (unsigned int)HRESULT_FROM_WIN32(GetLastError()));
		printf("%s", buf);
		OutputDebugStringA(buf);
	}

	return ret_val;
}

inline static bool setfunction(void* target, FARPROC fn)
{
	memcpy(target, &fn, sizeof(fn));
	return fn;
}
#	define loadraw(name, target) require(setfunction(&target, GetProcAddress((HINSTANCE)asardll, name)))
#	define closelib(var) FreeLibrary((HINSTANCE)var)
#else
#	include <dlfcn.h>
#	include <stdio.h>

#	ifdef __APPLE__
#		define EXTENSION ".dylib"
#	else
#		define EXTENSION ".so"
#	endif

	inline static void * getlib(void)
	{
		const char * names[]={"./libasar"EXTENSION, "libasar", NULL};
		for (int i=0;names[i];i++)
		{
			void * rval=dlopen(names[i], RTLD_LAZY);
			const char*e=dlerror();
			if(e)puts(e);
			if (rval) return rval;
		}
		return NULL;
	}

	inline static void * getlibfrompath(const char * path)
	{
		void * rval = dlopen(path, RTLD_LAZY);
		const char*e = dlerror();
		if (e)puts(e);
		if (rval) return rval;
		return NULL;
	}

#	define loadraw(name, target) *(void **)(&target)=dlsym(asardll, name); require(target)
#	define closelib(var) dlclose(var)
#endif

#include "asardll.h"

#undef asarfunc
#undef ASAR_DLL_H_INCLUDED

#define asarfunc
#include "asardll.h"

static void * asardll=NULL;

static bool (*asar_i_init)(void);
static void(*asar_i_close)(void);

#define require(b) if (!(b)) { asardll=NULL; return false; }
#define loadi(name) loadraw("asar_"#name, asar_i_##name)
#define load(name) loadraw("asar_"#name, asar_##name)

static bool asar_init_shared(void)
{
	loadi(init);
	loadi(close);
	load(version);
	load(apiversion);
	load(reset);
	load(patch);
	load(patch_ex);
	load(maxromsize);
	load(geterrors);
	load(getwarnings);
	load(getprints);
	load(getalllabels);
	load(getlabelval);
	load(getdefine);
	load(getalldefines);
	load(math);
	load(getwrittenblocks);
	load(getmapper);
	load(getsymbolsfile);
	if (asar_apiversion() < expectedapiversion || (asar_apiversion() / 100) > (expectedapiversion / 100)) return false;
	require(asar_i_init());
	return true;
}

bool asar_init(void)
{
	if (asardll) return true;
	asardll=getlib();
	require(asardll);
	if (!asar_init_shared()) return false;
	return true;
}

bool asar_init_with_dll_path(const char * dllpath)
{
	if (asardll) return true;
	asardll = getlibfrompath(dllpath);
	require(asardll);
	if (!asar_init_shared()) return false;
	return true;
}

void asar_close(void)
{
	if (!asardll) return;
	asar_i_close();
	closelib(asardll);
	asardll=NULL;
}
