#pragma once

/**
 * Management of output. This file is included from std.h for convenience.
 */

#include "platform.h"
#include "sync.h"

// State for output (prefix + banner). Note: These are heap-allocated and ref-counted. When a
// thread is created, a new output state is allocated.
class OutputState {
public:
	// Constructor. Initializes to 1 ref.
	OutputState();
	OutputState(const String &banner, const String &prefix);

	// Banner, something written before the first output.
	String banner;

	// Prefix, written before each line.
	String prefix;

	// Output any needed prefixes.
	void output(ostream &to);

	// Add a reference. Returns a pointer to itself for ease of use.
	OutputState *ref();

	// Release a reference.
	void unref();

private:
	// Reference count.
	nat refs;

	// Lock for the reference count.
	Lock refLock;
};

// Synchronization for output (i.e. stdout).
extern Lock outputLock;

// Current output state.
extern THREAD OutputState *outputState;


// Set a new, temporary state: (NOTE: not recursive, eg. if we have a prefix, SetPrefix replaces the
// current one, it does not add another one).
class SetState : NoCopy {
public:
	SetState(const String &banner, const String &prefix) : oldState(outputState) {
		outputState = new OutputState(banner, prefix);
	}

	~SetState() {
		outputState->unref();
		outputState = oldState;
	}

private:
	OutputState *oldState;
};


// Print line in the context of another thread.
#define PLN_THREAD(threadState, x)				\
	do {										\
		Lock::Guard _w(outputLock);				\
		(threadState)->output(std::cout);		\
		std::cout << x << endl;					\
	} while (false)

// Print line.
#define PLN(x) PLN_THREAD(outputState, x)

// Print line to stderr in the context of another thread.
#define PERROR_THREAD(threadState, x)			\
	do {										\
		Lock::Guard _w(outputLock);				\
		(threadState)->output(std::cerr);		\
		std::cerr << x << endl;					\
	} while (false)

// Print line to stderr.
#define PERROR(x) PERROR_THREAD(outputState, x)

// Print x = <value of x>
#define PVAR(x) PLN(#x << "=" << x)

// Output warning.
#define WARNING(x) PLN("WARNING: " << x)

// TODO.
#define TODO(x) PLN("TODO: " << __FILE__ << "(" << __LINE__ << "): " << x)


// Debug level.
enum {
	dbg_QUIET = 0,
	dbg_NORMAL,
	dbg_PEDANTIC,
	dbg_COMMAND,
	dbg_INFO,
	dbg_VERBOSE,
	dbg_DEBUG,
};

#define DEBUG(x, level)							\
	do {										\
		if (debugLevel >= dbg_ ## level) {		\
			if (dbg_ ## level > dbg_NORMAL) {	\
				PLN(#level ": " << x);			\
			} else {							\
				PLN(x);							\
			}									\
		}										\
	} while (false)
