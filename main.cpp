#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <cmath>

using namespace std;

#define MAX_RESULT_DOCUMENT_COUNT 5

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    istringstream query_word(text);
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
    void SetStopWords(const string &text) {
        for (const string &word: SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, const vector<int> &ratings) {
        ++document_count_;
        document_rating_[document_id] = ComputeAverageRating(ratings);
        const vector<string> words = SplitIntoWordsNoStop(document);
        double TF = 1.0 / words.size();
        for (const string &word: words) {
            word_to_documents_[word][document_id] += TF;
        }
    }

    vector<Document> FindTopDocuments(const string &raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) { return lhs.relevance > rhs.relevance; });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct DocumentContent {
        int id = 0;
        vector<string> words;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<int, int> document_rating_;
    int document_count_ = 0;

    map<string, map<int, double>> word_to_documents_;

    set<string> stop_words_;

    int ComputeAverageRating(const vector<int> &rating) {
        if (rating.empty()) {
            return 0;
        }
        int middle_rating = 0;
        for (const int rate: rating) {
            middle_rating += rate;
        }
        return middle_rating / (int) rating.size();
    }

    bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
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

    Query ParseQuery(const string &text) const {
        Query query_words;
        for (const string &word: SplitIntoWords(text)) {
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

    vector<Document> FindAllDocuments(const Query &query_words) const {
        map<int, double> document_to_relevance;
        for (const auto &word: query_words.plus_words) {
            if (word_to_documents_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, temp]: word_to_documents_.at(word)) {
                document_to_relevance[document_id] +=
                        temp * log(document_count_ * 1.0 / word_to_documents_.at(word).size());
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
        vector<Document> matched_documents;
        for (const auto [document_id, relevance]: document_to_relevance) {
            matched_documents.push_back({document_id, relevance, document_rating_.at(document_id)});
        }
        return matched_documents;
    }

    static int MatchDocument(const DocumentContent &content, const Query &query_words) {
        if (query_words.plus_words.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const string &word: content.words) {
            if (query_words.minus_words.count(word) != 0) {
                return 0;
            }
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query_words.plus_words.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return (int) matched_words.size();
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const string document = ReadLine();
        int rating_size = 0;
        cin >> rating_size;
        vector<int> ratings(rating_size, 0);
        for (int &rating: ratings) {
            cin >> rating;
        }
        search_server.AddDocument(document_id, document, ratings);
        ReadLine();
    }
    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto &document: search_server.FindTopDocuments(query)) {
        cout << "{ "
             << "document_id = " << document.id << ", "
             << "relevance = " << document.relevance << ", "
             << "rating = " << document.rating
             << " }" << endl;
    }
}