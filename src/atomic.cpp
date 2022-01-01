#include "std.h"
#include "atomic.h"

#ifdef WINDOWS

#include <intrin.h>

#if defined(_WIN64)

nat atomicInc(volatile nat &v) {
	return (nat)_InterlockedIncrement64((LONGLONG volatile *)&v) - 1;
}

void atomicWrite(volatile nat &v, nat write) {
	InterlockedExchange64((LONGLONG volatile *)&v, write);
}

nat atomicRead(volatile nat &v) {
	return (nat)_InterlockedOr64((LONGLONG volatile *)&v, 0);
}

#elif defined(_WIN32)

nat atomicInc(volatile nat &v) {
	return (nat)_InterlockedIncrement((LONG volatile *)&v) - 1;
}

void atomicWrite(volatile nat &v, nat write) {
	InterlockedExchange((LONG volatile *)&v, write);
}

nat atomicRead(volatile nat &v) {
	return (nat)_InterlockedOr((LONG volatile *)&v, 0);
}

#endif

#else

nat atomicInc(volatile nat &v) {
	return __sync_fetch_and_add(&v, 1);
}

void atomicWrite(volatile nat &v, nat write) {
	// Note: There may be better ways of doing this.
	__sync_synchronize();
	v = write;
	__sync_synchronize();
}

nat atomicRead(volatile nat &v) {
	return __sync_fetch_and_add(&v, 0);
}

#endif
