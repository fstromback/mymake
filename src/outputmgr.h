#pragma once
#include "pipe.h"
#include "thread.h"
#include "sync.h"

/**
 * Output manager that multiplexes output from child processes line by line so that output does not
 * get intertwined. Also supports adding a prefix to each line, so that it is possible to see which
 * compilation process it comes from.
 */
class OutputMgr : NoCopy {
public:
	// Clean up.
	~OutputMgr();

	// Add pipe. The pipe will be closed eventually.
	static void add(Pipe pipe, OutputState *state);
	static void addError(Pipe pipe, OutputState *state);

	// Remove pipe.
	// Not anymore: Waits until all output from the pipe is properly flushed (= the other end is closed).
	static void remove(Pipe pipe);

	// Clean up all data in the manager, ensuring that all data is flushed.
	static void shutdown();

private:
	// Disallow creation.
	OutputMgr();

	// Thread used for multiplexing.
	Thread thread;

	// Pipe to ourselves, used for communicating to the worker thread.
	Pipe selfWrite, selfRead;

	// Data for one pipe.
	struct PipeData {
		// Pipe.
		Pipe pipe;

		// Output state.
		OutputState *state;

		// To standard error?
		bool errorStream;

		// Buffer size.
		enum {
			bufferSize = 1024
		};

		// Buffer. Any unfinished lines are stored here.
		char buffer[bufferSize + 1];

		// Current number of chars in the buffer.
		nat bufferCount;

		// Create.
		inline PipeData(Pipe pipe, OutputState *state, bool errorStream);

		// Destroy.
		~PipeData();

		// Data arrived!
		void add(const char *src, nat size);

		// Flush buffer to stdout. Add newline.
		void flush();
	};

	// Queued items for 'toRemove'.
	struct RemoveData {
		// Pipe to remove.
		Pipe pipe;

		// Semaphore used to signal when deletion is complete.
		// We don't wait for the pipes to close on removal, that hangs the system in some cases.
		// The worst thing that happens is that some output is delayed a bit, we wont lose any
		// output as we wait for all pipes to become empty before removing them at least.
		// Sema *wake;
	};

	// All current pipes. Only used from our thread.
	typedef map<Pipe, PipeData *> PipeMap;
	PipeMap pipes;

	// Queue for edits to 'pipes'.
	queue<PipeData *> toAdd;
	queue<RemoveData> toRemove;

	// Queue for wakes from 'pipes'.
	queue<Sema *> wake;

	// Lock for 'toAdd' and 'toRemove'.
	Lock editsLock;

	// Running?
	bool running;

	// Our global instance.
	static OutputMgr me;

	// Helpers for add and remove.
	void addPipe(Pipe pipe, OutputState *state, bool errorStream);
	void removePipe(Pipe pipe);

	// Main function for the thread.
	void threadMain();

	void shutdownMe();
};
