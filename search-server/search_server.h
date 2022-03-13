#pragma once

#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "string_processing.h"

#include <vector>
#include <string>
#include <set>
#include <map>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <cmath>
#include <execution>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double DEVIATION = 1e-6;

using std::string_literals::operator""s;

class SearchServer
{
public:
    SearchServer()
    {}

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words);
    explicit SearchServer(const std::string& stop_words);

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    std::set<int>::const_iterator begin() const; //TODO TESTS
    std::set<int>::const_iterator end() const; //TODO TESTS

    size_t GetDocumentCount() const;


    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    const std::map<std::string, double>& GetWordFrequencies(int document_id) const; //TODO TESTS
    template <class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);
    void RemoveDocument(int document_id);
private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
        std::vector<std::string> doc_words;
    };

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    std::set<std::string> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    template <typename StringCollection>
    void SetStopWords(const StringCollection& stop_words);

    [[nodiscard]] bool IsStopWord(const std::string_view& word) const;
    [[nodiscard]] bool IsValidWord(const std::string_view& word) const;
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    QueryWord ParseQueryWord(const std::string_view& text) const;

    template <class ExecutionPolicy>
    Query ParseQuery(ExecutionPolicy&& policy, const std::string_view& text) const;
    Query ParseQuery(const std::string_view& text) const;
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    void SortAndUnique(std::vector<std::string_view>& vec_to_normalize) const;
};

void AddDocument(SearchServer& search_server, int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);
void FindTopDocuments(const SearchServer& search_server, const std::string_view& raw_query);
void MatchDocuments(const SearchServer& search_server, const std::string_view& query);

template <typename StringCollection>
void SearchServer::SetStopWords(const StringCollection& stop_words)
{
    using std::string_literals::operator""s;

    for (const std::string_view& word : stop_words)
    {
        if (word != ""s)
        {
            if (!IsValidWord(word))
            {
                throw std::invalid_argument("При создании поискового сервера в стоп-словах обнаружены спец-символы(ASCII 0-31). В поисковом сервере запрещено их использование"s);
            }
            if (word == "-" || word[1] == '-')
            {
                throw std::invalid_argument("При создании поискового сервер обнаружены невалидные стоп-слова - \"-\" или \"--*\""s);
            }
            stop_words_.insert(std::string(word));
        }
        else
        {
            throw std::invalid_argument("В векторе передано пустая строка"s);
        }
    }
}

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words)
{
    SetStopWords(stop_words);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < DEVIATION)
            {
                return lhs.rating > rhs.rating;
            }
            else
            {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id)
{
    {
        const auto& element_to_delete = document_ids_.find(document_id);
        if (element_to_delete != document_ids_.end())
        {
            document_ids_.erase(element_to_delete);
        }
    }

    {
        const auto& doc_to_freq = document_to_word_freqs_.find(document_id);
        if (doc_to_freq != document_to_word_freqs_.end())
        {
            const auto& words_freq = (*doc_to_freq).second;
            std::vector<const std::string*> words_to_remove(words_freq.size());

            std::transform(policy,
                words_freq.begin(),
                words_freq.end(),
                words_to_remove.begin(),
                [](const auto& word_to_freq)
                {
                    return &word_to_freq.first;
                });

            std::for_each(policy,
                words_to_remove.begin(),
                words_to_remove.end(),
                [this, document_id](const auto& word_to_remove)
                {
                    word_to_document_freqs_.at(*word_to_remove).erase(document_id);
                });

            document_to_word_freqs_.erase(doc_to_freq);
        }
    }

    {
        const auto& element_to_delete = documents_.find(document_id);
        if (element_to_delete != documents_.end())
        {
            documents_.erase(element_to_delete);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
SearchServer::Query SearchServer::ParseQuery(ExecutionPolicy&& policy, const std::string_view& text) const
{
    Query result;
    auto& min_words = result.minus_words;
    auto& pls_words = result.plus_words;

    auto words = SplitIntoWords(text);

    if constexpr (!std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>)
    {
        SortAndUnique(words);
    }

    for (const std::string_view& word : words)
    {
        const auto& query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                min_words.push_back(query_word.data);
            }
            else
            {
                pls_words.push_back(query_word.data);
            }
        }
    }

    return result;
}


#include "process_queries.h"