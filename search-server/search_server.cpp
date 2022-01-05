#include "print_functions.h"
#include "search_server.h"

SearchServer::SearchServer(const std::string& stopWords) {
	SetStopWords(SplitIntoWords(stopWords));
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
	std::string error = "";
	if (document_id < 0)
		error += "�������� document_id - �� ����� ���� ������������� ������\n";
	if (documents_.count(document_id) > 0)
		error += "�������� document_id - �������� � ����� id ��� ������������ � ��������� �������\n";
	if (isSpecSimbOrIncorDash(document))
		error += "��� ���������� ��������� ���������� ����-�������(ASCII 0-31) ��� ������������ ����(-) �����. � ��������� ������� ��������� �� �������������\n";
	if (error != ""s)
		throw std::invalid_argument(error);

	const auto words = SplitIntoWordsNoStop(document);

	const double inv_word_count = 1.0 / words.size();
	for (const std::string& word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	document_ids_.push_back(document_id);
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
	return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
	return document_ids_.at(index);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
	const auto query = ParseQuery(raw_query);
	std::string error = "";
	if (isSpecSimbOrIncorDash(raw_query))
		error += "��� ��������� ���������� ���������� ����-�������(ASCII 0-31) ��� ������������ ����(-) �����. � ��������� ������� ��������� �� �������������\n";
	if (documents_.count(document_id) == 0)
		error += "��� ��������� ���������� �� ������ �������������� �������� � ��������� �������";
	if (error != ""s)
		throw std::invalid_argument(error);

	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
	for (const std::string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const {
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Word "s + word + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
	if (text.empty()) {
		throw std::invalid_argument("Query word is empty"s);
	}
	std::string word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word "s + text + " is invalid");
	}

	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
	Query result;
	for (const std::string& word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.insert(query_word.data);
			}
			else {
				result.plus_words.insert(query_word.data);
			}
		}
	}
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
	const std::vector<int>& ratings) {
	try {
		search_server.AddDocument(document_id, document, status, ratings);
	}
	catch (const std::invalid_argument& e) {
		std::cout << "������ ���������� ��������� "s << document_id << ": "s << e.what() << std::endl;
	}
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
	std::cout << "���������� ������ �� �������: "s << raw_query << std::endl;
	try {
		for (const Document& document : search_server.FindTopDocuments(raw_query)) {
			PrintDocument(document);
		}
	}
	catch (const std::invalid_argument& e) {
		std::cout << "������ ������: "s << e.what() << std::endl;
	}
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
	try {
		std::cout << "������� ���������� �� �������: "s << query << std::endl;
		const int document_count = search_server.GetDocumentCount();
		for (int index = 0; index < document_count; ++index) {
			const int document_id = search_server.GetDocumentId(index);
			const auto [words, status] = search_server.MatchDocument(query, document_id);
			PrintMatchDocumentResult(document_id, words, status);
		}
	}
	catch (const std::invalid_argument& e) {
		std::cout << "������ �������� ���������� �� ������ "s << query << ": "s << e.what() << std::endl;
	}
}

[[nodiscard]] bool SearchServer::isSpecSimbOrIncorDash(const std::string& document) const
{
	bool isWord = false;
	for (size_t i = 0; i < document.length(); i++)
	{
		int ch = document[i];
		bool isDash = document[i] == '-';
		if (std::isspace(document[i]))
			isWord = false;
		else if (!isDash)
			isWord = true;
		char nextSimb = document[i + 1];
		if ((ch <= 31 && ch >= 0) || (isDash && !isWord && (nextSimb == '\0' || nextSimb == '-' || std::isspace(nextSimb))))
			return true;
	}
	return false;
}