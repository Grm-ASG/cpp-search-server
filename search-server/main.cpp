#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

#ifndef ASSERT_INCLUDE

# define ASSERT_INCLUDE

# define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
# define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
# define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
# define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
# define RUN_TEST(func)  RunTestImpl(func, #func)

#endif // !ASSERT_INCLUDE

const int MAX_RESULT_DOCUMENT_COUNT = 5;

using namespace std;

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const pair<Key, Value>& container) {
    out << container.first;
    out << ": ";
    out << container.second;
    return (out);
}

template <typename Container>
void Print(ostream& out, const Container& container)
{
    bool f_print = true;
    for (const auto& elem : container)
    {
        if (f_print)
        {
            f_print = false;
            out << elem;
        }
        else
            out << ", "s << elem;
    }
}

ostream& operator<<(ostream& out, const DocumentStatus& status) {
    switch (status)
    {
    case DocumentStatus::ACTUAL:
        out << "ACTUAL"s;
        break;
    case DocumentStatus::BANNED:
        out << "BANNED"s;
        break;
    case DocumentStatus::IRRELEVANT:
        out << "IRRELEVANT"s;
        break;
    case DocumentStatus::REMOVED:
        out << "DocumentStatus::REMOVED"s;
        break;
    }
    return (out);
}

template <typename Term>
ostream& operator<<(ostream& out, const vector<Term>& container) {
    out << '[';
    Print(out, container);
    out << ']';
    return (out);
}

