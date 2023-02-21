#pragma once
#include "search_server.h"
#include <vector>

template<typename Iterator>
class Paginator{
public:
Paginator(Iterator begin, Iterator end, size_t size){
for (Iterator i=begin;i<end;advance(i,size)){
   if (end-i<size)
    DocsOnPage_.push_back({i,end-1});
    else DocsOnPage_.push_back({i,i+size-1});
}
}

auto begin() const{
return DocsOnPage_.begin();
}

auto end() const{
return DocsOnPage_.end();
}

size_t size() const{
return DocsOnPage_.size();
}


private:
std::vector<std::pair<Iterator,Iterator>> DocsOnPage_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator <<(std::ostream& out, std::pair<Iterator,Iterator> It) {
for (auto i=It.first;i<=It.second;++i){
    out << *i;
    }
    return out;
}