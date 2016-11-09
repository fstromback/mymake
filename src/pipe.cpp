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

bool writePipe(Pipe to, const void *data, nat size) {
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
		bool error;

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
	buffer(new char[buf]),
	error(false) {

	startRead();
}

PipeSet::Data::~Data() {
	CancelIo(pipe);

	delete []buffer;
	CloseHandle(event);
}

void PipeSet::Data::startRead() {
	if (error)
		return;

	zeroMem(overlapped);
	overlapped.hEvent = event;

	ResetEvent(event);
	if (ReadFile(pipe, buffer, size, NULL, &overlapped) == FALSE) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// Arrange so that we're in a signaling state and will report 0 bytes next time.
			error = true;
			SetEvent(event);
		}
	}
}

void PipeSet::Data::read(void *to, nat &written) {
	if (error) {
		written = 0;
	} else {
		DWORD read;
		GetOverlappedResult(pipe, &overlapped, &read, TRUE);

		memcpy_s(to, size, buffer, read);
		written = read;

		startRead();
	}
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
	from = noPipe;
	written = 0;

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

#include <sys/select.h>

const Pipe noPipe = -1;

void createPipe(Pipe &read, Pipe &write, bool) {
	Pipe out[2];
	if (pipe(out)) {
		perror("Failed to create pipe: ");
		exit(10);
	}

	read = out[0];
	write = out[1];
}

void closePipe(Pipe end) {
	close(end);
}

bool writePipe(Pipe to, const void *data, nat size) {
	char *d = (char *)data;
	nat at = 0;
	while (at < size) {
		ssize_t s = write(to, d + at, size - at);
		if (s < 0)
			return false;

		at += s;
	}

	return true;
}

class PipeSet : NoCopy {
public:
	PipeSet(nat bufferSize);
	~PipeSet();

	nat bufferSize;
	fd_set fds;
	int maxFd;

	int lastFd;

	void add(Pipe pipe);
	void remove(Pipe pipe);
	void read(void *to, nat &written, Pipe &from);
};

PipeSet::PipeSet(nat bufferSize) : bufferSize(bufferSize), maxFd(0), lastFd(0) {
	FD_ZERO(&fds);
}

PipeSet::~PipeSet() {}

void PipeSet::add(Pipe p) {
	FD_SET(p, &fds);
	maxFd = max(maxFd, p + 1);
}

void PipeSet::remove(Pipe p) {
	FD_CLR(p, &fds);
	while (maxFd > 0) {
		if (FD_ISSET(maxFd - 1, &fds))
			break;
		maxFd--;
	}
}

void PipeSet::read(void *to, nat &written, Pipe &from) {
	written = 0;
	from = noPipe;

	if (maxFd == 0) {
		WARNING("No fds to read from!");
		sleep(1);
		return;
	}

	fd_set now = fds;
	int ready = select(maxFd, &now, null, null, null);
	if (ready == -1) {
		if (errno != EINTR) {
			perror("Failed to wait using select: ");
			exit(11);
		}

		// No big deal if we happen to wake up...
		return;
	}

	if (ready == 0) {
		printf("Something wrong with select...\n");
		sleep(1);
		return;
	}

	while (true) {
		if (++lastFd >= maxFd)
			lastFd = 0;

		if (!FD_ISSET(lastFd, &now))
			continue;

		// Found one! Read from it.
		from = lastFd;
		ssize_t r = ::read(from, to, bufferSize);
		if (r == 0) {
			// Closed fd. Ignore it until the manager detects it is closed.
			remove(lastFd);
		} else if (r < 0) {
			written = 0;
		} else {
			written = r;
		}

		// Done!
		break;
	}
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

#endif
