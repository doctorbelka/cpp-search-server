#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
   
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool not_empty;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& inner_server_;
    
}; 

template <typename DocumentPredicate>
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    using namespace std;
        vector<Document> docs=inner_server_.FindTopDocuments(raw_query,document_predicate);
        if (docs.empty()){
        requests_.push_back({false});
          if (requests_.size()>min_in_day_)
            requests_.pop_front();
        }
        else {
        requests_.push_back({true});
          if (requests_.size()>min_in_day_)
            requests_.pop_front();
        }
        return docs;
    }