// #pragma once // emits warning on GCC
#ifndef MYMAKE_STD_H
#define MYMAKE_STD_H

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

#ifdef max
#undef max
#endif

#else

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#endif

/**
 * Core mymake definitions. Should be included everywhere. Good candidate for a precompiled header.
 */

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <fstream>
#include <exception>

// Convenient name for size_t.
typedef size_t nat;

// Note: nat64 may be the same as nat.
#ifdef VISUAL_STUDIO
typedef __int64 int64;
typedef unsigned __int64 nat64;
#else
#include <inttypes.h>
typedef int64_t int64;
typedef uint64_t nat64;
#endif

// String type that follows our naming convention.
typedef std::string String;

// Exception.
typedef std::exception Error;

// Using various things from std.
using std::ostream;
using std::istream;
using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::istringstream;
using std::endl;
using std::vector;
using std::queue;
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

template <class T>
inline set<T> &operator <<(set<T> &to, const T &elem) {
	to.insert(elem);
	return to;
}

template <class T>
inline queue<T> &operator <<(queue<T> &to, const T &elem) {
	to.push(elem);
	return to;
}

template <class T>
inline vector<T> operator +(vector<T> a, const vector<T> &b) {
	for (nat i = 0; i < b.size(); i++)
		a << b[i];
	return a;
}

template <class T>
inline set<T> operator +(set<T> a, const set<T> &b) {
	a.insert(b.begin(), b.end());
	return a;
}

// Convert to number.
template <class T>
T to(const String &s, const T &def = T()) {
	std::istringstream ss(s);
	T t = def;
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
void join(ostream &to, const T &data, const String &between = ", ") {
	typename T::const_iterator i = data.begin();
	typename T::const_iterator end = data.end();
	if (i == end)
		return;

	to << toS(*i);
	for (++i; i != end; ++i) {
		to << between << toS(*i);
	}
}

template <class T>
String join(const T &data, const String &between = ", ") {
	std::ostringstream to;
	join(to, data, between);
	return to.str();
}

vector<String> split(const String &str, const String &delimiter);
String trim(const String &s);

// Tristate.
enum Tristate {
	tUnset,
	tNo,
	tYes,
};

// # of elements in array.
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))

// Output macros.
#include "output.h"


// Null.
#define null NULL

// Zero memory.
template <class T>
void zeroMem(T &v) {
	memset(&v, 0, sizeof(T));
}

#include "globals.h"

#endif
