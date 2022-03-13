#include "remove_duplicates.h"

using std::string_literals::operator""s;

void RemoveDuplicates(SearchServer& search_server)
{
	std::map<std::set<std::string>, std::vector<int>> doc_to_del;
	std::set<int> remove_docs;
	for (const int document_id : search_server)
	{
		std::set<std::string> words;
		std::map<std::string, double> word_with_freq = search_server.GetWordFrequencies(document_id);
		for (auto [word, freq] : word_with_freq)
		{
			words.insert(word);
		}
		doc_to_del[words].push_back(document_id);
	}

	for (auto doc : doc_to_del)
	{
		if (doc.second.size() > 1)
		{
			remove_docs.insert(std::next(doc.second.begin()), doc.second.end());
		}
	}

	for (const int to_remove : remove_docs)
	{
		search_server.RemoveDocument(to_remove);
	}
}