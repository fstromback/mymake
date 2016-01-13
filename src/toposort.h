#pragma once

/**
 * Topological sort, for finding correct compilation order of projects.
 */


/**
 * Directed node in toposort.
 */
template <class T>
struct Node {
	T name;
	set<T> dependsOn;
};

template <class T>
ostream &operator <<(ostream &to, const Node<T> &v) {
	return to << v.name << " -> " << join(v.dependsOn);
}

// Exception.
class TopoError : public Error {
public:
	inline TopoError(const String &msg) : msg(msg) {}
	inline ~TopoError() throw() {}
	inline const char *what() const throw() { return msg.c_str(); }
private:
	String msg;
};


// Find a topological order that fulfills all dependencies. Throws error on failure.
template <class T, class InputIt>
vector<Node<T>> topoSort(const InputIt &begin, const InputIt &end) {
	typedef set<T> Edges;

	struct Rev {
		nat incoming;
		Edges to;

		Rev() : incoming(0) {}
	};

	typedef map<T, Rev> RevMap;
	RevMap reverse;

	// Add all edges to our structures.
	for (InputIt at = begin; at != end; ++at) {
		// Remember # of dependencies.
		reverse[at->name].incoming = at->dependsOn.size();

		// Add reverse dependencies.
		for (typename Edges::const_iterator i = at->dependsOn.begin(), e = at->dependsOn.end(); i != e; ++i) {
			reverse[*i].to << at->name;
		}
	}

	// All items that currently have all dependencies fullfilled.
	queue<T> done;

	// Find all edges with all dependencies fulfilled.
	for (typename RevMap::const_iterator i = reverse.begin(), e = reverse.end(); i != e; ++i) {
		if (i->second.incoming == 0)
			done << i->first;
	}

	// Figure the order out!
	vector<T> order;
	while (!done.empty()) {
		T now = done.front(); done.pop();
		order << now;

		Edges &edge = reverse[now].to;
		for (typename Edges::const_iterator i = edge.begin(), e = edge.end(); i != e; ++i) {
			if (--reverse[*i].incoming == 0)
				done << *i;
		}
	}

	// Check if we found all.
	if (order.size() < reverse.size()) {
		// No. Find the nodes with minimum # of incoming edges.
		nat lowest = reverse.size();
		for (typename RevMap::const_iterator i = reverse.begin(), e = reverse.end(); i != e; ++i) {
			if (i->second.incoming > 0)
				lowest = min(lowest, i->second.incoming);
		}

		vector<String> cycleNodes;
		for (typename RevMap::const_iterator i = reverse.begin(), e = reverse.end(); i != e; ++i) {
			if (i->second.incoming == lowest)
				cycleNodes << i->first;
		}

		throw TopoError("Cycle detected between " + join(cycleNodes));
	}

	vector<Node<T>> result;
	map<T, Node<T>> lookup;
	for (InputIt at = begin; at != end; ++at) {
		lookup.insert(make_pair(at->name, *at));
	}

	result.reserve(order.size());
	for (nat i = 0; i < order.size(); i++)
		result << lookup[order[i]];

	return result;
}

template <class T>
vector<Node<T>> topoSort(const vector<Node<T>> &t) {
	return topoSort<T>(t.begin(), t.end());
}

template <class T>
vector<Node<T>> topoSort(const set<Node<T>> &t) {
	return topoSort<T>(t.begin(), t.end());
}
