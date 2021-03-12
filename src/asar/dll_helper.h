#if defined(_WIN32)
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

  FIBER_DATA()
      : _PrevFiber(0), _MyFiber(0), _pfn(nullptr), _Parameter(nullptr), _dwError(0), _bConvertToThread(0) {}

  unsigned long Create(unsigned __int64 dwStackCommitSize,
               unsigned __int64 dwStackReserveSize);
  unsigned long DoCallout(STACK_EXPAND pfn, void* Parameter);
};

unsigned long OnAttach();
void OnDetach();
unsigned long DoCallout(STACK_EXPAND pfn, void* Parameter);

#endif