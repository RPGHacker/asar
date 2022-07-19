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

size_t check_stack_left() {
	pthread_attr_t attrs;
	void *stack_start;
	size_t stack_size;

	pthread_getattr_np(pthread_self(), &attrs);
	pthread_attr_getstack(&attrs, &stack_start, &stack_size);
	pthread_attr_destroy(&attrs);

	// using a random local as a rough estimate for current top-of-stack
	size_t stack_left = (char*)&stack_size - (char*)stack_start;
	return stack_left;
	if(stack_left < 32768) asar_throw_error(pass, error_type_fatal, error_id_recursion_limit);
}
