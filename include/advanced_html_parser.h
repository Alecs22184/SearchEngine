#ifndef ADVANCED_HTML_PARSER_H
#define ADVANCED_HTML_PARSER_H

#include <string>
#include <vector>

class AdvancedHtmlParser {
public:
    static std::string ExtractText(const std::string& html);
    static std::string GetPageTitle(const std::string& html);
    static std::vector<std::string> ExtractMetaKeywords(const std::string& html);
    static std::string ExtractMetaDescription(const std::string& html);

private:
    static void RemoveComments(std::string& html);
    static void RemoveScriptsAndStyles(std::string& html);
};

#endif // ADVANCED_HTML_PARSER_H