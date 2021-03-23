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
		// Init.
		Data(Pipe pipe, nat buffer);
		~Data();

		// OVERLAPPED request data
		OVERLAPPED request;

		// Copy of the pipe handle so that we can find it.
		Pipe pipe;

		// Buffer.
		char *buffer;

		// Buffer size.
		nat size;

		// Any error?
		bool error;

		// Start reading from this pipe (port is the IO completion port so that we can post errors there).
		void startRead(HANDLE port);

		// Destroy this element. Makes sure that any pending IO operations are finished before
		// deallocating the object. Returns true if we can deallocate now.
		bool destroy();
	};

	typedef map<Pipe, Data *> DataMap;
	DataMap data;

	// IO completion port.
	HANDLE port;

	nat bufSize;

	void add(Pipe pipe);
	void remove(Pipe pipe);
	void read(void *to, nat &written, Pipe &from);
};

PipeSet::Data::Data(Pipe pipe, nat buf) :
	pipe(pipe),
	buffer(new char[buf]),
	size(buf),
	error(false) {
}

PipeSet::Data::~Data() {
	delete []buffer;
}

void PipeSet::Data::startRead(HANDLE port) {
	if (error)
		return;

	// Clear out the OVERLAPPED structure.
	zeroMem(request);

	if (ReadFile(pipe, buffer, size, NULL, &request) == FALSE) {
		int error = GetLastError();

		if (error == ERROR_IO_PENDING || error == 0) {
			// All is well.
		} else {
			// Some error. Perhaps we should signal the IO completion port so that we can be removed?
			error = true;
			PostQueuedCompletionStatus(port, 0, (ULONG_PTR)this, NULL);
		}
	}
}

bool PipeSet::Data::destroy() {
	if (error) {
		return true;
	} else {
		CancelIo(pipe);
		return false;
	}
}


PipeSet::PipeSet(nat bufSize) : bufSize(bufSize) {
	port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
}

PipeSet::~PipeSet() {
	for (DataMap::iterator i = data.begin(), end = data.end(); i != end; ++i) {
		delete i->second;
	}
	CloseHandle(port);
}

void PipeSet::add(Pipe p) {
	Data *d = new Data(p, bufSize);
	data.insert(make_pair(p, d));
	HANDLE nPort = CreateIoCompletionPort(p, port, (ULONG_PTR)d, 1);
	if (nPort == NULL) {
		WARNING("Failed to associate the port to the handle: " << GetLastError());
		exit(11);
	}
	port = nPort;

	d->startRead(port);
}

void PipeSet::remove(Pipe p) {
	DataMap::iterator i = data.find(p);
	if (i == data.end())
		return;

	if (i->second->destroy()) {
		delete i->second;
		data.erase(i);
	}
}

void PipeSet::read(void *to, nat &written, Pipe &from) {
	from = noPipe;
	written = 0;

	DWORD bytes = 0;
	Data *src = null;
	BOOL ok = FALSE;

	while (true) {
		ULONG_PTR key = 0;
		OVERLAPPED *request = null;
		ok = GetQueuedCompletionStatus(port, &bytes, &key, &request, INFINITE);

		if (!key) {
			WARNING(L"Failed to wait: " << GetLastError());
			exit(11);
		}

		src = (Data *)key;
		from = src->pipe;

		// Data from some old pipe that were still used?
		if (data.count(from) != 0) {
			break;
		}
	}

	if (ok == false || src->error || bytes == 0) {
		// If it returned an error or EOF, remove it now.
		written = 0;
		data.erase(from);
		delete src;
	} else {
		// Otherwise, extract the data.
		written = bytes;
		memcpy_s(to, bufSize, src->buffer, bytes);

		// Start the next read operation.
		src->startRead(port);
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
			written = 0;
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
