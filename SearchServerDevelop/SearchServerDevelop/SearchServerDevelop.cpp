#include "search_server.h"
#include "request_queue.h"
#include "test_example_functions.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;
/*
string GenerateWord( mt19937& generator, int max_length )
{
    const int length = uniform_int_distribution( 1, max_length )( generator );
    string word;
    word.reserve( length );
    for ( int i = 0; i < length; ++i )
    {
        word.push_back( uniform_int_distribution( 'a', 'z' )( generator ) );
    }
    return word;
}

vector<string> GenerateDictionary( mt19937& generator, int word_count, int max_length )
{
    vector<string> words;
    words.reserve( word_count );
    for ( int i = 0; i < word_count; ++i )
    {
        words.push_back( GenerateWord( generator, max_length ) );
    }
    sort( words.begin(), words.end() );
    words.erase( unique( words.begin(), words.end() ), words.end() );
    return words;
}

string GenerateQuery( mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0 )
{
    string query;
    for ( int i = 0; i < word_count; ++i )
    {
        if ( !query.empty() )
        {
            query.push_back( ' ' );
        }
        if ( uniform_real_distribution<>( 0, 1 )( generator ) < minus_prob )
        {
            query.push_back( '-' );
        }
        query += dictionary[uniform_int_distribution<int>( 0, dictionary.size() - 1 )( generator )];
    }
    return query;
}

vector<string> GenerateQueries( mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count )
{
    vector<string> queries;
    queries.reserve( query_count );
    for ( int i = 0; i < query_count; ++i )
    {
        queries.push_back( GenerateQuery( generator, dictionary, max_word_count ) );
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test( string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy )
{
    LOG_DURATION( mark );
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for ( int id = 0; id < document_count; ++id )
    {
        const auto [words, status] = search_server.MatchDocument( policy, query, id );
        word_count += words.size();
    }
    cout << word_count << endl;
}

#define TEST(policy) Test(#policy, search_server, query, execution::policy)

int main()
{
    mt19937 generator;

    const auto dictionary = GenerateDictionary( generator, 1000, 10 );
    const auto documents = GenerateQueries( generator, dictionary, 10'000, 70 );

    const string query = GenerateQuery( generator, dictionary, 500, 0.1 );

    SearchServer search_server( dictionary[0] );
    for ( size_t i = 0; i < documents.size(); ++i )
    {
        search_server.AddDocument( i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 } );
    }

    TEST( seq );
    TEST( par );
}*/


using std::string_literals::operator""s;

int main()
{
    setlocale( LC_ALL, "Russian" );
    try
    {
        TestSearchServer();
    }
    catch ( const std::exception& e )
    {
        std::cout << e.what() << std::endl;
    }
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cout << "Search server testing finished"s << std::endl;
}

// int main() {
//    SearchServer search_server("and in at"s);
//    RequestQueue request_queue(search_server);

//    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
//    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
//    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
//    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
//    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

//    // 1439 запросов с нулевым результатом
//    for (int i = 0; i < 1439; ++i)
//    {
//        request_queue.AddFindRequest("empty request"s);
//    }
//    // все еще 1439 запросов с нулевым результатом
//    request_queue.AddFindRequest("curly dog"s);
//    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
//    request_queue.AddFindRequest("big collar"s);
//    // первый запрос удален, 1437 запросов с нулевым результатом
//    request_queue.AddFindRequest("sparrow"s);
//    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
//    return 0;
// }


/*
#include "process_queries.h"
#include "search_server.h"

#include "log_duration.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

string GenerateWord(mt19937 & generator, int max_length) {
    const int length = uniform_int_distribution<int>(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution<int>('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937 & generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937 & generator, const vector<string>&dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937 & generator, const vector<string>&dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer & search_server, const vector<string>&queries, ExecutionPolicy && policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << "total relev : " << total_relevance << endl;
}

#define TEST(policy) Test(#policy, search_server, queries, execution::policy)

int main() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueries(generator, dictionary, 100, 70);

    TEST(seq);
    TEST(par);
}*/