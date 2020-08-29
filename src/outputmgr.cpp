#include "std.h"
#include "outputmgr.h"

OutputMgr::OutputMgr() {
	createPipe(selfRead, selfWrite, false);
	thread.start(&OutputMgr::threadMain, *this);
	running = true;
}

OutputMgr::~OutputMgr() {
	if (running)
		shutdownMe();
}

void OutputMgr::shutdownMe() {
	// Notify exit.
	writePipe(selfWrite, "E", 1);
	thread.join();

	// Clean up any remaints.
	for (PipeMap::iterator i = pipes.begin(), end = pipes.end(); i != end; ++i) {
		delete i->second;
	}

	while (!toAdd.empty()) {
		delete toAdd.front();
		toAdd.pop();
	}

	running = false;
}

void OutputMgr::addPipe(Pipe pipe, OutputState *state, bool errorStream) {
	Sema ack(0);

	{
		Lock::Guard z(editsLock);
		PipeData *data = new PipeData(pipe, state, errorStream);
		toAdd.push(data);
		wake.push(&ack);
	}

	// Notify and wait.
	writePipe(selfWrite, "U", 1);
	ack.down();
}

void OutputMgr::removePipe(Pipe pipe) {
	Sema ack(0);

	{
		Lock::Guard z(editsLock);
		RemoveData r = { pipe };
		toRemove.push(r);
		wake.push(&ack);
	}

	// Notify and wait.
	writePipe(selfWrite, "U", 1);
	ack.down();
}

void OutputMgr::threadMain() {
	bool exit = false;
	PipeSet *pipeSet = createPipeSet(PipeData::bufferSize);
	addPipeSet(pipeSet, selfRead);

	// Remember which threads have terminated recently.
	set<Pipe> terminated;

	// Remember which pipes shall be closed soon.
	set<Pipe> terminate;

	// Output buffer.
	char buffer[PipeData::bufferSize];
	nat written;
	Pipe from;

	// Keep going until we have forwarded all output (at least the one the remainder of the system cared to close).
	while (!exit || !terminate.empty()) {
		readPipeSet(pipeSet, buffer, written, from);

		PipeMap::iterator it = pipes.find(from);
		if (it == pipes.end()) {
			// From selfRead.
			bool update = false;
			for (nat i = 0; i < written; i++) {
				if (buffer[i] == 'E') {
					exit = true;
				} else if (buffer[i] == 'U') {
					update = true;
				}
			}

			if (update) {
				Lock::Guard z(editsLock);

				while (!toAdd.empty()) {
					PipeData *i = toAdd.front();
					toAdd.pop();
					pipes.insert(make_pair(i->pipe, i));
					addPipeSet(pipeSet, i->pipe);
				}

				while (!toRemove.empty()) {
					RemoveData e = toRemove.front();
					toRemove.pop();

					if (terminated.count(e.pipe)) {
						// Already terminated.
						terminated.erase(e.pipe);
						// Close the pipe. Now it is safe!
						PipeMap::iterator i = pipes.find(e.pipe);
						if (i == pipes.end())
							WARNING(L"The pipe " << e.pipe << L" was removed too early!");
						delete i->second;
						pipes.erase(i);
					} else {
						// Wait for termination.
						terminate.insert(e.pipe);
					}
				}

				while (!wake.empty()) {
					wake.front()->up();
					wake.pop();
				}
			}
		} else if (written != 0) {
			// We got some data!
			it->second->add(buffer, written);
		} else {
			// End of stream!
			removePipeSet(pipeSet, from);

			// Someone waiting for us?
			set<Pipe>::iterator i = terminate.find(from);
			if (i != terminate.end()) {
				// Yes, then we can remove it already.
				terminate.erase(i);

				// Remove the pipe.
				delete it->second;
				pipes.erase(it);
			} else {
				// No, but someone will come eventually!
				terminated.insert(from);
				// Note: We're not closing the pipe until someone has waited on the pipe, as the
				// pipe's identifier could be reused by the operating system, which causes us to
				// deadlock.
			}
		}
	}

	destroyPipeSet(pipeSet);
}

OutputMgr::PipeData::PipeData(Pipe pipe, OutputState *state, bool errorStream) :
	pipe(pipe), state(state), errorStream(errorStream), bufferCount(0) {}

OutputMgr::PipeData::~PipeData() {
	closePipe(pipe);

	if (bufferCount > 0)
		flush();
}

void OutputMgr::PipeData::add(const char *src, nat size) {
	for (nat i = 0; i < size; i++) {
		if (src[i] == '\n') {
			flush();
		} else if (bufferCount >= bufferSize) {
			flush();
			buffer[bufferCount++] = src[i];
		} else {
			buffer[bufferCount++] = src[i];
		}
	}
}

void OutputMgr::PipeData::flush() {
	buffer[bufferCount] = 0;
	if (state) {
		if (errorStream) {
			PERROR_THREAD(*state, buffer);
		} else {
			PLN_THREAD(*state, buffer);
		}
	} else {
		if (errorStream) {
			PERROR(buffer);
		} else {
			PLN(buffer);
		}
	}
	bufferCount = 0;
}

OutputMgr OutputMgr::me;

void OutputMgr::add(Pipe pipe, OutputState *state) {
	me.addPipe(pipe, state, false);
}

void OutputMgr::addError(Pipe pipe, OutputState *state) {
	me.addPipe(pipe, state, true);
}

void OutputMgr::remove(Pipe pipe) {
	me.removePipe(pipe);
}

void OutputMgr::shutdown() {
	me.shutdownMe();
}
