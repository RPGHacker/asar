#pragma once

#if defined(windows)
#	include "windows/thread-helpers-win32.h"
#else
#	include "generic/thread-helpers-pthread.h"
#endif
