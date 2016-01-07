#include "std.h"
#include "timestamp.h"
#include "platform.h"
#include <iomanip>

Timestamp::Timestamp() {
	LARGE_INTEGER li;
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	time = li.QuadPart / 10;
}

#ifdef WINDOWS
Timestamp fromFileTime(FILETIME ft) {
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	return Timestamp(li.QuadPart / 10);
}
#endif

Timestamp::Timestamp(nat64 t) {
	time = t;
}

bool Timestamp::operator >(const Timestamp &other) const {
	return time > other.time;
}

bool Timestamp::operator <(const Timestamp &other) const {
	return time < other.time;
}

bool Timestamp::operator >=(const Timestamp &other) const {
	return ((*this) > other) || ((*this) == other);
}

bool Timestamp::operator <=(const Timestamp &other) const {
	return ((*this) < other) || ((*this) == other);
}


Timespan Timestamp::operator -(const Timestamp &other) const {
	return Timespan(time - other.time);
}

Timestamp Timestamp::operator +(const Timespan &other) const {
	return Timestamp(time + other.time);
}

Timestamp Timestamp::operator -(const Timespan &other) const {
	return Timestamp(time - other.time);
}

Timestamp &Timestamp::operator +=(const Timespan &other) {
	time += other.time;
	return *this;
}

Timestamp &Timestamp::operator -=(const Timespan &other) {
	time -= other.time;
	return *this;
}

ostream &operator <<(ostream &to, const Timestamp &t) {
	LARGE_INTEGER li;
	li.QuadPart = t.time * 10LL;
	FILETIME fTime = { li.LowPart, li.HighPart };
	SYSTEMTIME sTime;
	FileTimeToSystemTime(&fTime, &sTime);

	using std::setw;

	char f = to.fill('0');
	to << setw(4) << sTime.wYear << "-"
	   << setw(2) << sTime.wMonth << "-"
	   << setw(2) << sTime.wDay << " "
	   << setw(2) << sTime.wHour << ":"
	   << setw(2) << sTime.wMinute << ":"
	   << setw(2) << sTime.wSecond << ","
	   << setw(4) << sTime.wMilliseconds;
	to.fill(f);

	return to;
}

