// loader for the freeram library
// most of this code is adapted from Asar's asardll.c
#if defined(_WIN32)
#	include <windows.h>

#	define getlib() LoadLibrary("freeram.dll")
#	define getptr(name) GetProcAddress((HINSTANCE)freeramdll, name)
#	define closelib() (FreeLibrary((HINSTANCE)freeramdll) ? 1 : 0)
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
		const char* names[]= {"./libfreeram" EXTENSION, "libfreeram" EXTENSION, NULL};
		for (int i=0;names[i];i++) {
			void* rval = dlopen(names[i], RTLD_LAZY);
			if (rval) return rval;
		}
		return NULL;
	}

#	define getptr(name) dlsym(freeramdll, name)
#	define closelib() (dlclose(freeramdll) ? 0 : 1)
#endif

#include "freeram.h"

static void* freeramdll = NULL;

static freeram_handle (*dll_openfunc)(const char*, char**);
static int (*dll_closefunc)(freeram_handle);
static int (*dll_getramfunc)(freeram_handle, int, const char*, const char*);
static int (*dll_unclaimfunc)(freeram_handle, const char*);

int freeram_loadlib() {
#define checkerr(funcptr) if(!dll_openfunc) {closelib(); freeramdll = NULL; return 0;}
	freeramdll = getlib();
	if(freeramdll == NULL) return 0;
	dll_openfunc = (freeram_handle(*)(const char *, char **))getptr("freeram_open");
	checkerr(dll_openfunc);
	dll_closefunc = (int(*)(freeram_handle))getptr("freeram_close");
	checkerr(dll_closefunc);
	dll_getramfunc = (int(*)(freeram_handle, int, const char*, const char*))getptr("freeram_get_ram");
	checkerr(dll_getramfunc);
	dll_unclaimfunc = (int(*)(freeram_handle, const char*))getptr("freeram_unclaim_ram");
	checkerr(dll_unclaimfunc);
	return 1;
#undef checkerr
}
int freeram_unloadlib() {
	return closelib();
}

freeram_handle freeram_open(const char* romname, char** error) {
	if(!freeramdll) return NULL;
	return dll_openfunc(romname, error);
}

int freeram_close(freeram_handle handle) {
	if(!freeramdll) return 0;
	return dll_closefunc(handle);
}

int freeram_get_ram(freeram_handle handle, int size, const char* identifier, const char* flags) {
	if(!freeramdll) return -2;
	return dll_getramfunc(handle, size, identifier, flags);
}

int freeram_unclaim_ram(freeram_handle handle, const char* identifier) {
	if(!freeramdll) return -2;
	return dll_unclaimfunc(handle, identifier);
}
