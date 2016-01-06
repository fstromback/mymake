#pragma once

#include "platform.h"

// Include the windows header if needed.
#ifdef WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Sometimes re-defines 'small' to 'char', no good!
#ifdef small
#undef small
#endif

// We need to remove their min and max macros...
#ifdef min
#undef min
#endif

#ifdef min
#undef min
#endif

#endif

/**
 * Core mymake definitions. Should be included everywhere. Good candidate for a precompiled header.
 */

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <exception>

// Convenient name for size_t.
typedef size_t nat;

// Note: nat64 may be the same as nat.
typedef __int64 int64;
typedef unsigned __int64 nat64;

// String type that follows our naming convention.
typedef std::string String;

// Exception.
typedef std::exception Error;

// Using various things from std.
using std::ostream;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::min;
using std::max;
using std::make_pair;
using std::pair;

/**
 * Class that disables copying and gives virtual destructor.
 */
class NoCopy {
public:
	NoCopy();
	virtual ~NoCopy();

private:
	// These are not even implemented as they should never be called.
	NoCopy(const NoCopy &);
	NoCopy &operator =(const NoCopy &);
};

// Add to vector.
template <class T>
inline vector<T> &operator <<(vector<T> &to, const T &elem) {
	to.push_back(elem);
	return to;
}

// Convert to number.
template <class T>
T to(const String &s) {
	std::istringstream ss(s);
	T t = T();
	ss >> t;
	return t;
}

// Convert to string.
inline const String &toS(const String &s) {
	return s;
}

template <class T>
String toS(const T &v) {
	std::ostringstream to;
	to << v;
	return to.str();
}

// Join strings.
template <class T>
void join(ostream &to, const T &data, const String &between = L", ") {
	T::const_iterator i = data.begin();
	T::const_iterator end = data.end();
	if (i == end)
		return;

	to << toS(*i);
	for (++i; i != end; ++i) {
		to << between << toS(*i);
	}
}

template <class T>
String join(const T &data, const String &between = L", ") {
	std::ostringstream to;
	join(to, data, between);
	return to.str();
}

// Tristate.
enum Tristate {
	tUnset,
	tNo,
	tYes,
};

// # of elements in array.
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))

// Output macros.

// Print line.
#define PLN(x) std::cout << x << endl

// Print x = <value of x>
#define PVAR(x) PLN(#x << " = " << x)

// TODO.
#define TODO(x) PLN("TODO: " << __FILE__ << "(" << __LINE__ << "): " << x)


// Debug level.
enum {
	dbg_QUIET = 0,
	dbg_NORMAL = 1,
	dbg_INFO = 2,
	dbg_VERBOSE = 3,
};

// Debug level.
extern int debugLevel;

#define DEBUG(x, level)							\
	do {										\
		if (debugLevel >= dbg_ ## level) {		\
			PLN(x);								\
		}										\
	} while (false)
