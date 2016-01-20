#pragma once

/**
 * Platform agnostic lock.
 */
class Lock : NoCopy {
public:
	// Create.
	Lock();

	// Destroy.
	~Lock();

	// Guard.
	class Guard : NoCopy {
	public:
		// Take the lock.
		Guard(Lock &lock);

		// Release the lock.
		~Guard();

	private:
		// Locked lock.
		Lock &lock;
	};

private:
#ifdef WINDOWS
	CRITICAL_SECTION lock;
#else
	pthread_mutex_t lock;
#endif
};
