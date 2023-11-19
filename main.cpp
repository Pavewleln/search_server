#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "./testFramework/testFramework.cpp"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    explicit SearchServer(const string &text) : documents_(), word_to_document_freqs_() {
        for (const string &word: SplitIntoWords(text)) {
            stop_words_.insert(word);
        };
    }

    template<typename StringCollection>
    explicit SearchServer(const StringCollection &stop_words): documents_(), word_to_document_freqs_() {
        for (const string &word: stop_words) {
            stop_words_.insert(word);
        };
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template<typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string &raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

    vector<Document> FindTopDocuments(const string &raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) {
            return status == DocumentStatus::ACTUAL;
        });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string &word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string &word: query.minus_words) {
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWords(const string &text) const {
        vector<string> words;
        string word;
        for (const char c: text) {
            if (c == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else {
                word += c;
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }

        return words;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const {
        vector<string> words;
        for (const string &word: SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating: ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    Query ParseQuery(const string &text) const {
        Query query;
        for (const string &word: SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string &word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance]: document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }
};

// --------- Начало модульных тестов поисковой системы ---------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(SearchServer("in the"s));
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_HINT(found_docs.size() == 1, "Document failed"s);
        const Document doc0 = found_docs[0];
        ASSERT_HINT(doc0.id == doc_id, "Document failed"s);
    }
    {
        SearchServer server(SearchServer("in the"s));
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "FindTopDocuments failed"s);
    }
    cout << "TestExcludeStopWordsFromAddedDocumentContent success" << endl;
}

void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
}

// --------- Окончание модульных тестов поисковой системы -----------


// ==================== для примера =========================


void PrintDocument(const Document &document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    TestSearchServer();
    SearchServer search_server(SearchServer("и в на"s));
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document &document: search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const Document &document: search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                                                  [](int document_id, DocumentStatus status,
                                                                     int rating) {
                                                                      return status == DocumentStatus::ACTUAL;
                                                                  })) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document &document: search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                                                  [](int document_id, DocumentStatus status,
                                                                     int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}