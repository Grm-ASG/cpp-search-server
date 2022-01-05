#pragma once
#include <vector>
#include <iostream>
#include <algorithm>
#include <deque>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "document.h"
#include "string_processing.h"

using std::string_literals::operator""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double DEVIATION = 1e-6;

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

template <typename stringContainer>
std::set<std::string> MakeUniqueNonEmptystrings(const stringContainer& strings) {
	std::set<std::string> non_empty_strings;
	for (const std::string& str : strings) {
		if (!str.empty()) {
			non_empty_strings.insert(str);
		}
	}
	return non_empty_strings;
}

class SearchServer {
public:
	SearchServer()
	{}

	template <typename StringCollection>
	explicit SearchServer(const StringCollection& stopWords);
	explicit SearchServer(const std::string& stopWords);

	void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
	std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	int GetDocumentCount() const;
	int GetDocumentId(int index) const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	struct QueryWord {
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	struct Query {
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::vector<int> document_ids_;

	template <typename StringCollection>
	void SetStopWords(const StringCollection& stopWords);

	[[nodiscard]] bool isSpecSimbOrIncorDash(const std::string& document) const;
	[[nodiscard]] bool IsStopWord(const std::string& word) const;
	[[nodiscard]] static bool IsValidWord(const std::string& word);
	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
	static int ComputeAverageRating(const std::vector<int>& ratings);
	QueryWord ParseQueryWord(const std::string& text) const;
	Query ParseQuery(const std::string& text) const;
	double ComputeWordInverseDocumentFreq(const std::string& word) const;
	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query);
void MatchDocuments(const SearchServer& search_server, const std::string& query);

template <typename StringCollection>
void SearchServer::SetStopWords(const StringCollection& stopWords) {
	for (const std::string& word : stopWords) {
		if (word != ""s)
		{
			if (!IsValidWord(word))
				throw std::invalid_argument("ѕри создании поискового сервера в стоп-словах обнаружены спец-символы(ASCII 0-31). ¬ поисковом сервере запрещено их использование"s);
			if (word == "-" || word[1] == '-')
				throw std::invalid_argument("ѕри создании поискового сервер обнаружены невалидные стоп-слова - \"-\" или \"--*\""s);
			stop_words_.insert(word);
		}
	}
}

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stopWords) {
	SetStopWords(stopWords);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
	if (isSpecSimbOrIncorDash(raw_query))
		throw std::invalid_argument("ѕри поиске документов в строке запроса обнаружены спец - символы(ASCII 0 - 31) или некорректные стоп(-) слова.¬ поисковом сервере запрещено их использование\n"s);

	const auto query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(query, document_predicate);

	sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
		if (std::abs(lhs.relevance - rhs.relevance) < DEVIATION) {
			return lhs.rating > rhs.rating;
		}
		else {
			return lhs.relevance > rhs.relevance;
		}
		});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
	std::map<int, double> document_to_relevance;
	for (const std::string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
			const auto& document_data = documents_.at(document_id);
			if (document_predicate(document_id, document_data.status, document_data.rating)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (const std::string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.erase(document_id);
		}
	}

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance) {
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
};
