#define NULL 0
#ifdef _WIN32
	#include <windows.h>
	#define getlib() LoadLibrary("asar.dll")
	#define loadraw(name, target) *((int(**)())&target)=(int(*)())GetProcAddress((HINSTANCE)asardll, name); require(target)
	#define closelib(var) FreeLibrary((HINSTANCE)var)
#else
	#include <dlfcn.h>
	#include <stdio.h>

	#ifdef __APPLE__
		#define EXTENSION ".dylib"
	#else
		#define EXTENSION ".so"
	#endif

	inline static void * getlib()
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

	#define loadraw(name, target) *(void **)(&target)=dlsym(asardll, name); require(target)
	#define closelib(var) dlclose(var)
#endif
#define asarfunc
#include "asardll.h"

static void * asardll=NULL;

static bool (*asar_i_init)();
static void (*asar_i_close)();

bool asar_init()
{
#define require(b) if (!(b)) { asardll=NULL; return false; }
	if (asardll) return true;
	asardll=getlib();
	require(asardll);
#define loadi(name) loadraw("asar_"#name, asar_i_##name)
#define load(name) loadraw("asar_"#name, asar_##name)
	loadi(init);
	loadi(close);
	load(version);
	load(apiversion);
	load(reset);
	load(patch);
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
	if (asar_apiversion()<expectedapiversion || (asar_apiversion()/100)>(expectedapiversion/100)) return false;
	require(asar_i_init());
	return true;
}

void asar_close()
{
	if (!asardll) return;
	asar_i_close();
	closelib(asardll);
	asardll=NULL;
}
