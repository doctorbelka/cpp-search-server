#include "search_server.h"
#include <numeric>
using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text)) 
    {
    }

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Invalid document_id"s);
        }
        const auto words = SearchServer::SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id]+=inv_word_count;
            id_to_word_freqs_[document_id][word]+=inv_word_count;
        }
   
        documents_.emplace(document_id, SearchServer::DocumentData{SearchServer::ComputeAverageRating(ratings), status});
        document_ids_.insert(document_id);
    }

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return SearchServer::FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

 vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
        return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }



tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
        const auto query = SearchServer::ParseQuery(raw_query);

        vector<string> matched_words;
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
        return {matched_words, documents_.at(document_id).status};
    }


bool SearchServer::IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

bool SearchServer::IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!SearchServer::IsValidWord(word)) {
                throw invalid_argument("Word "s + word + " is invalid"s);
            }
            if (!SearchServer::IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(),ratings.end(),0);
        
        return rating_sum / static_cast<int>(ratings.size());
    }

SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
        if (text.empty()) {
            throw invalid_argument("Query word is empty"s);
        }
        string word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
            throw invalid_argument("Query word "s + text + " is invalid");
        }

        return {word, is_minus, SearchServer::IsStopWord(word)};
    }

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
        SearchServer::Query result;
        for (const string& word : SplitIntoWords(text)) {
            const auto query_word = SearchServer::ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                } else {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return result;
    }

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
        return log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const{
    const static map<string,double> res;
    if (find(SearchServer::begin(),SearchServer::end(),document_id)==SearchServer::end()){ 
        return res;
    }
   else {
       return id_to_word_freqs_.at(document_id);
   }
    return res;
}

void SearchServer::RemoveDocument(int document_id){
    for (auto [word, _] : id_to_word_freqs_.at(document_id))
 {
            word_to_document_freqs_.at(word).erase(document_id);
     if (word_to_document_freqs_.at(word).empty()) {
               word_to_document_freqs_.erase(word);
           }
    }
    id_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
        
}