#include "std.h"
#include "output.h"

Lock outputLock;

THREAD OutputState *outputState;

OutputState::OutputState() : banner(), prefix(), refs(1), refLock() {}

OutputState::OutputState(const String &banner, const String &prefix)
	: banner(banner), prefix(prefix), refs(1), refLock() {}


OutputState *OutputState::ref() {
	Lock::Guard z(refLock);
	refs++;
	return this;
}

void OutputState::unref() {
	bool destroy = false;
	{
		Lock::Guard z(refLock);
		if (--refs == 0)
			destroy = true;
	}

	// Note: Has to be done *after* releasing the lock!
	if (destroy)
		delete this;
}

void OutputState::output(ostream &to) {
	if (!banner.empty()) {
		if (!prefix.empty())
			to << prefix;

		to << banner << endl;
		banner = "";
	}

	if (!prefix.empty()) {
		to << prefix;
	}
}
