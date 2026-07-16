#pragma once

/**
 * Wildcard matcher.
 */
class Wildcard {
public:
	// Create
	Wildcard(const String &str);

	// Compare.
	inline bool operator ==(const Wildcard &o) const {
		return pattern == o.pattern;
	}
	inline bool operator !=(const Wildcard &o) const {
		return pattern != o.pattern;
	}

	// See if the wildcard pattern matches a string.
	bool matches(const String &str) const;

	// Output
	friend ostream &operator <<(ostream &to, const Wildcard &from);

private:
	String pattern;

	// Recursive helpers.
	bool matches(nat strAt, const String &str, nat patternAt) const;
	bool matchesStar(nat strAt, const String &str, nat patternAt) const;
};
