#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <sstream>

#define MAX_RESULT_DOCUMENT_COUNT 5

using namespace std;

string GetLine() {
    string s;
    getline(cin, s);
    return s;
}
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    GetLine();
    return result;
}

vector<string> ParseStringToVector(const string &str) {
    vector<string> words;
    string word;
    istringstream substr(str);
    while (substr >> word) {
        words.push_back(word);
    }
    return words;
}

set<string> ParseStringToSet(const string &str) {
    set<string> words;
    string word;
    istringstream substr(str);
    while (substr >> word) {
        words.insert(word);
    }
    return words;
}

vector<string> SplitIntoWordsNoStop(const string &document, const set<string> &stop_words) {
    vector<string> words;
    for (const string &word: ParseStringToVector(document)) {
        if (stop_words.count(word) == 0) {
            words.push_back(word);
        }
    }
    return words;
}

set<string> ParseQuery(const string& text, const set<string>& stop_words) {
    set<string> query_words;
    for (const string& word : SplitIntoWordsNoStop(text, stop_words)) {
        query_words.insert(word);
    }
    return query_words;
}


int MatchDocument(const pair<int, vector<string>>& content, const set<string>& query_words) {
    if (query_words.empty()) {
        return 0;
    }
    set<string> matched_words;
    for (const string& word : content.second) {
        if (matched_words.count(word) != 0) {
            continue;
        }
        if (query_words.count(word) != 0) {
            matched_words.insert(word);
        }
    }
    return static_cast<int>(matched_words.size());
}


void AddDocument(vector<pair<int, vector<string>>> &documents,
                 int document_id,
                 const set<string> &stop_words,
                 const string &document) {
    const vector<string> words = SplitIntoWordsNoStop(document, stop_words);
    documents.push_back({document_id, words});
}

// Для каждого документа возвращает пару {релевантность, id}
vector<pair<int, int>> FindAllDocuments(
        const vector<pair<int, vector<string>>> &documents,
        const set<string> &query_words) {
    vector<pair<int, int>> relevance_documents;
    for (const auto &document: documents) {
        const int relevance = MatchDocument(document, query_words);
        if (relevance > 0) {
            relevance_documents.push_back({relevance, document.first});
        }
    }
    return relevance_documents;
}

// Возвращает самые релевантные документы в виде вектора пар {id, релевантность}
vector<pair<int, int>> FindTopDocuments(
        const vector<pair<int, vector<string>>> &documents,
        const set<string> &stop_words, const string &raw_query) {
    set<string> query_words = ParseQuery(raw_query, stop_words);
    vector<pair<int, int>> relevance_documents = FindAllDocuments(documents, query_words);

    sort(relevance_documents.begin(), relevance_documents.end());
    reverse(relevance_documents.begin(), relevance_documents.end());

    if(relevance_documents.size() > MAX_RESULT_DOCUMENT_COUNT){
        relevance_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    for (auto& relevance_document : relevance_documents) {
        swap(relevance_document.first, relevance_document.second);
    }

    return relevance_documents;
}

int main() {
    const string str_stop_word_input = GetLine();
    const set<string> stop_words = ParseStringToSet(str_stop_word_input);
    const int document_count = ReadLineWithNumber();
    vector<pair<int, vector<string>>> documents;
    for (int document_id = 0; document_id < document_count; ++document_id) {
        AddDocument(documents, document_id, stop_words, GetLine());
    }
    const string raw_query = GetLine();
    for(auto [document_id, relevance]: FindTopDocuments(documents, stop_words, raw_query)){
        cout << "{ document_id = " << document_id << ", " << "relevance = " << relevance << " }" << endl;
    }

    return 0;
}