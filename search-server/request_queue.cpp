#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
	: server_(search_server)
{}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus statusDoc) {
	return (AddFindRequest(raw_query,
		[statusDoc](int document_id, DocumentStatus document_status, int rating) {
			return document_status == statusDoc;
		}));
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	return (AddFindRequest(raw_query,
		[](int document_id, DocumentStatus document_status, int rating) {
			return document_status == DocumentStatus::ACTUAL;
		}));
}

int RequestQueue::GetNoResultRequests() const {
	return (accumulate(requests_.begin(), requests_.end(), 0, [](int sum, const QueryResult& elem) {
		if (elem.numOfResults == 0)
			++sum;
		return sum;
		}));
}