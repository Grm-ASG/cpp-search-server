#include "paginator.h"

// Definition methods of Paginator class
template <typename Iterator>
auto Paginator<Iterator>::begin() const
{
	return _pages.begin();
}

template <typename Iterator>
auto Paginator<Iterator>::end() const
{
	return _pages.end();
}

template <typename Iterator>
size_t Paginator<Iterator>::size() const
{
	return _pages.size();
}

// Definition methods of IteratorRange class
template <typename Iterator>
Iterator IteratorRange<Iterator>::begin() const
{
	return _begin;
}

template <typename Iterator>
Iterator IteratorRange<Iterator>::end() const
{
	return _end;
}

template <typename Iterator>
size_t IteratorRange<Iterator>::size() const
{
	return _size;
}