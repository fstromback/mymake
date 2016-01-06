#pragma once

class Timestamp;

class Timespan {
public:
	Timespan();
	Timespan(const void *from);

	inline bool operator ==(const Timespan &other) const { return time == other.time; }
	inline bool operator !=(const Timespan &other) const { return time != other.time; }
	inline bool operator >(const Timespan &other) const { return time > other.time; }
	inline bool operator <(const Timespan &other) const { return time < other.time; }
	inline bool operator >=(const Timespan &other) const { return time >= other.time; }
	inline bool operator <=(const Timespan &other) const { return time <= other.time; }

	inline Timespan operator +(const Timespan &other) const { return Timespan(time + other.time); }
	inline Timespan operator -(const Timespan &other) const { return Timespan(time - other.time); }
	inline Timespan operator -() const { return Timespan(-time); }
	inline Timespan operator *(int factor) const { return Timespan(time * factor); }
	inline Timespan operator *(float factor) const { return Timespan(int64(time * factor)); }
	inline Timespan operator /(int number) const { return Timespan(time / number); }

	inline int operator /(const Timespan &other) const { return int(time / other.time); }
	inline Timespan operator %(const Timespan &other) const { return Timespan(time % other.time); }

	// return this time expressed in terms of "other".
	inline float compare(const Timespan &other) const { return float(time) / float(other.time); }

	inline Timespan &operator -=(const Timespan &other) { time -= other.time; return *this; }
	inline Timespan &operator +=(const Timespan &other) { time += other.time; return *this; }

	inline int millis() const { return int(time / 1000); };
	inline int64 micros() const { return time; };
	void save(void *to) const;

	String format() const; //Format as (M)M:SS

	static const int size = sizeof(int64);

	static const Timespan zero;
	static inline Timespan ms(int ms) { return Timespan(ms * 1000); };
	static inline Timespan us(int64 us) { return Timespan(us); };

	friend Timespan abs(const Timespan &time);
	friend class Timestamp;
	friend ostream &operator <<(ostream &to, const Timespan &t);
private:
	Timespan(int64 micros);

	int64 time; //In microseconds
};

