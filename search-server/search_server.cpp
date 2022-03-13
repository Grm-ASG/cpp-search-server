#include "search_server.h"

using std::string_literals::operator""s;

SearchServer::SearchServer(const std::string& stopWords)
{
    SetStopWords(SplitIntoWords(stopWords));
}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings)
{
    std::string error = ""s;
    if (document_id < 0)
    {
        error += "Неверный document_id - не может быть отрицательным числом\n"s;
    }
    if (documents_.count(document_id) > 0)
    {
        error += "Неверный document_id - документ с таким id уже присутствует в поисковой системе\n"s;
    }
    if (error != ""s)
    {
        throw std::invalid_argument(error);
    }

    const auto words = SplitIntoWordsNoStop(document);
    std::vector<std::string> words_std(words.begin(), words.end());
    const double inv_word_count = 1.0 / words.size();

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, words_std });

    for (const std::string& word : documents_.at(document_id).doc_words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    document_ids_.insert(document_id);
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const
{
    //return MatchDocument(std::execution::seq, raw_query, document_id);
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    if (documents_.count(document_id) == 0)
    {
        throw std::out_of_range("Document out of range");
    }

    bool isMinus = false;

    for (const std::string_view& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) > 0 &&
            word_to_document_freqs_.at(word).count(document_id) > 0)
        {
            matched_words.clear();
            isMinus = true;
            break;
        }
    }
    if (!isMinus)
    {
        for (const std::string_view& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) > 0 &&
                word_to_document_freqs_.at(word).count(document_id) > 0)
            {
                matched_words.push_back(word);
            }
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

[[nodiscard]] bool SearchServer::IsStopWord(const std::string_view& word) const
{
    return stop_words_.count(static_cast<std::string>(word)) > 0;
}

[[nodiscard]] bool SearchServer::IsValidWord(const std::string_view& word) const
{
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const
{
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view& text) const
{
    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid"s);
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const
{
    return ParseQuery(std::execution::seq, std::move(text));
}

void SearchServer::SortAndUnique(std::vector<std::string_view>& vec_to_normalize) const
{
    if (vec_to_normalize.size() > 1)
    {
        const auto& begin_iter = vec_to_normalize.begin();
        const auto& end_iter = vec_to_normalize.end();
        std::sort(begin_iter, end_iter);
        auto it_pl = std::unique(begin_iter, end_iter);
        vec_to_normalize.resize(it_pl - begin_iter);
    }
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings)
{
    try
    {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string_view& raw_query)
{
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try
    {
        for (const Document& document : search_server.FindTopDocuments(raw_query))
        {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string_view& query) //TODO Check if we didn't find the document with index, maybe throw exeption
{
    try
    {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        const size_t document_count = search_server.GetDocumentCount();
        for (size_t index = 0; index < document_count; ++index)
        {
            const auto end_iter = search_server.end();
            const auto iter = std::find(search_server.begin(), end_iter, index);
            const int document_id = iter != end_iter ? *iter : -1;
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.cbegin();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.cend();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    const auto iter_to_doc = document_to_word_freqs_.find(document_id);

    const static std::map<std::string, double> dummy;
    if (iter_to_doc == document_to_word_freqs_.end())
        return dummy;
    return iter_to_doc->second;
}

void SearchServer::RemoveDocument(int document_id)
{
    RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy, std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const
{
    if (documents_.count(document_id) == 0)
    {
        throw std::out_of_range("Wrong document id");
    }

    auto query = ParseQuery(policy, raw_query);

    const auto pred = [this, document_id](std::string_view word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };

    const auto& status = documents_.at(document_id).status;

    if (std::any_of(policy,
        query.minus_words.begin(),
        query.minus_words.end(),
        pred))
    {
        return { std::vector<std::string_view>{}, status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto it = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        pred
    );

    matched_words.erase(it, matched_words.end());
    SortAndUnique(matched_words);

    return { matched_words, status };
}