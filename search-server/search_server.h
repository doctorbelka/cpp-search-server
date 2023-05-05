#pragma once
#include "document.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <iostream>
#include <string>
#include <execution>
#include <cmath>
#include <type_traits>
#include <utility>
#include <tuple>
#include <iterator>
#include <string_view>
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON=1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
      
    explicit SearchServer(const std::string& stop_words_text);
    
    explicit SearchServer(const std::string_view stop_words_text);
  
    
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query,DocumentPredicate document_predicate) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;
    
    int GetDocumentCount() const;
    
    std::set<int>::const_iterator begin() const{
        return document_ids_.begin();
    }
    
   
    std::set<int>::const_iterator end() const{
        return document_ids_.end();
    }
    
    using matchtuple = std::tuple<std::vector<std::string_view>, DocumentStatus>;
    matchtuple MatchDocument(std::string_view raw_query, int document_id) const;
    matchtuple MatchDocument(const std::execution::sequenced_policy&,std::string_view raw_query, int document_id) const;
    matchtuple MatchDocument(const std::execution::parallel_policy&,std::string_view raw_query, int document_id) const;
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    
    void GetStopWords(){
        for (auto word : stop_words_){
            std::cout<<word<<std::endl;
        }
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::deque<std::string> words_;
    const std::set<std::string> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;         //set
    std::map<int ,std::map<std::string_view, double>> id_to_word_freqs_;

    bool IsStopWord(const std::string& word) const;

    static bool IsValidWord(const std::string& word);

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    std::vector<std::string> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string& text) const;

    struct Query {
        std::vector<std::string> plus_words;
        std::vector<std::string> minus_words;
    };

    
    Query ParseQuery(std::string_view text, bool par=false) const;


    
    double ComputeWordInverseDocumentFreq(const std::string& word) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
     DocumentPredicate document_predicate) const;
    
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query,
     DocumentPredicate document_predicate) const;
};
    
 

 template <typename StringContainer>
  SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) 
    {
            using namespace std;
        if (!all_of(stop_words_.begin(), stop_words_.end(), SearchServer::IsValidWord)) {
            throw invalid_argument("Some of stop words are invalid"s);
        }
    }
    
template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,DocumentPredicate document_predicate) const {
    using namespace std;
        const auto query = SearchServer::ParseQuery(raw_query);

        auto matched_documents = SearchServer::FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query,DocumentPredicate document_predicate) const{
    using namespace std;
    if (std::is_same_v<ExecutionPolicy,std::execution::sequenced_policy>) return SearchServer::FindTopDocuments(raw_query, document_predicate);
        const auto query = SearchServer::ParseQuery(raw_query);

        auto matched_documents = SearchServer::FindAllDocuments(execution::par, query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
    

     template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const{
    if (std::is_same_v<ExecutionPolicy,std::execution::sequenced_policy>) return SearchServer::FindTopDocuments(raw_query, status);
    return SearchServer::FindTopDocuments(std::execution::par,
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    
    template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const{
    if (std::is_same_v<ExecutionPolicy,std::execution::sequenced_policy>) return SearchServer::FindTopDocuments(raw_query);
    return SearchServer::FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
    }




template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query,
     DocumentPredicate document_predicate) const {
    using namespace std;
       map<int, double> document_to_relevance;
   
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
 

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const SearchServer::Query& query,
     DocumentPredicate document_predicate) const {
    using namespace std;
   
        ConcurrentMap<int, double> document_to_relevance(100);
    for_each(execution::par,query.plus_words.begin(),query.plus_words.end(),[this,&document_predicate, &document_to_relevance](const string& word){if (word_to_document_freqs_.count(word) == 0) {
                return;
            }
            const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }});

    for_each(execution::par,query.minus_words.begin(),query.minus_words.end(),[this, &document_to_relevance](const string& word){ if (word_to_document_freqs_.count(word) == 0) {
                return;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }});

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

