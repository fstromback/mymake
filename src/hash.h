#pragma once
#include "platform.h"
#include "path.h"

/**
 * Utilities for using std::unordered_map conveniently.
 */

#ifdef _MSC_VER
#if _MSC_VER <= 1500
#define USE_STDEXT
#endif
#endif

#ifdef USE_STDEXT

#include <hash_map>
#include <hash_set>

namespace stdext {

	inline size_t hash_value(const String &s) {
		return hash_value(s.c_str());
	}

	inline size_t hash_value(const Path &p) {
		return p.hash();
	}

}

using stdext::hash_map;
using stdext::hash_set;

#else

#include <unordered_map>
#include <unordered_set>

namespace std {

	template <>
	struct hash<Path> {
		size_t operator ()(const Path &p) const {
			return p.hash();
		}
	}

}

template <class K, class V>
using hash_map = std::unordered_map<K, V>;

template <class K>
using hash_set = std::unordered_set<K>;

#endif
