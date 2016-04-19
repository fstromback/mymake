#include "std.h"
#include "timespan.h"

#include <cmath>
#include <sstream>
#include <iomanip>

const Timespan Timespan::zero(Timespan::ms(0));

Timespan::Timespan() {
	time = 0;
}

Timespan::Timespan(int64 micros) {
	this->time = micros;
}

Timespan::Timespan(const void *from) {
	time = *((int64 *)from);
}


void Timespan::save(void *to) const {
	*((int64 *)to) = time;
}

static int64 my_abs(int64 v) {
	if (v < 0)
		return -v;
	else
		return v;
}

ostream &operator <<(ostream &output, const Timespan &t) {
	int64 time = my_abs(t.time);

	if (time < 1000) {
		output << t.time << L" us";
	} else if (time < 1000 * 1000) {
		output << t.time / 1000 << L" ms";
	} else if (time < 60 * 1000 * 1000) {
		output << std::fixed << std::setprecision(2) << (t.time / 1000000.0) << L" s";
	} else {
		output << std::fixed << std::setprecision(2) << (t.time / 60000000.0) << L" min";
	}

	return output;
}

String Timespan::format() const {
	std::ostringstream output;

	int64 seconds = my_abs(this->time) / (1000 * 1000);
	int64 minutes = seconds / 60;
	seconds = seconds % 60;
	output << minutes << ":" <<std::setw(2) << std::setfill('0') << seconds;
	return output.str();
}

Timespan abs(const Timespan &time) {
	return Timespan(my_abs(time.micros()));
}
