#include "std.h"
#include "pipe.h"

#ifdef WINDOWS

const Pipe noPipe = INVALID_HANDLE_VALUE;

volatile LONG pipeSerialNr;

void createPipe(Pipe &read, Pipe &write, bool forChild) {
	SECURITY_ATTRIBUTES writeSa;
	writeSa.nLength = sizeof(SECURITY_ATTRIBUTES);
	writeSa.bInheritHandle = forChild ? TRUE : FALSE;
	writeSa.lpSecurityDescriptor = NULL;

	char pipeName[MAX_PATH];
	nat bufferSize = 4096;

	sprintf_s(pipeName, MAX_PATH,
			"\\\\.\\pipe\\mymake.%08x.%08x",
			GetCurrentProcessId(),
			InterlockedIncrement(&pipeSerialNr));

	read = CreateNamedPipe(
		pipeName,
		PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_WAIT,
		1,				// Number of pipes
		bufferSize,	// Out buffer size
		bufferSize,	// In buffer size
		120 * 1000,	// Timeout in ms
		NULL);

	if (read == INVALID_HANDLE_VALUE) {
		WARNING("Failed to create pipe!");
		read = noPipe;
		write = noPipe;
		return;
	}

	write = CreateFile(pipeName,
					GENERIC_WRITE,
					0,
					&writeSa,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (write == INVALID_HANDLE_VALUE) {
		CloseHandle(read);
		WARNING("Failed to create pipe!");
		read = noPipe;
		write = noPipe;
		return;
	}
}

void closePipe(Pipe end) {
	CloseHandle(end);
}

bool writePipe(Pipe to, void *data, nat size) {
	// WriteFile on a pipe ensures that either all or none of the buffer is written.
	DWORD written;
	return WriteFile(to, data, size, &written, NULL) == TRUE;
}

class PipeSet : NoCopy {
public:
	PipeSet(nat bufSize);
	~PipeSet();

	struct Data {
		Pipe pipe;
		OVERLAPPED overlapped;
		HANDLE event;
		nat size;
		char *buffer;

		Data(Pipe pipe, nat buf);
		~Data();

		void startRead();
		void read(void *to, nat &written);
	};

	typedef map<Pipe, Data *> DataMap;
	DataMap data;

	nat bufSize;

	// Linear list of all event variables.
	vector<HANDLE> events;
	vector<Data *> sources;

	void add(Pipe pipe);
	void remove(Pipe pipe);
	void read(void *to, nat &written, Pipe &from);
};

PipeSet::Data::Data(Pipe pipe, nat buf) :
	pipe(pipe),
	event(CreateEvent(NULL, TRUE, FALSE, NULL)),
	size(buf),
	buffer(new char[buf]) {

	startRead();
}

PipeSet::Data::~Data() {
	CancelIo(pipe);

	delete []buffer;
	CloseHandle(event);
}

void PipeSet::Data::startRead() {
	zeroMem(overlapped);
	overlapped.hEvent = event;

	ResetEvent(event);
	ReadFile(pipe, buffer, size, NULL, &overlapped);
}

void PipeSet::Data::read(void *to, nat &written) {
	DWORD read;
	GetOverlappedResult(pipe, &overlapped, &read, TRUE);

	memcpy_s(to, size, buffer, read);
	written = read;

	startRead();
}

PipeSet::PipeSet(nat bufSize) : bufSize(bufSize) {}

PipeSet::~PipeSet() {
	for (DataMap::iterator i = data.begin(), end = data.end(); i != end; ++i) {
		delete i->second;
	}
}

void PipeSet::add(Pipe p) {
	events.clear();
	data.insert(make_pair(p, new Data(p, bufSize)));
}

void PipeSet::remove(Pipe p) {
	DataMap::iterator i = data.find(p);
	if (i == data.end())
		return;

	events.clear();

	delete i->second;
	data.erase(i);
}

void PipeSet::read(void *to, nat &written, Pipe &from) {
	if (events.size() != data.size()) {
		events.clear();
		sources.clear();
		for (DataMap::iterator i = data.begin(), end = data.end(); i != end; ++i) {
			events << i->second->event;
			sources << i->second;
		}
	}

	Data *src = null;
	DWORD r = WaitForMultipleObjects(events.size(), &events[0], FALSE, INFINITE);
	if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + events.size()) {
		src = sources[r - WAIT_OBJECT_0];
	} else if (r >= WAIT_ABANDONED_0 && r < WAIT_ABANDONED_0 + events.size()) {
		src = sources[r - WAIT_ABANDONED_0];
	} else {
		WARNING("Failed to wait!");
		exit(11);
	}

	from = src->pipe;
	src->read(to, written);
}



PipeSet *createPipeSet(nat bufferSize) {
	return new PipeSet(bufferSize);
}

void destroyPipeSet(PipeSet *pipes) {
	delete pipes;
}

void addPipeSet(PipeSet *pipes, Pipe pipe) {
	pipes->add(pipe);
}

void removePipeSet(PipeSet *pipes, Pipe pipe) {
	pipes->remove(pipe);
}

void readPipeSet(PipeSet *pipes, void *to, nat &written, Pipe &from) {
	pipes->read(to, written, from);
}


#else

#error "Implement me!"

#endif
