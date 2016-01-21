#pragma once

/**
 * This file contains macros for finding out which platform
 * we are currently being compiled on. This file is designed
 * to not depend on anything, and can therefore safely be included
 * from anywhere.
 */

/**
 * Platform defined:
 * UNIX - compiled on unix-compatible platform (eg linux)
 * WINDOWS - compiled on windows
 */

/**
 * Compilers:
 * VISUAL_STUDIO - Visual Studio compiler. Set to the version, eg 2008 for VS2008.
 * GCC - GCC compiler, TODO: version?
 */


// Detect OS version.
#if defined(_WIN32)
#define WINDOWS
#elif defined(__unix__)
#define UNIX
#else
#error "Unknown os!"
#endif

// Detect compiler
#if defined(_MSC_VER)
#define VISUAL_STUDIO
#elif defined(__GNUC__)
#define GCC
#else
#error "Unknown compiler!"
#endif

#if defined(VISUAL_STUDIO)
#define THREAD __declspec(thread)
#elif defined(GCC)
#define THREAD __thread
#endif
