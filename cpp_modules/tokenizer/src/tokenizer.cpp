#include "tokenizer.h"
#include <fstream>
#include <cctype>
#include <algorithm>
#include <cmath>

namespace search {

// Конструкторы
Tokenizer::Tokenizer() 
    : min_token_length_(2),
      max_token_length_(50),
      remove_numbers_(false),
      remove_punctuation_(true),
      case_folding_(true) {
    // Добавляем базовые стоп-слова для русского языка
    const char* russian_stopwords[] = {
        "и", "в", "во", "не", "что", "он", "на", "я", "с", "со", "как", "а",
        "то", "все", "она", "так", "его", "но", "да", "ты", "к", "у", "же",
        "вы", "за", "бы", "по", "только", "ее", "мне", "было", "вот", "от",
        "меня", "еще", "нет", "о", "из", "ему", "теперь", "когда", "даже",
        "ну", "ли", "если", "уже", "или", "ни", "быть", "был", "него", "до",
        "вас", "нибудь", "опять", "уж", "вам", "ведь", "там", "потом", "себя",
        "ничего", "ей", "может", "они", "тут", "где", "есть", "надо", "ней",
        "для", "мы", "тебя", "их", "чем", "была", "сам", "чтоб", "без", "будто",
        "чего", "раз", "тоже", "себе", "под", "будет", "ж", "тогда", "кто",
        "этот", "того", "потому", "этого", "какой", "совсем", "ним", "здесь",
        "этом", "один", "почти", "мой", "тем", "чтобы", "нее", "сейчас", "были",
        "куда", "зачем", "всех", "никогда", "можно", "при", "наконец", "два",
        "об", "другой", "хоть", "после", "над", "больше", "тот", "через",
        "эти", "нас", "про", "всего", "них", "какая", "много", "разве", "три",
        "эту", "моя", "впрочем", "хорошо", "свою", "этой", "перед", "иногда",
        "лучше", "чуть", "том", "нельзя", "такой", "им", "более", "всегда",
        "конечно", "всю", "между", nullptr
    };
    
    for (int i = 0; russian_stopwords[i] != nullptr; ++i) {
        stopwords_.insert(ds::String(russian_stopwords[i]), true);
    }
}

Tokenizer::Tokenizer(const ds::String& stopwords_path)
    : Tokenizer() {
    load_stopwords(stopwords_path);
}

Tokenizer::Tokenizer(const ds::String& stopwords_path,
                     size_t min_token_length,
                     size_t max_token_length,
                     bool remove_numbers,
                     bool remove_punctuation,
                     bool case_folding)
    : min_token_length_(min_token_length),
      max_token_length_(max_token_length),
      remove_numbers_(remove_numbers),
      remove_punctuation_(remove_punctuation),
      case_folding_(case_folding) {
    if (!stopwords_path.empty()) {
        load_stopwords(stopwords_path);
    }
}

bool Tokenizer::load_stopwords(const ds::String& filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    ds::String line;
    char ch;
    while (file.get(ch)) {
        if (ch == '\n') {
            if (!line.empty()) {
                if (case_folding_) {
                    line = line.to_lower();
                }
                stopwords_.insert(line, true);
                line.clear();
            }
        } else {
            line.push_back(ch);
        }
    }
    
    // Добавляем последнюю строку, если она есть
    if (!line.empty()) {
        if (case_folding_) {
            line = line.to_lower();
        }
        stopwords_.insert(line, true);
    }
    
    return true;
}

ds::String Tokenizer::normalize_token(const ds::String& token) const {
    if (token.empty()) {
        return token;
    }
    
    ds::String result;
    result.reserve(token.size());
    
    // Удаляем пунктуацию с начала и конца
    size_t start = 0;
    size_t end = token.size();
    
    if (remove_punctuation_) {
        while (start < end && is_punctuation(token[start])) {
            ++start;
        }
        while (end > start && is_punctuation(token[end - 1])) {
            --end;
        }
    }
    
    // Копируем символы с учетом настроек
    for (size_t i = start; i < end; ++i) {
        char c = token[i];
        
        // Пропускаем цифры, если нужно
        if (remove_numbers_ && std::isdigit(static_cast<unsigned char>(c))) {
            continue;
        }
        
        // Пропускаем пунктуацию внутри слова (например, апострофы)
        if (remove_punctuation_ && is_punctuation(c) && c != '\'' && c != '-') {
            continue;
        }
        
        // Приводим к нижнему регистру, если нужно
        if (case_folding_) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        
        result.push_back(c);
    }
    
    return result;
}

ds::Vector<ds::String> Tokenizer::tokenize(const ds::String& text) const {
    ds::Vector<ds::String> tokens;
    
    if (text.empty()) {
        return tokens;
    }
    
    ds::String current_token;
    current_token.reserve(max_token_length_);
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        
        if (is_delimiter(c)) {
            // Завершаем текущий токен
            if (!current_token.empty()) {
                ds::String normalized = normalize_token(current_token);
                
                // Проверяем длину и не является ли стоп-словом
                if (normalized.size() >= min_token_length_ && 
                    normalized.size() <= max_token_length_ &&
                    !is_stopword(normalized)) {
                    tokens.push_back(normalized);
                }
                
                current_token.clear();
            }
        } else {
            // Добавляем символ к текущему токену
            current_token.push_back(c);
            
            // Если токен превысил максимальную длину, начинаем новый
            if (current_token.size() > max_token_length_) {
                ds::String normalized = normalize_token(current_token);
                if (normalized.size() >= min_token_length_ && 
                    !is_stopword(normalized)) {
                    tokens.push_back(normalized);
                }
                current_token.clear();
            }
        }
    }
    
