#if defined(_WIN32)

#include <windows.h>

template <typename ret> 
struct fiber_result {
    ULONG status;
    ret retval;
};

template <typename functor, typename ret = std::result_of<functor()>::type> 
ret run_as_fiber(functor &&callback) {
    static_assert(std::is_default_constructible<ret>::value, "Return value of callback must be default constructible");
    struct fiber_wrapper {
        functor &callback;
        void *original;
        ret result;

        void execute() {
            result = callback();
            SwitchToFiber(original);
        }

    } wrapper{callback, nullptr, ret{}};

    auto fiber = CreateFiberEx(
        4 * 1024 * 1024, 8 * 1024 * 1024, 0,
        [](void *parameter) {
          reinterpret_cast<fiber_wrapper *>(parameter)->execute();
        },
        &wrapper);

    if (!fiber) {
        return callback();
    }

    void *main_thread = nullptr;
    if (!IsThreadAFiber()) { // this should probably always be true I suspect, unless the host does something weird
        main_thread = ConvertThreadToFiber(nullptr);

        if (!main_thread) {
            return callback();
        }
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