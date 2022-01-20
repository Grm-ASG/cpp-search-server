#pragma once

#include "document.h"
#include "search_server.h"

#include <string>
#include <vector>
#include <deque>
#include <numeric>

class RequestQueue
{
public:
    explicit RequestQueue( const SearchServer& search_server );

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest( const std::string& raw_query, DocumentPredicate document_predicate );
    std::vector<Document> AddFindRequest( const std::string& raw_query, DocumentStatus statusDoc );
    std::vector<Document> AddFindRequest( const std::string& raw_query );
    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        std::string query;
        int numOfResults;
    };
    const SearchServer& server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest( const std::string& raw_query, DocumentPredicate document_predicate )
{
    std::vector<Document> searchResult = server_.FindTopDocuments( raw_query, document_predicate );
    if ( requests_.size() >= min_in_day_ )
    {
        requests_.pop_front();
    }
    requests_.push_back( { raw_query, static_cast< int >( searchResult.size() ) } );
    return searchResult;
}