#pragma once
#include "document.h"
#include "search_server.h"

void PrintDocument(const Document& document);
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);

template <typename Container>
void Print(std::ostream& out, const Container& container)
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