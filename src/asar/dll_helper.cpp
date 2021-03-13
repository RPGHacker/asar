#include "dll_helper.h"
#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable: 4668) // warning C4668: '_WIN32_WINNT_WIN10_RS<n>' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
// I'm including <windows.h> here and not in the dll_helper.h file because
// including it there yields errors in winnt.h when I include dll_helper.h in other files that use platform/file-helpers.h,
// (see file-helpers-win32.cpp for more info)
#include <windows.h>        
#pragma warning(pop)

constexpr unsigned long long MB(unsigned long long n) { return n * 1024 * 1204; }
thread_local FIBER_DATA *g_pData;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  switch (fdwReason) {
  case DLL_THREAD_ATTACH:
  case DLL_PROCESS_ATTACH:
    if (OnAttach() != 0)
      return FALSE;
    break;
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    OnDetach();
    break;
  }
  return TRUE;
}

ULONG FIBER_DATA::Create(unsigned long long dwStackCommitSize,
                         unsigned long long dwStackReserveSize) {
  if (ConvertThreadToFiber(this)) {
    _bConvertToThread = TRUE;
  } else {
    ULONG dwError = GetLastError();

    if (dwError != ERROR_ALREADY_FIBER) {
      return dwError;
    }
  }
  _MyFiber =
      CreateFiberEx(dwStackCommitSize, dwStackReserveSize, 0, _FiberProc, this);
  return _MyFiber ? NOERROR : GetLastError();
}

VOID CALLBACK FIBER_DATA::_FiberProc(void *lpParameter) {
  reinterpret_cast<FIBER_DATA *>(lpParameter)->FiberProc();
}

VOID FIBER_DATA::FiberProc() {
  for (;;) {
    _dwError = _pfn(_Parameter);
    SwitchToFiber(_PrevFiber);
  }
}

FIBER_DATA::~FIBER_DATA() {
  if (_MyFiber) {
    DeleteFiber(_MyFiber);
  }

  if (_bConvertToThread) {
    ConvertFiberToThread();
  }
}

ULONG FIBER_DATA::DoCallout(STACK_EXPAND pfn, void *Parameter) {
  _PrevFiber = GetCurrentFiber();
  _pfn = pfn;
  _Parameter = Parameter;
  SwitchToFiber(_MyFiber);
  return _dwError;
}

ULONG OnAttach() {
  if (FIBER_DATA *pData = new FIBER_DATA) {
    if (ULONG dwError = pData->Create(MB(4), MB(8))) {
      delete pData;
      return dwError;
    }

    g_pData = pData;

    return NOERROR;
  }

  return ERROR_NO_SYSTEM_RESOURCES;
}

void OnDetach() {
  if (FIBER_DATA *pData = g_pData) {
    delete pData;
  }
}

ULONG DoCallout(STACK_EXPAND pfn, void* Parameter) {
  if (FIBER_DATA *pData = g_pData) {
    return pData->DoCallout(pfn, Parameter);
  }

  return ERROR_GEN_FAILURE;
}
#endif