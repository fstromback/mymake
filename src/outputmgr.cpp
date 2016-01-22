#include "std.h"
#include "outputmgr.h"

OutputMgr::OutputMgr() : ack(0) {
	createPipe(selfRead, selfWrite, false);
	thread.start(&OutputMgr::threadMain, *this);
}

OutputMgr::~OutputMgr() {
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
}

void OutputMgr::addPipe(Pipe pipe, const String &prefix, bool errorStream) {
	{
		Lock::Guard z(editsLock);
		PipeData *data = new PipeData(pipe, prefix, errorStream);
		toAdd.push(data);
	}

	// Notify.
	writePipe(selfWrite, "U", 1);
	ack.down();
}

void OutputMgr::removePipe(Pipe pipe) {
	{
		Lock::Guard z(editsLock);
		toRemove.push(pipe);
	}

	// Notify.
	writePipe(selfWrite, "U", 1);
	ack.down();
}

void OutputMgr::threadMain() {
	bool exit = false;
	PipeSet *pipeSet = createPipeSet(PipeData::bufferSize);
	addPipeSet(pipeSet, selfRead);

	// Output buffer.
	char buffer[PipeData::bufferSize];
	nat written;
	Pipe from;

	while (!exit) {
		readPipeSet(pipeSet, buffer, written, from);

		PipeMap::iterator it = pipes.find(from);
		if (it == pipes.end()) {
			// From selfRead.
			nat update = 0;
			for (nat i = 0; i < written; i++) {
				if (buffer[i] == 'E') {
					exit = true;
				} else if (buffer[i] == 'U') {
					update++;
				}
			}

			if (update > 0) {
				Lock::Guard z(editsLock);

				while (!toAdd.empty()) {
					PipeData *i = toAdd.front();
					toAdd.pop();
					pipes.insert(make_pair(i->pipe, i));
					addPipeSet(pipeSet, i->pipe);
				}

				while (!toRemove.empty()) {
					Pipe e = toRemove.front();
					toRemove.pop();
					removePipeSet(pipeSet, e);
					PipeMap::iterator i = pipes.find(e);
					if (i != pipes.end()) {
						delete i->second;
						pipes.erase(i);
					}
				}

				for (nat i = 0; i < update; i++)
					ack.up();
			}
		} else {
			it->second->add(buffer, written);
		}
	}

	destroyPipeSet(pipeSet);
}

OutputMgr::PipeData::PipeData(Pipe pipe, const String &prefix, bool errorStream) :
	pipe(pipe), prefix(prefix), errorStream(errorStream), bufferCount(0) {}

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
	if (errorStream) {
		PERROR(prefix << buffer);
	} else {
		PLN(prefix << buffer);
	}
	bufferCount = 0;
}

OutputMgr &OutputMgr::me() {
	static OutputMgr me;
	return me;
}

void OutputMgr::add(Pipe pipe, const String &prefix) {
	me().addPipe(pipe, prefix, false);
}

void OutputMgr::addError(Pipe pipe, const String &prefix) {
	me().addPipe(pipe, prefix, true);
}

void OutputMgr::remove(Pipe pipe) {
	me().removePipe(pipe);
}