    // Добавляем последний токен, если есть
    if (!current_token.empty()) {
        ds::String normalized = normalize_token(current_token);
        if (normalized.size() >= min_token_length_ && 
            normalized.size() <= max_token_length_ &&
            !is_stopword(normalized)) {
            tokens.push_back(normalized);
        }
    }
    
    return tokens;
}

ds::Vector<Tokenizer::TokenWithPosition> Tokenizer::tokenize_with_positions(const ds::String& text) const {
    ds::Vector<TokenWithPosition> tokens;
    
    if (text.empty()) {
        return tokens;
    }
    
    ds::String current_token;
    size_t token_start = 0;
    current_token.reserve(max_token_length_);
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        
        if (is_delimiter(c)) {
            // Завершаем текущий токен
            if (!current_token.empty()) {
                ds::String normalized = normalize_token(current_token);
                
                if (normalized.size() >= min_token_length_ && 
                    normalized.size() <= max_token_length_ &&
                    !is_stopword(normalized)) {
                    
                    TokenWithPosition token_info;
                    token_info.token = normalized;
                    token_info.position = token_start;
                    token_info.length = current_token.size();
                    
                    tokens.push_back(token_info);
                }
                
                current_token.clear();
            }
            
            // Следующий токен начнется после разделителя
            token_start = i + 1;
        } else {
            // Начинаем новый токен, если это первый символ
            if (current_token.empty()) {
                token_start = i;
            }
            
            // Добавляем символ к текущему токену
            current_token.push_back(c);
            
            // Если токен превысил максимальную длину, начинаем новый
            if (current_token.size() > max_token_length_) {
                ds::String normalized = normalize_token(current_token);
                if (normalized.size() >= min_token_length_ && 
                    !is_stopword(normalized)) {
                    
                    TokenWithPosition token_info;
                    token_info.token = normalized;
                    token_info.position = token_start;
                    token_info.length = current_token.size();
                    
                    tokens.push_back(token_info);
                }
                
                current_token.clear();
                token_start = i + 1;
            }
        }
    }
    
    // Добавляем последний токен, если есть
    if (!current_token.empty()) {
        ds::String normalized = normalize_token(current_token);
        if (normalized.size() >= min_token_length_ && 
            normalized.size() <= max_token_length_ &&
            !is_stopword(normalized)) {
            
            TokenWithPosition token_info;
            token_info.token = normalized;
            token_info.position = token_start;
            token_info.length = current_token.size();
            
            tokens.push_back(token_info);
        }
    }
    
    return tokens;
}

ds::Vector<ds::Vector<ds::String>> Tokenizer::batch_tokenize(const ds::Vector<ds::String>& texts) const {
    ds::Vector<ds::Vector<ds::String>> result;
    result.reserve(texts.size());
    
    for (const auto& text : texts) {
        result.push_back(tokenize(text));
    }
    
    return result;
}

Tokenizer::TokenStats Tokenizer::calculate_stats(const ds::Vector<ds::Vector<ds::String>>& tokenized_docs) const {
    TokenStats stats = {0, 0, 0, 0.0, 0.0};
    
    if (tokenized_docs.empty()) {
        return stats;
    }
    
    ds::HashTable<ds::String, bool> unique_tokens;
    size_t total_tokens = 0;
    size_t total_chars = 0;
    
    for (const auto& doc_tokens : tokenized_docs) {
        for (const auto& token : doc_tokens) {
            ++total_tokens;
            total_chars += token.size();
            unique_tokens.insert(token, true);
        }
    }
    
    stats.total_tokens = total_tokens;
    stats.unique_tokens = unique_tokens.size();
    stats.avg_token_length = total_tokens > 0 ? static_cast<double>(total_chars) / total_tokens : 0.0;
    stats.tokens_per_document = static_cast<double>(total_tokens) / tokenized_docs.size();
    
    return stats;
}

bool Tokenizer::save_config(const ds::String& filepath) const {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << "min_token_length=" << min_token_length_ << "\n";
    file << "max_token_length=" << max_token_length_ << "\n";
    file << "remove_numbers=" << (remove_numbers_ ? "true" : "false") << "\n";
    file << "remove_punctuation=" << (remove_punctuation_ ? "true" : "false") << "\n";
    file << "case_folding=" << (case_folding_ ? "true" : "false") << "\n";
    
    return true;
}

bool Tokenizer::load_config(const ds::String& filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    ds::String line;
    char ch;
    
    while (file.get(ch)) {
        if (ch == '\n') {
            if (!line.empty()) {
                auto parts = line.split('=');
                if (parts.size() == 2) {
                    ds::String key = parts[0].trim();
                    ds::String value = parts[1].trim();
                    
                    if (key == "min_token_length") {
                        min_token_length_ = std::stoul(value.c_str());
                    } else if (key == "max_token_length") {
                        max_token_length_ = std::stoul(value.c_str());
                    } else if (key == "remove_numbers") {
                        remove_numbers_ = (value == "true");
                    } else if (key == "remove_punctuation") {
                        remove_punctuation_ = (value == "true");
                    } else if (key == "case_folding") {
                        case_folding_ = (value == "true");
                    }
                }
                line.clear();
            }
        } else {
            line.push_back(ch);
        }
    }
    
    return true;
}

} // namespace search