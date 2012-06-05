#pragma once

#ifndef _WIN32
#include <dirent.h>
#define PATH_DELIM '/'
#else
#include <windows.h>
typedef unsigned short mode_t;

#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

#define PATH_DELIM '\\'
#endif
