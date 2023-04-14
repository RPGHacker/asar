#pragma once

#if defined(_WIN32)

#include <windows.h>

struct function_pointer_wrapper /*have this struct at global level*/
{
	static void (*fiber_callback)(void *);
	static void __stdcall execute_fiber(void* parameter) { return fiber_callback(parameter); }

	static unsigned long (*thread_callback)(void*);
	static unsigned long __stdcall execute_thread(void* parameter) { return thread_callback(parameter); }
};

void (*function_pointer_wrapper::fiber_callback)(void*) = nullptr;
unsigned long (*function_pointer_wrapper::thread_callback)(void*) = nullptr;

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

	function_pointer_wrapper::fiber_callback = [](void *parameter){
		reinterpret_cast<fiber_wrapper*>(parameter)->execute();
	};
	auto fiber = CreateFiberEx(16*1024*1024, 16*1024*1024, 0, &function_pointer_wrapper::execute_fiber, &wrapper);

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


template <typename functor>
bool run_as_thread(functor&& callback) {
	struct thread_wrapper {
		functor& callback;
		bool result;

		unsigned long execute() {
			result = callback();
			return (result ? 0 : -1);
		}

	} wrapper{ callback, false };

	function_pointer_wrapper::thread_callback = [](void* parameter) {
		return reinterpret_cast<thread_wrapper*>(parameter)->execute();
	};

	auto thread = CreateThread(NULL, 16 * 1024 * 1024, &function_pointer_wrapper::execute_thread, &wrapper, 0, NULL);

	if (!thread) {
		return callback();
	}

	WaitForSingleObject(thread, INFINITE);

	CloseHandle(thread);

	return wrapper.result;
}

bool have_enough_stack_left() {
	MEMORY_BASIC_INFORMATION mbi;
	char stackvar;
	VirtualQuery(&stackvar, &mbi, sizeof(mbi));
	char* stack_bottom = (char*)mbi.AllocationBase;
	return (&stackvar - stack_bottom) >= 32768;
}
#endif
