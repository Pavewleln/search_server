#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <cmath>
#include <tuple>


#define MAX_RESULT_DOCUMENT_COUNT 5

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector<std::string> SplitIntoWords(const std::string &text) {
    std::vector<std::string> words;
    std::string word;
    std::istringstream query_word(text);
    while (query_word >> word) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:
    enum class DocumentStatus {
        ACTUAL,
        IRRELEVANT,
        BANNED,
        REMOVED
    };

    void SetStopWords(const std::string &text) {
        for (const std::string &word: SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void
    AddDocument(int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &ratings) {
        ++document_count_;
        document_data_[document_id] = {ComputeAverageRating(ratings), status};
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        double TF = 1.0 / words.size();
        for (const std::string &word: words) {
            word_to_documents_[word][document_id] += TF;
        }
    }

    std::vector<Document>
    FindTopDocuments(const std::string &raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, status);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

    int GetDocumentCount() const {
        return document_data_.size();
    }

    std::tuple<std::vector<std::string>, DocumentStatus>
    MatchDocument(const std::string &raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        std::vector<std::string> matched_words;
        for (const std::string &word: query.plus_words) {
            if (word_to_documents_.count(word) == 0) {
                continue;
            }
            if (word_to_documents_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string &word: query.minus_words) {
            if (word_to_documents_.count(word) == 0) {
                continue;
            }
            if (word_to_documents_.at(word).count(document_id)) {
                matched_words.clear();
            }
        }
        return {matched_words, document_data_.at(document_id).status};
    }

private:

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    struct DocumentData {
        int document_rating;
        DocumentStatus status;
    };
    int document_count_ = 0;

    std::map<int, DocumentData> document_data_;

    std::map<std::string, std::map<int, double>> word_to_documents_;

    std::set<std::string> stop_words_;

    static int ComputeAverageRating(const std::vector<int> &ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int middle_rating = 0;
        for (const int rating: ratings) {
            middle_rating += rating;
        }
        return middle_rating / (int) ratings.size();
    }

    bool IsStopWord(const std::string &word) const {
        return stop_words_.count(word) > 0;
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const {
        std::vector<std::string> words;
        for (const std::string &word: SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const std::string &text) const {
        Query query_words;
        for (const std::string &word: SplitIntoWords(text)) {
            if (!IsStopWord(word[0] == '-' ? word.substr(1) : word)) {
                if (word[0] == '-') {
                    query_words.minus_words.insert(word.substr(1));
                } else {
                    query_words.plus_words.insert(word);
                }
            }
        }
        return query_words;
    }

    std::vector<Document> FindAllDocuments(const Query &query_words, DocumentStatus status) const {
        std::map<int, double> document_to_relevance;
        for (const auto &word: query_words.plus_words) {
            if (word_to_documents_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, temp]: word_to_documents_.at(word)) {
                if (document_data_.at(document_id).status == status) {
                    document_to_relevance[document_id] +=
                            temp * log(document_count_ * 1.0 / word_to_documents_.at(word).size());
                }
            }
        }
        for (const auto &word: query_words.minus_words) {
            if (word_to_documents_.count(word) == 0) {
                continue;
            }
            for (const auto &[document_id, _]: word_to_documents_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance]: document_to_relevance) {
            matched_documents.push_back({document_id, relevance, document_data_.at(document_id).document_rating});
        }
        return matched_documents;
    }
};
//
//SearchServer CreateSearchServer() {
//    SearchServer search_server;
//    search_server.SetStopWords(ReadLine());
//    const int document_count = ReadLineWithNumber();
//    for (int document_id = 0; document_id < document_count; ++document_id) {
//        const string document = ReadLine();
//        int rating_size = 0;
//        cin >> rating_size;
//        vector<int> ratings(rating_size, 0);
//        for (int &rating: ratings) {
//            cin >> rating;
//        }
//        search_server.AddDocument(document_id, document, ratings);
//        ReadLine();
//    }
//    return search_server;
//}

void PrintDocument(const Document &document) {
    std::cout << "{ "
              << "document_id = " << document.id << ", "
              << "relevance = " << document.relevance << ", "
              << "rating = " << document.rating
              << " }" << std::endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на");
    search_server.AddDocument(0, "белый кот и модный ошейник", SearchServer::DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост", SearchServer::DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза", SearchServer::DocumentStatus::ACTUAL,
                              {5, -12, 2, 1});
    for (const Document &document: search_server.FindTopDocuments("ухоженный кот")) {
        PrintDocument(document);
    }
}