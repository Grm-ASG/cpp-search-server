#pragma once

#include "document.h"

#include <vector>
#include <iostream>

void PrintDocument( const Document& document );
void PrintMatchDocumentResult( int document_id, const std::vector<std::string>& words, DocumentStatus status );
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