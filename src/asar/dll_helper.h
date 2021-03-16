#if defined(_WIN32)

#include <windows.h>

struct function_pointer_wrapper/*have this struct at global level*/
{
	static void (*callback)(void *);
	static void __stdcall execute(void *parameter) { return callback(parameter); }
};

void (*function_pointer_wrapper::callback)(void *)  = nullptr;

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

	function_pointer_wrapper::callback = [](void *parameter){
		reinterpret_cast<fiber_wrapper*>(parameter)->execute();
	};
	auto fiber = CreateFiberEx(4*1024*1024, 8*1024*1024, 0, &function_pointer_wrapper::execute, &wrapper);

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
