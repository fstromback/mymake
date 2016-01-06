#pragma once

#include "timespan.h"

class Timespan;

/**
 * Represents a timestamp relative to some date. Eg. January 1, 1601. This
 * is assumed to be stable between different runs of the program. For high
 * performance timing that is not 'stable', look at Clock/Time (TODO).
 * The internal time is measured in us, but it is only accurate
 * to a few milliseconds (on Windows).
 */
class Timestamp {
public:
	Timestamp(); // Creates a time "now".
	Timestamp(nat64 t);

	// measured in us.
	nat64 time;

	inline bool operator ==(const Timestamp &other) const { return time == other.time; }
	inline bool operator !=(const Timestamp &other) const { return time != other.time; }
	bool operator >(const Timestamp &other) const;
	bool operator <(const Timestamp &other) const;
	bool operator >=(const Timestamp &other) const;
	bool operator <=(const Timestamp &other) const;

	Timespan operator -(const Timestamp &other) const;
	Timestamp operator +(const Timespan &other) const;
	Timestamp operator -(const Timespan &other) const;
	Timestamp &operator +=(const Timespan &other);
	Timestamp &operator -=(const Timespan &other);

	friend class Timespan;
	friend ostream &operator <<(ostream &to, const Timestamp &t);
};
