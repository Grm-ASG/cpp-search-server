#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
	: server_(search_server)
{}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus doc_status)
{
	return (AddFindRequest(raw_query,
		[doc_status](int document_id, DocumentStatus document_status, int rating)
		{
			return doc_status == document_status;
		}));
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query)
{
	return (AddFindRequest(raw_query, DocumentStatus::ACTUAL));
}

int RequestQueue::GetNoResultRequests() const
{
	return (std::accumulate(requests_.begin(), requests_.end(), 0, [](int sum, const QueryResult& elem)
		{
			if (elem.num_of_results == 0)
			{
				++sum;
			}
			return sum;
		}));
}
