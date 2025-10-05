#include "database.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <regex>
#include <map>

Database::Database() : connected_(false) {}

Database::~Database() {
    Disconnect();
}

bool Database::Connect(const std::string& host, int port, const std::string& dbname,
    const std::string& user, const std::string& password) {
    try {
        std::string connection_string =
            "host=" + host + " " +
            "port=" + std::to_string(port) + " " +
            "dbname=" + dbname + " " +
            "user=" + user + " " +
            "password=" + password;

        conn_ = std::make_unique<pqxx::connection>(connection_string);

        if (conn_->is_open()) {
            connected_ = true;
            std::cout << "Connected to PostgreSQL database: " << conn_->dbname() << std::endl;
            return true;
        }
        else {
            std::cerr << "Failed to connect to PostgreSQL" << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "PostgreSQL connection error: " << e.what() << std::endl;
        return false;
    }
}

void Database::Disconnect() {
    if (connected_ && conn_) {
        conn_->close();
        connected_ = false;
        std::cout << "Disconnected from PostgreSQL" << std::endl;
    }
}

bool Database::CreateTables() {
    if (!connected_) return false;

    try {
        pqxx::work txn(*conn_);

        // Таблица документов
        txn.exec(
            "CREATE TABLE IF NOT EXISTS documents ("
            "id SERIAL PRIMARY KEY,"
            "url TEXT UNIQUE NOT NULL,"
            "title TEXT,"
            "content TEXT,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"
        );

        // Таблица слов
        txn.exec(
            "CREATE TABLE IF NOT EXISTS words ("
            "id SERIAL PRIMARY KEY,"
            "word TEXT UNIQUE NOT NULL"
            ")"
        );

        // Таблица связей
        txn.exec(
            "CREATE TABLE IF NOT EXISTS document_words ("
            "document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE,"
            "word_id INTEGER REFERENCES words(id) ON DELETE CASCADE,"
            "frequency INTEGER NOT NULL,"
            "PRIMARY KEY (document_id, word_id)"
            ")"
        );

        // Индексы для улучшения производительности
        txn.exec("CREATE INDEX IF NOT EXISTS idx_words_word ON words(word)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_document_words_word_id ON document_words(word_id)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_documents_url ON documents(url)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_document_words_document_id ON document_words(document_id)");

        txn.commit();
        std::cout << "Database tables created successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating tables: " << e.what() << std::endl;
        return false;
    }
}

int Database::AddDocument(const std::string& url, const std::string& title, const std::string& content) {
    if (!connected_) return -1;

    try {
        pqxx::work txn(*conn_);

        // Проверяем существование документа
        pqxx::result result = txn.exec(
            "SELECT id FROM documents WHERE url = " + txn.quote(url)
        );

        if (!result.empty()) {
            return result[0][0].as<int>();
        }

        // Добавляем новый документ
        result = txn.exec(
            "INSERT INTO documents (url, title, content) VALUES (" +
            txn.quote(url) + ", " + txn.quote(title) + ", " + txn.quote(content) +
            ") RETURNING id"
        );

        int doc_id = result[0][0].as<int>();
        txn.commit();

        return doc_id;
    }
    catch (const std::exception& e) {
        std::cerr << "Error adding document: " << e.what() << std::endl;
        return -1;
    }
}

bool Database::DocumentExists(const std::string& url) {
    if (!connected_) return false;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec(
            "SELECT id FROM documents WHERE url = " + txn.quote(url)
        );
        return !result.empty();
    }
    catch (const std::exception& e) {
        std::cerr << "Error checking document existence: " << e.what() << std::endl;
        return false;
    }
}

bool Database::UpdateDocument(const std::string& url, const std::string& title, const std::string& content) {
    if (!connected_) return false;

    try {
        pqxx::work txn(*conn_);

        txn.exec(
            "UPDATE documents SET title = " + txn.quote(title) +
            ", content = " + txn.quote(content) +
            " WHERE url = " + txn.quote(url)
        );

        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating document: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Document> Database::GetAllDocuments() {
    std::vector<Document> documents;
    if (!connected_) return documents;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec("SELECT id, url, title, content FROM documents");

        for (const auto& row : result) {
            documents.emplace_back(
                row["id"].as<int>(),
                row["url"].as<std::string>(),
                row["title"].as<std::string>(),
                row["content"].as<std::string>()
            );
        }

        return documents;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting all documents: " << e.what() << std::endl;
        return documents;
    }
}

int Database::AddWord(const std::string& word) {
    if (!connected_) return -1;

    try {
        pqxx::work txn(*conn_);

        // Пытаемся вставить слово, если существует - возвращаем ID
        pqxx::result result = txn.exec(
            "INSERT INTO words (word) VALUES (" + txn.quote(word) +
            ") ON CONFLICT (word) DO UPDATE SET word = EXCLUDED.word RETURNING id"
        );

        int word_id = result[0][0].as<int>();
        txn.commit();
        return word_id;
    }
    catch (const std::exception& e) {
        std::cerr << "Error adding word: " << e.what() << std::endl;
        return -1;
    }
}

int Database::GetWordId(const std::string& word) {
    if (!connected_) return -1;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec(
            "SELECT id FROM words WHERE word = " + txn.quote(word)
        );

        if (result.empty()) {
            return -1;
        }

        return result[0][0].as<int>();
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting word ID: " << e.what() << std::endl;
        return -1;
    }
}

std::vector<std::string> Database::GetAllWords() {
    std::vector<std::string> words;
    if (!connected_) return words;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec("SELECT word FROM words");

        for (const auto& row : result) {
            words.push_back(row["word"].as<std::string>());
        }

        return words;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting all words: " << e.what() << std::endl;
        return words;
    }
}

