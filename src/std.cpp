#include "std.h"

NoCopy::NoCopy() {}

NoCopy::~NoCopy() {}


vector<String> split(const String &str, const String &delimiter) {
	vector<String> r;

	nat start = 0;
	nat end = str.find(delimiter);
	while (end != String::npos) {
		r.push_back(str.substr(start, end - start));

		start = end + delimiter.size();
		end = str.find(delimiter, start);
	}

	if (start < str.size())
		r.push_back(str.substr(start));

	return r;
}


String trim(const String &s) {
	if (s.size() == 0)
		return "";

	nat start = 0, end = s.size() - 1;
	for (start = 0; start < s.size(); start++) {
		if (!isspace(s[start]))
			break;
	}

	while (end > start) {
		if (!isspace(s[end]))
			break;
		if (end > 1)
			--end;
	}

	if (start > end)
		return "";
	return s.substr(start, end - start + 1);
}


Lock outputLock;
