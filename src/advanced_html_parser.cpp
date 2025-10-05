#include "advanced_html_parser.h"
#include <regex>
#include <algorithm>

std::string AdvancedHtmlParser::ExtractText(const std::string& html) {
    std::string text = html;

    RemoveComments(text);
    RemoveScriptsAndStyles(text);

    // Удаляем все HTML теги
    std::regex tag_regex("<[^>]*>");
    text = std::regex_replace(text, tag_regex, " ");

    // Заменяем HTML entities
    std::regex nbsp_regex("&nbsp;");
    text = std::regex_replace(text, nbsp_regex, " ");

    std::regex amp_regex("&amp;");
    text = std::regex_replace(text, amp_regex, "&");

    std::regex lt_regex("&lt;");
    text = std::regex_replace(text, lt_regex, "<");

    std::regex gt_regex("&gt;");
    text = std::regex_replace(text, gt_regex, ">");

    std::regex quot_regex("&quot;");
    text = std::regex_replace(text, quot_regex, "\"");

    // Удаляем множественные пробелы
    std::regex space_regex("\\s+");
    text = std::regex_replace(text, space_regex, " ");

    // Обрезаем пробелы по краям
    text.erase(0, text.find_first_not_of(" \t\n\r\f\v"));
    text.erase(text.find_last_not_of(" \t\n\r\f\v") + 1);

    return text;
}

void AdvancedHtmlParser::RemoveComments(std::string& html) {
    // Удаляем многострочные комментарии с помощью итеративного подхода
    size_t start_pos = 0;
    while ((start_pos = html.find("<!--", start_pos)) != std::string::npos) {
        size_t end_pos = html.find("-->", start_pos);
        if (end_pos == std::string::npos) {
            break;
        }
        html.erase(start_pos, end_pos - start_pos + 3);
    }
}

void AdvancedHtmlParser::RemoveScriptsAndStyles(std::string& html) {
    // Удаляем script теги
    size_t start_pos = 0;
    while ((start_pos = html.find("<script", start_pos)) != std::string::npos) {
        size_t end_pos = html.find("</script>", start_pos);
        if (end_pos == std::string::npos) {
            break;
        }
        html.erase(start_pos, end_pos - start_pos + 9); // 9 = длина "</script>"
    }

    // Удаляем style теги
    start_pos = 0;
    while ((start_pos = html.find("<style", start_pos)) != std::string::npos) {
        size_t end_pos = html.find("</style>", start_pos);
        if (end_pos == std::string::npos) {
            break;
        }
        html.erase(start_pos, end_pos - start_pos + 8); // 8 = длина "</style>"
    }
}

std::string AdvancedHtmlParser::GetPageTitle(const std::string& html) {
    size_t title_start = html.find("<title>");
    if (title_start == std::string::npos) {
        return "";
    }

    title_start += 7; // Длина "<title>"
    size_t title_end = html.find("</title>", title_start);
    if (title_end == std::string::npos) {
        return "";
    }

    return html.substr(title_start, title_end - title_start);
}

std::vector<std::string> AdvancedHtmlParser::ExtractMetaKeywords(const std::string& html) {
    std::vector<std::string> keywords;

    size_t pos = 0;
    while ((pos = html.find("name=\"keywords\"", pos)) != std::string::npos) {
        size_t content_start = html.find("content=\"", pos);
        if (content_start == std::string::npos) {
            break;
        }

        content_start += 9; // Длина "content=\""
        size_t content_end = html.find("\"", content_start);
        if (content_end == std::string::npos) {
            break;
        }

        std::string content = html.substr(content_start, content_end - content_start);

        // Разделяем ключевые слова по запятым
        size_t keyword_start = 0;
        while (keyword_start < content.length()) {
            size_t keyword_end = content.find(',', keyword_start);
            if (keyword_end == std::string::npos) {
                keyword_end = content.length();
            }

            std::string keyword = content.substr(keyword_start, keyword_end - keyword_start);

            // Убираем пробелы
            keyword.erase(0, keyword.find_first_not_of(" \t"));
            keyword.erase(keyword.find_last_not_of(" \t") + 1);

            if (!keyword.empty()) {
                keywords.push_back(keyword);
            }

            keyword_start = keyword_end + 1;
        }

        pos = content_end;
    }

    return keywords;
}

std::string AdvancedHtmlParser::ExtractMetaDescription(const std::string& html) {
    size_t pos = html.find("name=\"description\"");
    if (pos == std::string::npos) {
        return "";
    }

    size_t content_start = html.find("content=\"", pos);
    if (content_start == std::string::npos) {
        return "";
    }

    content_start += 9; // Длина "content=\""
    size_t content_end = html.find("\"", content_start);
    if (content_end == std::string::npos) {
        return "";
    }

    return html.substr(content_start, content_end - content_start);
}