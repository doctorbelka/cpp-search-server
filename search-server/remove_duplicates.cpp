#include "remove_duplicates.h"
#include <vector>
using namespace std;

void RemoveDuplicates(SearchServer& search_server){
    vector<int> id_to_remove;
    map<int, vector<string>> words_in_doc;
    for (const int document_id : search_server) {
        for (auto [word, _] : search_server.GetWordFrequencies(document_id)) {
            words_in_doc[document_id].push_back(word);
        }
    }
    for (const int document_id : search_server){
        if (count(id_to_remove.begin(), id_to_remove.end(), document_id)) continue;
        for (auto i=search_server.begin();i!=search_server.end();++i){
            if (*i==document_id) continue;
            if (count(id_to_remove.begin(), id_to_remove.end(), *i)) continue;
            if (words_in_doc[document_id]==words_in_doc[*i]) {
                if (*i > document_id) {
                    id_to_remove.push_back(*i);
                    cout << "Found duplicate document id "s << *i << "\n";
                }
                else {
                    id_to_remove.push_back(document_id);
                    cout << "Found duplicate document id "s << document_id << "\n";
                }
            }
        }
    }
    for (const int& id : id_to_remove){
        search_server.RemoveDocument(id);
    }
}