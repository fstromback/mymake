#include "process.h"

const size_t bufferSize = 1024;

struct WriteData {
	istream &stream;
	HANDLE handle;
};

DWORD WINAPI writeThread(void *data) {
	WriteData *d = (WriteData *)data;
	char buffer[bufferSize];
	DWORD filled;
	DWORD written;

	d->stream.read(buffer, bufferSize);
	filled = (DWORD)d->stream.gcount();
	while (filled > 0) {
		if (!WriteFile(d->handle, buffer, filled, &written, NULL)) {
			cout << "Error: " << GetLastError() << endl;
			CloseHandle(d->handle);
			return 0;
		}

		if (written < filled) {
			memmove(buffer, buffer + written, filled - written);
			filled -= written;
		} else {
			d->stream.read(buffer, bufferSize);
			filled = (DWORD)d->stream.gcount();
		}
	}

	CloseHandle(d->handle);
	return 0;
}

struct ReadData {
	ostream &stream;
	HANDLE handle;
};

DWORD WINAPI readThread(void *data) {
	ReadData *d = (ReadData *)data;
	char buffer[bufferSize];
	DWORD written;

	while (ReadFile(d->handle, buffer, bufferSize, &written, NULL)) {
		d->stream.write(buffer, written);

		if (written == 0)
			break;
	}

	CloseHandle(d->handle);
	return 0;
}

void runProcess(const string &exe, const string &cmd, istream &inStream, ostream &outStream) {
	STARTUPINFO startup;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;

	SECURITY_ATTRIBUTES writeSa;
	writeSa.nLength = sizeof(SECURITY_ATTRIBUTES);
	writeSa.bInheritHandle = TRUE;
	writeSa.lpSecurityDescriptor = NULL;

	HANDLE input, output;
	CreatePipe(&startup.hStdInput, &input, &writeSa, 0);
	CreatePipe(&output, &startup.hStdOutput, &writeSa, 0);
	startup.hStdError = startup.hStdOutput;

	SetHandleInformation(input, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(output, HANDLE_FLAG_INHERIT, 0);

	PROCESS_INFORMATION info;
	char *cmdline = new char[cmd.size() + 1];
	memcpy(cmdline, cmd.c_str(), cmd.size() + 1);
	if (CreateProcess(exe.c_str(), cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &info) == 0) {
		cout << "Failed to start " << exe << endl;
		exit(1);
	}
	delete []cmdline;

	CloseHandle(startup.hStdInput);
	CloseHandle(startup.hStdOutput);
	CloseHandle(info.hThread);

	WriteData wd = { inStream, input };
	ReadData rd = { outStream, output };

	HANDLE wThread = CreateThread(NULL, 0, &writeThread, &wd, 0, NULL);
	HANDLE rThread = CreateThread(NULL, 0, &readThread, &rd, 0, NULL);

	WaitForSingleObject(info.hProcess, INFINITE);
	WaitForSingleObject(wThread, INFINITE);
	WaitForSingleObject(rThread, INFINITE);

	CloseHandle(info.hProcess);
}
