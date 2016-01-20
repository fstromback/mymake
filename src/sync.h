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


/**
 * Semaphore.
 */
class Sema : NoCopy {
public:
	// Create, setting initial count. Negative numbers block.
	Sema(nat initial = 0);

	// Destroy.
	~Sema();

	// Up the sema; maybe releasing a thread.
	void up();

	// Down the sema; maybe blocking.
	void down();

private:
#ifdef WINDOWS
	HANDLE sema;
#else
	sem_t sema;
#endif
};


/**
 * Condition variable. Once it has been signaled, it stays that way.
 */
class Condition : NoCopy {
public:
	// Create.
	Condition();

	// Wait.
	void wait();

	// Signal.
	void signal();

private:
	// Semaphore for the waiting.
	Sema sema;
};
