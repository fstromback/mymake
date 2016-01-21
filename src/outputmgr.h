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

	// Add pipe.
	static void add(Pipe pipe, const String &prefix);
	static void addError(Pipe pipe, const String &prefix);

	// Remove pipe.
	static void remove(Pipe pipe);

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

		// Prefix.
		String prefix;

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
		inline PipeData(Pipe pipe, const String &prefix, bool errorStream);

		// Destroy.
		~PipeData();

		// Data arrived!
		void add(const char *src, nat size);

		// Flush buffer to stdout. Add newline.
		void flush();
	};

	// All current pipes.
	typedef map<Pipe, PipeData *> PipeMap;
	PipeMap pipes;

	// Queue for edits to 'pipes'.
	queue<PipeData *> toAdd;
	queue<Pipe> toRemove;

	// Lock for 'toAdd' and 'toRemove'.
	Lock editsLock;

	// Get global instance.
	static OutputMgr &me();

	// Helpers for add and remove.
	void addPipe(Pipe pipe, const String &prefix, bool errorStream);
	void removePipe(Pipe pipe);

	// Main function for the thread.
	void threadMain();
};
