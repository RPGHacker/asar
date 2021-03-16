#if defined(_WIN32)

#include <windows.h>

template <typename functor> 
bool run_as_fiber(functor &&callback) {
    struct fiber_wrapper {
        functor &callback;
        void *original;
        bool result;

        void execute() {
            result = callback();
            SwitchToFiber(original);
        }

    } wrapper{callback, nullptr, false};

    auto fiber = CreateFiberEx(
        4 * 1024 * 1024, 8 * 1024 * 1024, 0,
        [](void *parameter) {
          reinterpret_cast<fiber_wrapper *>(parameter)->execute();
        },
        &wrapper);

    if (!fiber) {
        return callback();
    }

    void* main_thread = ConvertThreadToFiber(nullptr);
    if (!main_thread && GetLastError() != ERROR_ALREADY_FIBER) { 
            return callback();
    }
    wrapper.original = GetCurrentFiber();
    SwitchToFiber(fiber);
    DeleteFiber(fiber);
    if (main_thread) {
        ConvertFiberToThread();
    }
    return wrapper.result;
}
#endif