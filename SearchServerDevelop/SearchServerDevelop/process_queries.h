#pragma once

#include "search_server.h"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <chrono>
#include <execution>
#include <list>

#include "document.h"

using std::string_literals::operator""s;

void RemoveDuplicates( SearchServer& search_server );
void PrintDocument( const Document& document );
void PrintMatchDocumentResult( int document_id, const std::vector<std::string_view>& words, DocumentStatus status );
std::ostream& operator<<( std::ostream& out, const Document& doc );

template <typename Container>
void Print( std::ostream& out, const Container& container )
{
    using std::string_literals::operator""s;
    bool f_print = true;
    for ( const auto& elem : container )
    {
        if ( f_print )
        {
            f_print = false;
            out << elem;
        }
        else
        {
            out << ", "s << elem;
        }
    }
}

template <typename Key, typename Value>
std::ostream& operator<<( std::ostream& out, const std::pair<Key, Value>& container )
{
    out << container.first;
    out << ": ";
    out << container.second;
    return ( out );
}

template <typename Term>
std::ostream& operator<<( std::ostream& out, const std::vector<Term>& container )
{
    out << '[';
    Print( out, container );
    out << ']';
    return ( out );
}

template <typename Term>
std::ostream& operator<<( std::ostream& out, const std::set<Term>& container )
{
    out << '{';
    Print( out, container );
    out << '}';
    return ( out );
}

template <typename Key, typename Value>
std::ostream& operator<<( std::ostream& out, const std::map<Key, Value>& container )
{
    out << '{';
    Print( out, container );
    out << '}';
    return ( out );
}

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries );

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries );