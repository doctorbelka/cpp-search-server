#include "search_server.h"
#include <numeric>
using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text)) 
    {
    }

SearchServer::SearchServer(const string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text))
{
}


void SearchServer::AddDocument(int document_id,string_view document, DocumentStatus status, const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Invalid document_id"s);
        }
        const auto words = SearchServer::SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            words_.push_back(word);
            word_to_document_freqs_[*find(words_.begin(),words_.end(),word)][document_id]+=inv_word_count;
            id_to_word_freqs_[document_id][*find(words_.begin(),words_.end(),word)]+=inv_word_count;
        }
   
        documents_.emplace(document_id, SearchServer::DocumentData{SearchServer::ComputeAverageRating(ratings), status});
        document_ids_.insert(document_id);
    }


vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
        return SearchServer::FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

 vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
        return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }


int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

    
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
if (document_ids_.find(document_id)==document_ids_.end()) throw out_of_range("Out of range"s);
    
        const static auto query = SearchServer::ParseQuery(raw_query);
   
        vector<string_view> matched_words;
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

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&,string_view raw_query, int document_id) const{
    return SearchServer::MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&,  string_view raw_query, int document_id) const {
    if (document_ids_.find(document_id) == document_ids_.end()) throw out_of_range("Out of range"s);
   static auto query = SearchServer::ParseQuery(raw_query, true);
  
    if (any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), [this, document_id](auto& word) {
        return word_to_document_freqs_.at(SearchServer::ParseQueryWord(word).data).count(document_id); })) return { {}, documents_.at(document_id).status };
        vector<string_view> matched_words(query.plus_words.size());
       auto it= copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, &document_id](auto& word) {
            return word_to_document_freqs_.at(word).count(document_id); });
    sort(matched_words.begin(),it);
    auto last = unique(matched_words.begin(), it);
   
        matched_words.erase(last, matched_words.end());
        return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

bool SearchServer::IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }


vector<string> SearchServer::SplitIntoWordsNoStop(string_view text) const {
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


SearchServer::Query SearchServer::ParseQuery(string_view text, bool par) const {
    SearchServer::Query result;
    if (!par){
        
        for (const string& word : SplitIntoWords(text)) {
            const auto query_word = SearchServer::ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.push_back(query_word.data);
                } else {
                    result.plus_words.push_back(query_word.data);
                }
            }
        }
    sort(result.plus_words.begin(),result.plus_words.end());
    auto last=unique(result.plus_words.begin(),result.plus_words.end());
    result.plus_words.erase(last,result.plus_words.end());
    sort(result.minus_words.begin(),result.minus_words.end());
    last=unique(result.minus_words.begin(),result.minus_words.end());
    result.minus_words.erase(last,result.minus_words.end());
        return result;
    }
    else{
     
        for (const string& word : SplitIntoWords(text)) {
            const auto query_word = SearchServer::ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.push_back(query_word.data);
                } else {
                    result.plus_words.push_back(query_word.data);
                }
            }
        }
    return result;
    }
    }

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
        return log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }



const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const{
    const static map<string_view,double> res;
    if (find(SearchServer::begin(),SearchServer::end(),document_id)==SearchServer::end()){ 
        return res;
    }
   else {
       return id_to_word_freqs_.at(document_id);
   }
    return res;
}

void SearchServer::RemoveDocument(int document_id){
    if (document_ids_.find(document_id)==document_ids_.end()) return;
    for (auto [word, _] : id_to_word_freqs_.at(document_id))
 {
           auto it = word_to_document_freqs_.find(word);
        if (it != word_to_document_freqs_.end()) {
            it->second.erase(document_id);
            if (it->second.empty()) {
                word_to_document_freqs_.erase(it);
            }
        }
    }
    id_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
        
}


void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id){
    if (document_ids_.find(document_id)==document_ids_.end()) return;
    std::vector<const std::string_view*> to_delete(id_to_word_freqs_.at(document_id).size());
    
    transform( id_to_word_freqs_.at(document_id).begin(),id_to_word_freqs_.at(document_id).end(),to_delete.begin(),[](auto& mapa){return &mapa.first;});
    for_each(std::execution::par,to_delete.begin(),to_delete.end(),[this, &document_id](auto& word) {
        auto it = word_to_document_freqs_.find(*word);
        if (it != word_to_document_freqs_.end()) {
            it->second.erase(document_id);
          
        }
    });
     for_each(to_delete.begin(),to_delete.end(),[this, &document_id](auto& word){
         auto it = word_to_document_freqs_.find(*word);
        if (it != word_to_document_freqs_.end())
         
                word_to_document_freqs_.erase(it);
         
     });
    id_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id){
      SearchServer::RemoveDocument(document_id);
    }

