#pragma once

#ifdef _WIN32
int mkdir(const char *path, unsigned short mode); // mode is ignored!
int execvW(const char *file, const char *const *argv);
#define execv execvW
int unlinkW(const char *file);
#define unlink unlinkW
char *strdupW(const char *str);
#define strdup strdupW
#define getcwd _getcwd
#include <direct.h>
#else
#include <unistd.h>
#endif
