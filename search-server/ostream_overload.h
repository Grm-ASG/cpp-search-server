#pragma once
#include "search_server.h"
#include "paginator.h"

std::ostream& operator<<(std::ostream& out, const Document& doc);

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::pair<Key, Value>& container) {
	out << container.first;
	out << ": ";
	out << container.second;
	return (out);
}

template <typename Term>
std::ostream& operator<<(std::ostream& out, const std::vector<Term>& container) {
	out << '[';
	Print(out, container);
	out << ']';
	return (out);
}

template <typename Term>
std::ostream& operator<<(std::ostream& out, const std::set<Term>& container) {
	out << '{';
	Print(out, container);
	out << '}';
	return (out);
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value>& container) {
	out << '{';
	Print(out, container);
	out << '}';
	return (out);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>&iter) {
	for (Iterator ptr = iter.begin(); ptr < iter.end(); ++ptr)
	{
		out << "{ ";
		out << *ptr;
		out << " }";
	}
	return (out);
}