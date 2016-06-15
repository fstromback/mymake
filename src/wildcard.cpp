#include "std.h"
#include "wildcard.h"

Wildcard::Wildcard(const String &pattern) : pattern(pattern) {}

bool Wildcard::matches(const String &str) const {
	return matches(0, str, 0);
}

bool Wildcard::matches(nat strAt, const String &str, nat patternAt) const {
	for (; patternAt < pattern.length(); patternAt++) {
		if (strAt == str.length())
			return false; //premature ending

		switch (pattern[patternAt]) {
		case '*':
			return matchesStar(strAt, str, patternAt + 1);
			break;
		case '?':
			strAt++;
			break;
		case '/':
			// / matches both / and \ for simplicity
			if (str[strAt] != '/' && str[strAt] != '\\')
				return false;
			strAt++;
			break;
		default:
			if (pattern[patternAt] != str[strAt])
				return false;
			strAt++;
			break;
		}
	}

	return strAt == str.length();
}

bool Wildcard::matchesStar(nat strAt, const String &str, nat patternAt) const {
	for (; strAt <= str.length(); strAt++) {
		if (matches(strAt, str, patternAt))
			return true;
	}
	return false;
}

ostream &operator <<(ostream &to, const Wildcard &w) {
	return to << w.pattern;
}
