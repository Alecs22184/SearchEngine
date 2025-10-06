#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

struct Document {
    int id;
    std::string url;
    std::string title;
    std::string content;

    Document(int id, const std::string& url, const std::string& title, const std::string& content)
        : id(id), url(url), title(title), content(content) {
    }
};

struct SearchResult {
    std::string url;
    std::string title;
    std::string snippet;
    int relevance;

    SearchResult(const std::string& url, const std::string& title,
        const std::string& snippet, int relevance)
        : url(url), title(title), snippet(snippet), relevance(relevance) {
    }
};

class Database {
public:
    Database();
    ~Database();

    bool Connect(const std::string& host, int port, const std::string& dbname,
        const std::string& user, const std::string& password);
    void Disconnect();
    bool CreateTables();

    // Document operations
    int AddDocument(const std::string& url, const std::string& title, const std::string& content);
    bool DocumentExists(const std::string& url);
    bool UpdateDocument(const std::string& url, const std::string& title, const std::string& content);
    std::vector<Document> GetAllDocuments();

    // Word operations
    int AddWord(const std::string& word);
    int GetWordId(const std::string& word);
    std::vector<std::string> GetAllWords();

    // Document-Word relationships
    void AddDocumentWord(int document_id, int word_id, int frequency);
    void UpdateDocumentWords(int document_id, const std::map<std::string, int>& word_frequencies);
    void ClearDocumentWords(int document_id);

    // Search
    std::vector<SearchResult> SearchDocuments(const std::vector<std::string>& search_words, int limit);

    // Statistics
    void PrintStats();
    int GetDocumentCount();
    int GetWordCount();
    int GetDocumentWordCount();

private:
    std::string GenerateSnippet(const std::string& content, const std::vector<std::string>& search_words);

    std::unique_ptr<pqxx::connection> conn_;
    std::mutex db_mutex_;
    bool connected_ = false;
};

#endif // DATABASE_H