template <typename Term>
ostream& operator<<(ostream& out, const set<Term>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return (out);
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return (out);
}

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector < string > words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL, IRRELEVANT, BANNED, REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document,
        DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{ ComputeAverageRating(ratings), status });
    }

    template<typename Function>
    vector<Document> FindTopDocuments(const string& raw_query,
        Function ft_selection) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, ft_selection);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

    vector<Document> FindTopDocuments(const string& raw_query,
        DocumentStatus statusDoc) const {
        return (FindTopDocuments(raw_query,
            [statusDoc](int document_id, DocumentStatus status,
                int rating) {
                    return status == statusDoc;
            }));
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return (FindTopDocuments(raw_query,
            [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::ACTUAL;
            }));
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector < string > matched_words;

        if (documents_.count(document_id) == 0)
            return { matched_words, DocumentStatus::ACTUAL };

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector < string > words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(
            GetDocumentCount() * 1.0
            / word_to_document_freqs_.at(word).size());
    }

    template<typename Function>
    vector<Document> FindAllDocuments(const Query& query,
        Function ft_selection) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0)
                continue;

            const double inverse_document_freq = ComputeWordInverseDocumentFreq(
                word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(
                word))
                document_to_relevance[document_id] += term_freq
                * inverse_document_freq;
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0)
                continue;

            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
                document_to_relevance.erase(document_id);
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            if (ft_selection(document_id, documents_.at(document_id).status,
                documents_.at(document_id).rating))
                matched_documents.push_back({ document_id, relevance,
                        documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};


// -------- Имплементация UnitTestFramework ----------
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename Function>
void RunTestImpl(Function func, const string& funcName) {
    func();
    cerr << funcName << " OK"s << endl;
}
// -------- конец блока кода UnitTestFramework ----------

// -------- Начало модульных тестов поисковой системы ----------
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет что поисковая система исключает из выдачи документы со стоп словами
void TestMinusWordsExcludeDocumentFromQuery(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::ACTUAL, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2

    vector<struct Document> document = server.FindTopDocuments("-Hello world"s);
    int i = 0;
    ASSERT_EQUAL(document[i].id, 1);
    ASSERT_EQUAL(document[i].rating, -15);
    ASSERT_EQUAL(document[i].relevance, 0.23104906018664842);
    ASSERT_EQUAL_HINT(document.size(), 1u, "Только 1 документ должен быть найден"s);
    document.clear();

    document = server.FindTopDocuments("Hello -world"s);
    ASSERT_HINT(document.empty(), "Не должно быть найденных документов"s);
    document.clear();

    document = server.FindTopDocuments("Hello - world"s);
    ASSERT(document[i].id == 0 && document[i].rating == 6 && document[i].relevance == 1.0397207708399179);
    ASSERT(document[++i].id == 1 && document[i].rating == -15 && document[i].relevance == 0.23104906018664842);
    ASSERT_EQUAL_HINT(document.size(), 2u, "2 документа должены быть найдены, пустое минус-слово должно игнорироваться в любом месте"s);
    document.clear();

    document = server.FindTopDocuments("Hello world -"s);
    ASSERT(document[--i].id == 0 && document[i].rating == 6 && document[i].relevance == 1.0397207708399179);
    ASSERT(document[++i].id == 1 && document[i].rating == -15 && document[i].relevance == 0.23104906018664842);
    ASSERT_EQUAL_HINT(document.size(), 2u, "2 документа должены быть найдены, пустое минус-слово должно игнорироваться в любом месте"s);
    document.clear();
}

// Тест проверки метода Matching
void TestMatchingDocuments(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::BANNED, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 }); // Avarage rating = 2
    server.AddDocument(4, "Removed doc in server"s, DocumentStatus::REMOVED, { 3, 10, 20 }); // Avarage rating = 11

    tuple<vector<string>, DocumentStatus> testTuple;
    tuple<vector<string>, DocumentStatus> testMatching = server.MatchDocument("", 0);

    ASSERT(testMatching == testTuple);
    vector<string> blankVectorOfWords;

    testMatching = server.MatchDocument("Test query", 0);
    ASSERT(testMatching == testTuple);

    {// Тест пустого возврата при нахождении минус слова и верного статуса
        testMatching = server.MatchDocument("second the -world", 1);
        testTuple = { blankVectorOfWords, DocumentStatus::ACTUAL };
        ASSERT(testMatching == testTuple);

        testMatching = server.MatchDocument("third -in doc", 2);
        testTuple = { blankVectorOfWords, DocumentStatus::BANNED };
        ASSERT(testMatching == testTuple);

        testMatching = server.MatchDocument("-Another document", 3);
        testTuple = { blankVectorOfWords, DocumentStatus::IRRELEVANT };
        ASSERT(testMatching == testTuple);

        testMatching = server.MatchDocument("server -Removed", 4);
        testTuple = { blankVectorOfWords, DocumentStatus::REMOVED };
        ASSERT(testMatching == testTuple);
    }

    {// Тест возврата слов и статусов
        testMatching = server.MatchDocument("second The", 1);
        testTuple = { {"The"s, "second"s}, DocumentStatus::ACTUAL };
        ASSERT(testMatching == testTuple);

        testMatching = server.MatchDocument("Third in doc", 2);
        testTuple = { {"Third"s, "doc"s, "in"s}, DocumentStatus::BANNED };
        ASSERT(testMatching == testTuple);

        testMatching = server.MatchDocument("Another document", 3);
        testTuple = { {"Another"s, "document"s}, DocumentStatus::IRRELEVANT };
        ASSERT(testMatching == testTuple);

        testMatching = server.MatchDocument("server Removed", 4);
        testTuple = { {"Removed"s, "server"s}, DocumentStatus::REMOVED };
        ASSERT(testMatching == testTuple);
    }
}

// Тест выдачи релевантности запроса
void TestSortingDocumentsWithRelevance(void)
{
    SearchServer server;
    vector<string> documents = { "first document at server excel"s, "second document in system word"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
        server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 5, 7, 9 });

    vector<Document> result = server.FindTopDocuments("excel word document in windows at execute some programms");
    int doc_size = result.size() - 1;
    for (int i = 0; i < doc_size - 1; ++i) {
        ASSERT_HINT(result[i].relevance >= result[i + 1].relevance, "Неверная сортировка реливантности документа, i документ не может иметь релевантность меньше нижестоящего");
    }
}

