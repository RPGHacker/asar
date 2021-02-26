#include "fopen_helper.h"
FILE* wfopen(const char* handle, const char* mode) {
	#ifdef _WIN32
  wchar_t *wideHandle = new wchar_t[strlen(handle) + 1];
  wchar_t *wideMode = new wchar_t[strlen(mode) + 1];
  mbstowcs(wideHandle, handle, strlen(handle));
  mbstowcs(wideMode, mode, strlen(mode));
  FILE *file = _wfopen(wideHandle, wideMode);
  delete[] wideHandle;
  delete[] wideMode;
	#else
  FILE *file = fopen(handle, mode);
	#endif
  return file;
}