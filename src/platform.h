#pragma once

/**
 * This file contains macros for finding out which platform
 * we are currently being compiled on. This file is designed
 * to not depend on anything, and can therefore safely be included
 * from anywhere.
 */

/**
 * Machine type defines:
 * X86 - x86 cpu
 * X64 - x86-64/amd64 cpu
 */

/**
 * Platform defined:
 * LINUX - compiled on linux
 * POSIX - compiled on posix-compatible platform (eg linux)
 * WINDOWS - compiled on windows
 */

/**
 * Endian-ness: Look in Endian.h for more helpers regarding endianness.
 * LITTLE_ENDIAN
 * BIG_ENDIAN
 */


/**
 * Compilers:
 * VISUAL_STUDIO - Visual Studio compiler. Set to the version, eg 2008 for VS2008.
 * GCC - GCC compiler, TODO: version?
 */


// Detect the current architecture and platform.
#if defined(_WIN64)
#define X64
#define WINDOWS
#elif defined(_WIN32)
#define X86
#define WINDOWS
#else
#error "Unknown platform, please add it here!"
#endif

// Detect the current compiler.
#if defined(_MSC_VER)
// Visual Studio compiler!
#if _MSC_VER >= 1800
#define VISUAL_STUDIO 2013
#elif _MSC_VER >= 1700
#define VISUAL_STUDIO 2012
#elif _MSC_VER >= 1600
#define VISUAL_STUDIO 2010
#elif _MSC_VER >= 1500
#define VISUAL_STUDIO 2008
#else
#error "Too early VS version, earliest supported is VS2008"
#endif

#elif defined(__GNUC__)
// GCC
#define GCC __GNUC__

#endif

#if defined(X86) || defined(X64)
#define LITTLE_ENDIAN
#else
#error "Unknown endianness for your platform. Define either LITTLE_ENDIAN or BIG_ENDIAN here."
#endif