void Database::AddDocumentWord(int document_id, int word_id, int frequency) {
    if (!connected_) return;

    try {
        pqxx::work txn(*conn_);

        txn.exec(
            "INSERT INTO document_words (document_id, word_id, frequency) VALUES (" +
            txn.quote(document_id) + ", " + txn.quote(word_id) + ", " + txn.quote(frequency) +
            ") ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = document_words.frequency + EXCLUDED.frequency"
        );

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Error adding document-word relationship: " << e.what() << std::endl;
    }
}

void Database::UpdateDocumentWords(int document_id, const std::map<std::string, int>& word_frequencies) {
    if (!connected_) return;

    try {
        pqxx::work txn(*conn_);

        // Очищаем старые связи
        ClearDocumentWords(document_id);

        // Добавляем новые связи
        for (const auto& [word, freq] : word_frequencies) {
            int word_id = AddWord(word);
            if (word_id != -1) {
                AddDocumentWord(document_id, word_id, freq);
            }
        }

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating document words: " << e.what() << std::endl;
    }
}

void Database::ClearDocumentWords(int document_id) {
    if (!connected_) return;

    try {
        pqxx::work txn(*conn_);
        txn.exec("DELETE FROM document_words WHERE document_id = " + txn.quote(document_id));
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Error clearing document words: " << e.what() << std::endl;
    }
}

std::vector<SearchResult> Database::SearchDocuments(const std::vector<std::string>& search_words, int limit) {
    std::vector<SearchResult> results;

    if (!connected_ || search_words.empty()) return results;

    try {
        pqxx::work txn(*conn_);

        // Строим запрос для поиска документов, содержащих все слова
        std::string query =
            "SELECT d.url, d.title, d.content, SUM(dw.frequency) as relevance "
            "FROM documents d "
            "JOIN document_words dw ON d.id = dw.document_id "
            "JOIN words w ON dw.word_id = w.id "
            "WHERE w.word IN (";

        for (size_t i = 0; i < search_words.size(); ++i) {
            if (i > 0) query += ", ";
            query += txn.quote(search_words[i]);
        }

        query +=
            ") "
            "GROUP BY d.id, d.url, d.title, d.content "
            "HAVING COUNT(DISTINCT w.word) = " + std::to_string(search_words.size()) + " "
            "ORDER BY relevance DESC "
            "LIMIT " + std::to_string(limit);

        pqxx::result result = txn.exec(query);

        for (const auto& row : result) {
            std::string url = row["url"].as<std::string>();
            std::string title = row["title"].as<std::string>();
            std::string content = row["content"].as<std::string>();
            int relevance = row["relevance"].as<int>();

            std::string snippet = GenerateSnippet(content, search_words);
            results.emplace_back(url, title, snippet, relevance);
        }

        return results;
    }
    catch (const std::exception& e) {
        std::cerr << "Error searching documents: " << e.what() << std::endl;
        return results;
    }
}

std::string Database::GenerateSnippet(const std::string& content, const std::vector<std::string>& search_words) {
    if (content.length() <= 200) {
        return content;
    }

    // Если есть поисковые слова, попробуем найти контекст вокруг них
    if (!search_words.empty()) {
        std::string content_lower = content;
        std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // Ищем первое вхождение любого из поисковых слов
        for (const auto& word : search_words) {
            size_t pos = content_lower.find(word);
            if (pos != std::string::npos) {
                // Нашли слово, берем контекст вокруг него
                size_t start = (pos > 100) ? pos - 100 : 0;
                size_t end = std::min(content.length(), start + 200);
                std::string snippet = content.substr(start, end - start);

                if (start > 0) snippet = "..." + snippet;
                if (end < content.length()) snippet = snippet + "...";

                return snippet;
            }
        }
    }

    // Если не нашли поисковых слов или они не найдены в тексте, возвращаем начало
    return content.substr(0, 200) + "...";
}

void Database::PrintStats() {
    if (!connected_) return;

    try {
        pqxx::work txn(*conn_);

        auto doc_count = txn.exec("SELECT COUNT(*) FROM documents")[0][0].as<int>();
        auto word_count = txn.exec("SELECT COUNT(*) FROM words")[0][0].as<int>();
        auto rel_count = txn.exec("SELECT COUNT(*) FROM document_words")[0][0].as<int>();

        std::cout << "=== Database Statistics ===" << std::endl;
        std::cout << "Documents: " << doc_count << std::endl;
        std::cout << "Words: " << word_count << std::endl;
        std::cout << "Document-Word relationships: " << rel_count << std::endl;
        std::cout << "===========================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting statistics: " << e.what() << std::endl;
    }
}

int Database::GetDocumentCount() {
    if (!connected_) return 0;
    try {
        pqxx::work txn(*conn_);
        auto result = txn.exec("SELECT COUNT(*) FROM documents");
        return result[0][0].as<int>();
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting document count: " << e.what() << std::endl;
        return 0;
    }
}

int Database::GetWordCount() {
    if (!connected_) return 0;
    try {
        pqxx::work txn(*conn_);
        auto result = txn.exec("SELECT COUNT(*) FROM words");
        return result[0][0].as<int>();
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting word count: " << e.what() << std::endl;
        return 0;
    }
}

int Database::GetDocumentWordCount() {
    if (!connected_) return 0;
    try {
        pqxx::work txn(*conn_);
        auto result = txn.exec("SELECT COUNT(*) FROM document_words");
        return result[0][0].as<int>();
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting document-word count: " << e.what() << std::endl;
        return 0;
    }
}