#include "html_parser.h"
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string HtmlParser::ExtractText(const std::string& html) {
    std::string text = html;

    // Удаляем script и style теги
    std::regex script_regex("<script[^>]*>.*?</script>", std::regex::icase);
    text = std::regex_replace(text, script_regex, "");

    std::regex style_regex("<style[^>]*>.*?</style>", std::regex::icase);
    text = std::regex_replace(text, style_regex, "");

    // Удаляем HTML теги
    std::regex tag_regex("<[^>]*>");
    text = std::regex_replace(text, tag_regex, " ");

    // Заменяем HTML entities
    std::regex nbsp_regex("&nbsp;");
    text = std::regex_replace(text, nbsp_regex, " ");

    std::regex amp_regex("&amp;");
    text = std::regex_replace(text, amp_regex, "&");

    // Удаляем множественные пробелы
    std::regex space_regex("\\s+");
    text = std::regex_replace(text, space_regex, " ");

    return CleanText(text);
}

std::vector<std::string> HtmlParser::ExtractLinks(const std::string& html, const std::string& base_url) {
    std::vector<std::string> links;

    // Простой regex для извлечения ссылок
    std::regex link_regex("<a\\s+[^>]*href=\"([^\"]*)\"[^>]*>", std::regex::icase);

    auto words_begin = std::sregex_iterator(html.begin(), html.end(), link_regex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string link = match[1].str();

        // Пропускаем пустые ссылки и якоря
        if (link.empty() || link[0] == '#') {
            continue;
        }

        // Преобразуем относительные ссылки в абсолютные
        if (link.find("http") != 0) {
            if (link[0] == '/') {
                size_t protocol_end = base_url.find("://");
                if (protocol_end != std::string::npos) {
                    size_t domain_end = base_url.find('/', protocol_end + 3);
                    if (domain_end != std::string::npos) {
                        link = base_url.substr(0, domain_end) + link;
                    }
                    else {
                        link = base_url + link;
                    }
                }
            }
            else {
                size_t last_slash = base_url.find_last_of('/');
                if (last_slash != std::string::npos) {
                    link = base_url.substr(0, last_slash + 1) + link;
                }
            }
        }

        links.push_back(link);
    }

    return links;
}

std::vector<std::string> HtmlParser::ExtractWords(const std::string& text) {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        if (IsValidWord(word)) {
            words.push_back(ToLower(word));
        }
    }

    return words;
}

std::string HtmlParser::GetPageTitle(const std::string& html) {
    size_t title_start = html.find("<title>");
    if (title_start == std::string::npos) {
        return "";
    }

    title_start += 7; // Длина "<title>"
    size_t title_end = html.find("</title>", title_start);
    if (title_end == std::string::npos) {
        return "";
    }

    std::string title = html.substr(title_start, title_end - title_start);
    return CleanText(title);
}

std::string HtmlParser::CleanText(const std::string& text) {
    std::string cleaned;

    for (char c : text) {
        if (std::isalnum(c) || std::isspace(c)) {
            cleaned += c;
        }
        else {
            cleaned += ' ';
        }
    }

    // Удаляем множественные пробелы
    std::regex space_regex("\\s+");
    cleaned = std::regex_replace(cleaned, space_regex, " ");

    // Убираем пробелы в начале и конце
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r\f\v"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r\f\v") + 1);

    return cleaned;
}

bool HtmlParser::IsValidWord(const std::string& word) {
    if (word.length() < 3 || word.length() > 32) {
        return false;
    }

    // Проверяем, что слово состоит в основном из букв
    int letter_count = 0;
    for (char c : word) {
        if (std::isalpha(c)) {
            letter_count++;
        }
    }

    return letter_count >= word.length() * 0.7; // Хотя бы 70% букв
}

std::string HtmlParser::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string HtmlParser::NormalizeUrl(const std::string& url, const std::string& base_url) {
    // Простая реализация нормализации URL
    std::string result = url;

    // Если URL относительный, преобразуем в абсолютный
    if (url.find("://") == std::string::npos) {
        if (!url.empty() && url[0] == '/') {
            // Абсолютный путь относительно домена
            size_t protocol_pos = base_url.find("://");
            if (protocol_pos != std::string::npos) {
                size_t domain_end = base_url.find('/', protocol_pos + 3);
                if (domain_end != std::string::npos) {
                    std::string domain = base_url.substr(0, domain_end);
                    result = domain + url;
                }
                else {
                    result = base_url + url;
                }
            }
        }
        else {
            // Относительный путь
            size_t last_slash = base_url.find_last_of('/');
            if (last_slash != std::string::npos) {
                std::string base_path = base_url.substr(0, last_slash + 1);
                result = base_path + url;
            }
        }
    }

    return result;
}