// Тест проверки добавления документа
void TestAddDocument(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::ACTUAL, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::ACTUAL, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::ACTUAL, { 1, 2, 3 }); // Avarage rating = 2

    {
        vector<Document> result = server.FindTopDocuments("Hello");
        vector<Document> testingRes;
        testingRes.push_back({ 0, 0.69314718055994529, 6 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        vector<Document> result = server.FindTopDocuments("second");
        vector<Document> testingRes;
        testingRes.push_back({ 1, 0.46209812037329684, -15 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        vector<Document> result = server.FindTopDocuments("doc");
        vector<Document> testingRes;
        testingRes.push_back({ 2, 0.34657359027997264, 0 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        vector<Document> result = server.FindTopDocuments("Another document");
        vector<Document> testingRes;
        testingRes.push_back({ 3, 0.69314718055994529, 2 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }
}

// Проверка возврата правильного статуса документа
void TestDocumentStatus(void)
{
    SearchServer server;

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, { 5, 3, 10 }); // Avarage rating = 6
    server.AddDocument(1, "The second world"s, DocumentStatus::BANNED, { -10, -15, -20 }); // Avarage rating = -15
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::IRRELEVANT, { 1, 0, 0 }); // Avarage rating = 0
    server.AddDocument(3, "Another document in server"s, DocumentStatus::REMOVED, { 1, 2, 3 }); // Avarage rating = 2

    {
        vector<Document> result = server.FindTopDocuments("Hello", DocumentStatus::ACTUAL);
        vector<Document> testingRes;
        testingRes.push_back({ 0, 0.69314718055994529, 6 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        vector<Document> result = server.FindTopDocuments("second", DocumentStatus::BANNED);
        vector<Document> testingRes;
        testingRes.push_back({ 1, 0.46209812037329684, -15 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        vector<Document> result = server.FindTopDocuments("doc", DocumentStatus::IRRELEVANT);
        vector<Document> testingRes;
        testingRes.push_back({ 2, 0.34657359027997264, 0 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }

    {
        vector<Document> result = server.FindTopDocuments("Another document", DocumentStatus::REMOVED);
        vector<Document> testingRes;
        testingRes.push_back({ 3, 0.69314718055994529, 2 });
        ASSERT(testingRes.front().id == result.front().id &&
            testingRes.front().relevance == result.front().relevance &&
            testingRes.front().rating == result.front().rating);
    }
}

void TestRatingCalculation(void)
{
    SearchServer server;
    vector<int> test1 = { 1, 2, 3, 4, 5, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 };
    vector<int> test2 = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178 };
    vector<int> test3 = { -1, -3, -5, -7, -9, -11, -13, -15, -17, -19, -21, -23, -25, -27, -29, -31, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, -55, -57, -59, -61, -63, -65, -67, -69, -71, -73, -75, -77, -79, -81, -83, -85, -87, -89, -91, -93, -95, -97, -99, -101, -103, -105, -107, -109, -111, -113, -115, -117, -119, -121, -123, -125, -127, -129, -131, -133, -135, -137, -139, -141, -143, -145, -147, -149, -151, -153, -155, -157, -159, -161, -163, -165, -167, -169, -171, -173, -175, -177 };
    vector<int> test4 = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 1, 2, 3, 4, 5, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, -21, -23, -25, -27, -29, -31, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, -55, -57, -59, -61, -63, -65, -67, -69, 4, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 };

    server.AddDocument(0, "Hello world"s, DocumentStatus::ACTUAL, test1);
    server.AddDocument(1, "The second world"s, DocumentStatus::BANNED, test2);
    server.AddDocument(2, "Third doc in server"s, DocumentStatus::IRRELEVANT, test3);
    server.AddDocument(3, "Another document in server"s, DocumentStatus::REMOVED, test4);

    {
        vector<Document> result = server.FindTopDocuments("Hello", DocumentStatus::ACTUAL);
        ASSERT_EQUAL(result.front().rating, 2);
    }

    {
        vector<Document> result = server.FindTopDocuments("second", DocumentStatus::BANNED);
        ASSERT_EQUAL(result.front().rating, 90);
    }

    {
        vector<Document> result = server.FindTopDocuments("doc", DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(result.front().rating, -89);
    }

    {
        vector<Document> result = server.FindTopDocuments("Another document", DocumentStatus::REMOVED);
        ASSERT_EQUAL(result.front().rating, -4);
    }
}

// Тест корректного вычисления релевантности найденных документов.
void TestCorrectRelevance(void)
{
    SearchServer server;
    vector<string> documents = { "first document at server excel"s, "second document in system word"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        vector<int> ratingVec = { 10 };
        for (size_t j = 0; j < i; j++)
        {
            ratingVec.push_back(((j + 1) * (i + 3) % 100));
        }
        server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, ratingVec);
    }
    vector<Document> result = server.FindTopDocuments("excel word document in", DocumentStatus::ACTUAL);
    ASSERT_EQUAL(result[0].relevance, 0.69314718055994529);
    ASSERT_EQUAL(result[1].relevance, 0.41588830833596718);
    ASSERT_EQUAL(result[2].relevance, 0.41588830833596718);
    ASSERT_EQUAL(result[3].relevance, 0.41588830833596718);
    ASSERT_EQUAL(result[4].relevance, 0.27725887222397810);
}

// Тест Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания
void TestCorrectOrderRelevance(void)
{
    SearchServer server;
    vector<string> documents = { "first document at server excel with some programms"s, "second document in system word and some execute files"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        vector<int> ratingVec = { 10 };
        for (size_t j = 0; j < i; j++)
        {
            ratingVec.push_back(((j + 1) * (i + 3) % 100));
        }
        server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, ratingVec);
    }

    vector<Document> result = server.FindTopDocuments("excel word document in windows at execute some programms", DocumentStatus::ACTUAL);

    int doc_size = result.size() - 1;
    for (int i = 0; i < doc_size - 1; ++i) {
        ASSERT_HINT(result[i].relevance >= result[i + 1].relevance, "Неверная сортировка реливантности документа, i документ не может иметь релевантность меньше нижестоящего");
    }
}

// Тест Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestPredacateWorks(void)
{
    SearchServer server;
    vector<string> documents = { "first document at server excel with some programms"s, "second document in system word and some execute files"s, "third document of database adobe"s, "fourth document on configuration photoshop"s, "fifth document at test windows"s, "sixth document in server office"s, "seventh document of system linux"s, "eighth document on database plm"s, "ninth document at configuration cad"s, "tenth document in test solidworks"s, "eleventh document of server compas"s, "twelfth document on system excel"s, "thirteenth document at database word"s, "fourteenth document in configuration adobe"s, "fifteenth document of test photoshop"s, "sixteenth document on server windows"s };

    for (size_t i = 0; i < 16; i++)
    {
        vector<int> ratingVec = { 10 };
        for (size_t j = 0; j < i; j++)
        {
            ratingVec.push_back(((j + 1) * (i + 3) % 100));
        }
        server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, ratingVec);
    }

    vector<Document> result;
    {
        result = server.FindTopDocuments("excel word document in windows at execute some programms",
            [](int document_id, DocumentStatus status, int rating)
            {
                return document_id % 2 == 0;
            });

        int arrId[] = { 0, 12, 4, 8, 14 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].id, arrId[i], "Неверный ID документа при предикатном поиске");
        }

        int arrRating[] = { 10, 44, 16, 45, 39 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].rating, arrRating[i], "Неверный рейтинг при предикатном поиске");
        }

        double arrRelevance[] = { 1.0397207708399179, 0.69314718055994529, 0.69314718055994529, 0.27725887222397810, 0.0000000000000000 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].relevance, arrRelevance[i], "Неверная релевантность при предикатном поиске");
        }
    }

    {
        result = server.FindTopDocuments("excel word document in windows at execute some programms",
            [](int document_id, DocumentStatus status, int rating)
            {
                return rating > 35;
            });

        int arrId[] = { 12, 15, 11, 13, 8 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].id, arrId[i], "Неверный ID документа при предикатном поиске");
        }

        int arrRating[] = { 44, 48, 44, 47, 45 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].rating, arrRating[i], "Неверный рейтинг при предикатном поиске");
        }

        double arrRelevance[] = { 0.69314718055994529, 0.41588830833596718, 0.41588830833596718, 0.27725887222397810, 0.27725887222397810 };
        for (size_t i = 0; i < 5; i++)
        {
            ASSERT_EQUAL_HINT(result[i].relevance, arrRelevance[i], "Неверная релевантность при предикатном поиске");
        }
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordsExcludeDocumentFromQuery);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestSortingDocumentsWithRelevance);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestDocumentStatus);
    RUN_TEST(TestRatingCalculation);
    RUN_TEST(TestCorrectRelevance);
    RUN_TEST(TestCorrectOrderRelevance);
    RUN_TEST(TestPredacateWorks);
}
// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}