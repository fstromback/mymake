#ifdef _WIN32
#include <Windows.h>
#include <process.h>

int rmdir(const char *dir) {
	if (RemoveDirectory(dir) == TRUE) return 0;
	else return -1;
}

int mkdir(const char *dir, unsigned short mode) {
	if (CreateDirectory(dir, NULL)) return 0;
	else return -1;
}

int unlinkW(const char *file) {
	if (DeleteFile(file) == TRUE) return 0;
	else return -1;
}

char *strdupW(const char *str) {
	int sz = strlen(str);
	char *toReturn = new char[sz + 1];
	toReturn[sz] = 0;
	for (int i = 0; i < sz; i++) toReturn[i] = str[i];
	return toReturn;
}

int execvW(const char *file, const char *const *argv) {
	return _execv(file, argv);
}

#endif