#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include <string>
#include <vector>

class HtmlParser {
public:
    static std::string ExtractText(const std::string& html);
    static std::vector<std::string> ExtractLinks(const std::string& html, const std::string& base_url);
    static std::vector<std::string> ExtractWords(const std::string& text);
    static std::string GetPageTitle(const std::string& html);
    static std::string NormalizeUrl(const std::string& url, const std::string& base_url);

private:
    static std::string CleanText(const std::string& text);
    static bool IsValidWord(const std::string& word);
    static std::string ToLower(const std::string& str);
};

#endif // HTML_PARSER_H