#if defined(_WIN32)
typedef unsigned long(_stdcall *STACK_EXPAND)(void *Parameter);
class FIBER_DATA {
public:
  void *m_PrevFiber, *m_CurrentFiber;
  STACK_EXPAND m_Callback;
  void *m_CallbackParam;
  unsigned long m_CallbackError;
  int m_ConvertToThread;

  static void _stdcall FiberProc(void *lpParameter);

  void m_FiberProc();

public:
  ~FIBER_DATA();

  FIBER_DATA()
      : m_PrevFiber(0), m_CurrentFiber(0), m_Callback(nullptr),
        m_CallbackParam(nullptr), m_CallbackError(0), m_ConvertToThread(0) {}

  unsigned long Create(unsigned long long dwStackCommitSize,
                       unsigned long long dwStackReserveSize);
  unsigned long m_DoCallback(STACK_EXPAND Callback, void *Parameter);
};

unsigned long OnAttach();
void OnDetach();
unsigned long DoCallback(STACK_EXPAND Callback, void *Parameter);

#endif