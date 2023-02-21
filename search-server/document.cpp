#include "document.h"
using namespace std;

 Document::Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }


ostream& operator <<(ostream& out, const Document& doc) {
    out << "{ "s << "document_id = "s << doc.id << ", "s << "relevance = "s << doc.relevance << ", "s << "rating = "s << doc.rating << " }"s;
    return out;
}