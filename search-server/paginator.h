#pragma once
#include "search_server.h"
#include "document.h"

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end);

    Iterator begin() const;
    Iterator end() const;
    size_t size() const;

private:
    Iterator _begin;
    Iterator _end;
    size_t _size;
};

template <typename Iterator>
IteratorRange<Iterator>::IteratorRange(Iterator begin, Iterator end)
    : _begin(begin), _end(end)
{
    _size = end - begin;
}

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

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size);

    auto begin() const;
    auto end() const;
    size_t size() const;
private:
    std::vector<IteratorRange<Iterator>> _pages;
};

template <typename Iterator>
Paginator<Iterator>::Paginator(Iterator begin, Iterator end, size_t page_size)
{
    const size_t numOfDocs = end - begin;
    for (size_t i = 0; i < numOfDocs / page_size; i++)
    {
        _pages.push_back({ begin, begin + page_size });
        std::advance(begin, page_size);
    }
    if (begin < end)
    {
        _pages.push_back({ begin, end });
    }
}

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

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& iter) {
    for (Iterator ptr = iter.begin(); ptr < iter.end(); ++ptr)
    {
        out << "{ ";
        out << *ptr;
        out << " }";
    }
    return (out);
}