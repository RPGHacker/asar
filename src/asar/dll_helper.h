#if defined(_WIN32)
#include <cstdio>
typedef unsigned long(_stdcall *STACK_EXPAND)(void* Parameter);
class FIBER_DATA {
public:
  void* _PrevFiber,*_MyFiber;
  STACK_EXPAND _pfn;
  void* _Parameter;
  unsigned long _dwError;
  int _bConvertToThread;

  static void _stdcall _FiberProc(void* lpParameter);

  void FiberProc();

public:
  ~FIBER_DATA();

#pragma warning(push)
#pragma warning(disable : 26495)
  FIBER_DATA() : _bConvertToThread(0), _MyFiber(0) {}
#pragma warning(pop)

  unsigned long Create(unsigned __int64 dwStackCommitSize,
               unsigned __int64 dwStackReserveSize);
  unsigned long DoCallout(STACK_EXPAND pfn, void* Parameter);
};

unsigned long OnAttach();
void OnDetach();

#endif