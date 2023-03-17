#include "remove_duplicates.h"
#include <vector>
using namespace std;

void RemoveDuplicates(SearchServer& search_server){
    vector<int> id_to_remove;
    vector<string> words;
    map<vector<string>, int> words_in_doc;
    for (const int document_id : search_server) {
        for (auto [word, _] : search_server.GetWordFrequencies(document_id)) {
            words.push_back(word);
        }
        if (words_in_doc.find(words)!=words_in_doc.end()){
            id_to_remove.push_back(document_id);
            cout<<"Found duplicate document id "s<<document_id<<"\n"s;
            words.clear();
        }
        else {
            words_in_doc.insert({words,document_id});
            words.clear();
        }
    }
    for (const int& id : id_to_remove){
        search_server.RemoveDocument(id);
    }
}