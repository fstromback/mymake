#include "std.h"
#include "atomic.h"

#ifdef WINDOWS

nat atomicInc(volatile nat &v) {
	if (sizeof(v) == 4) {
		return (nat)InterlockedIncrement((LONG volatile *)&v) - 1;
	} else {
		return (nat)InterlockedIncrement64((LONGLONG volatile *)&v) - 1;
	}
}

#else

nat atomicInc(volatile nat &v) {
	return __sync_fetch_and_add(&v, 1);
}

#endif
