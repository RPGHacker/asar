#pragma once

#include <stdint.h>
#include <pthread.h>

struct function_pointer_wrapper /*have this struct at global level*/
{
	static void* (*thread_callback)(void*);
	static void* execute_thread(void* parameter) { return thread_callback(parameter); }
};

void* (*function_pointer_wrapper::thread_callback)(void*) = nullptr;

template <typename functor>
bool run_as_thread(functor&& callback) {
	struct thread_wrapper {
		functor& callback;
		bool result;

		void* execute() {
			result = callback();
			if (result)
				pthread_exit(NULL);
			else
				pthread_exit(reinterpret_cast<void*>((uintptr_t)-1));
		}

	} wrapper{ callback, false };

	function_pointer_wrapper::thread_callback = [](void* parameter) {
		return reinterpret_cast<thread_wrapper*>(parameter)->execute();
	};
	
	pthread_attr_t settings;                                                         
	pthread_t thread_id;
	int ret;
	
	ret = pthread_attr_init(&settings);                                               
	if (ret == -1){
		return callback();
	}
	
	ret = pthread_attr_setstacksize(&settings, 16 * 1024 * 1024);                                               
	if (ret == -1){
		return callback();
	}

    ret = pthread_create(&thread_id, &settings, &function_pointer_wrapper::execute_thread, &wrapper);
	if (ret == -1){
		return callback();
	}

	void* thread_ret;
	pthread_join(thread_id, &thread_ret);

	return wrapper.result;
}

#ifndef NO_USE_THREADS
void* stack_bottom = nullptr;
void init_stack_use_check() {
	pthread_attr_t attrs;
	size_t stack_size = 0;
	pthread_getattr_np(pthread_self(), &attrs);
	pthread_attr_getstack(&attrs, &stack_bottom, &stack_size);
	pthread_attr_destroy(&attrs);
}
void deinit_stack_use_check() {
	stack_bottom = nullptr;
}
bool have_enough_stack_left() {
	// using a random local as a rough estimate for current top-of-stack
	char stackvar;
	size_t stack_left = &stackvar - (char*)stack_bottom;
	return stack_bottom == nullptr || stack_left >= 32768;
}
#endif
