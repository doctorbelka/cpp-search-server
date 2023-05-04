#include "process_queries.h"
#include <algorithm>
#include <execution>
#include <utility>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
  std::vector<std::vector<Document>> result(queries.size()); 
    std::transform(std::execution::par,queries.begin(),queries.end(),result.begin(),[&search_server](auto query){return search_server.FindTopDocuments(query);});
    return result;
} 

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<Document> result;
    for (auto doc : ProcessQueries(search_server,queries)){
            for (auto doc2 : doc){
                result.push_back(std::move(doc2));
            }
        
    }
    return result;
} 