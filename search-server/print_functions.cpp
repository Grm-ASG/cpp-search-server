#include "print_functions.h"

using std::string_literals::operator""s;

void PrintDocument( const Document& document )
{
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult( int document_id, const std::vector<std::string>& words, DocumentStatus status )
{
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast< int >( status ) << ", "s
        << "words ="s;
    for ( const std::string& word : words )
    {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

std::ostream& operator<<( std::ostream& out, const Document& doc )
{
    out << "document_id = " << doc.id
        << ", relevance = " << doc.relevance
        << ", rating = " << doc.rating;

    return ( out );
}