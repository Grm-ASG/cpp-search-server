#include "print_functions.h"

void PrintDocument(const Document& document) {
    using std::string_literals::operator""s;
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    using std::string_literals::operator""s;
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

std::ostream& operator<<(std::ostream& out, const Document& doc) {
    using std::string_literals::operator""s;
    out << "document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating;
    return (out);
}