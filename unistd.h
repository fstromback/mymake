#pragma once

#ifdef _WIN32
int rmdir(const char *dir);
int mkdir(const char *path, unsigned short mode); //mode ignoreras!
int execvW(const char *file, const char *const *argv);
#define execv execvW
int unlinkW(const char *file);
#define unlink unlinkW
char *strdupW(const char *str);
#define strdup strdupW
#else
#include <unistd.h>
#endif
