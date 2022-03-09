#include "process_queries.h"

using std::string_literals::operator""s;

void RemoveDuplicates( SearchServer& search_server )
{
    std::map<std::set<std::string>, std::vector<int>> doc_to_del;
    std::set<int> remove_docs;
    for ( const int document_id : search_server )
    {
        std::set<std::string> words;
        std::map<std::string, double> word_with_freq = search_server.GetWordFrequencies( document_id );
        for ( auto [word, freq] : word_with_freq )
        {
            words.insert( word );
        }
        doc_to_del[words].push_back( document_id );
    }

    for ( auto doc : doc_to_del )
    {
        if ( doc.second.size() > 1 )
        {
            remove_docs.insert( std::next( doc.second.begin() ), doc.second.end() );
        }
    }

    for ( const int to_remove : remove_docs )
    {
        search_server.RemoveDocument( to_remove );
        std::cout << "Found duplicate document id "s << to_remove << std::endl;
    }
}

void PrintDocument( const Document& document )
{
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult( int document_id, const std::vector<std::string_view>& words, DocumentStatus status )
{
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast< int >( status ) << ", "s
        << "words ="s;
    for ( const std::string_view& word : words )
    {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

std::ostream& operator<<( std::ostream& out, const Document& document )
{
    out << "document_id = " << document.id
        << ", relevance = " << document.relevance
        << ", rating = " << document.rating;

    return ( out );
}

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries )
{
    std::vector<std::vector<Document>> result( queries.size() );

    std::transform( std::execution::par,
                    queries.begin(),
                    queries.end(),
                    result.begin(),
                    [&search_server] ( const std::string& query )
                    {
                        return search_server.FindTopDocuments( query );
                    } );
    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries )
{
    auto arr_of_docs = ProcessQueries( search_server, queries );
    std::list<Document> list_of_docs;

    for ( std::vector<Document>& doc_vec : arr_of_docs )
    {
        list_of_docs.splice( list_of_docs.end(), { doc_vec.begin(), doc_vec.end() } );
    }

    return list_of_docs;
}